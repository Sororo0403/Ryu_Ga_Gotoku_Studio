#include "scene/GameScene.h"

#include "camera/CameraManager.h"
#include "core/AssetManager.h"
#include "core/WinApp.h"
#include "input/Input.h"
#include "model/Material.h"
#include "model/ModelManager.h"

#ifdef _DEBUG
#include "imgui.h"
#endif

#include <algorithm>
#include <cmath>
#include <system_error>

namespace {
constexpr float kFloorTopY = 0.04f;
constexpr float kEnemyRespawnSeconds = 1.0f;
constexpr DirectX::XMFLOAT3 kPlayerSpawnPosition{-1.6f, kFloorTopY, 0.0f};
constexpr DirectX::XMFLOAT3 kEnemySpawnPosition{1.35f, kFloorTopY, 0.0f};
constexpr float kOrbitYawSpeed = 1.8f;
constexpr float kOrbitPitchSpeed = 1.2f;
constexpr float kOrbitMouseYawSpeed = 0.0035f;
constexpr float kOrbitMousePitchSpeed = 0.0030f;
constexpr float kOrbitMinPitch = 0.08f;
constexpr float kOrbitMaxPitch = 0.90f;
constexpr float kOrbitMinDistance = 4.5f;
constexpr float kOrbitMaxDistance = 14.0f;
constexpr float kOrbitTargetHeight = 1.05f;

Material MakeMaterial(float r, float g, float b, float a = 1.0f) {
    Material material{};
    material.color = {r, g, b, a};
    material.enableTexture = 0;
    material.roughness = 0.82f;
    material.reflectionStrength = 0.02f;
    if (a < 1.0f) {
        material.blendMode = static_cast<int32_t>(BlendMode::Transparent);
        material.depthWrite = 0;
    }
    return material;
}
} // namespace

void GameScene::Initialize(const SceneContext& ctx) {
    BaseScene::Initialize(ctx);
    debugUiMode_ = false;
    if (ctx_->systems.winApp) {
        ctx_->systems.winApp->SetCursorVisible(false);
    }
    InitializeCamera();
    InitializeModels();
    InitializeAttackData();
    ResetCombat();

    if (ctx_->systems.log) {
        ctx_->systems.log("Weak / strong combo prototype initialized");
    }
}

void GameScene::Update() {
    if (!ctx_) {
        return;
    }

    const float deltaTime = std::clamp(ctx_->frame.deltaTime, 0.0f, 1.0f / 20.0f);

    if (Input* input = ctx_->systems.input) {
        if (input->IsKeyTrigger(DIK_F1)) {
            debugUiMode_ = !debugUiMode_;
            if (ctx_->systems.winApp) {
                ctx_->systems.winApp->SetCursorVisible(debugUiMode_);
            }
        }
        if (input->IsKeyTrigger(DIK_ESCAPE)) {
            if (ctx_->systems.winApp) {
                ctx_->systems.winApp->RequestClose();
            }
            return;
        }
        if (!debugUiMode_) {
            player_.UpdateInput(*input, deltaTime);
        }
        if (player_.ShouldResetRequested()) {
            ResetCombat();
            UpdateOrbitCamera(deltaTime);
            return;
        }
    }

    if (player_.GetHitStopTimer() > 0.0f) {
        player_.NotifyHitStopConsumed(deltaTime);
        UpdateOrbitCamera(deltaTime);
        return;
    }

    if (UpdateEnemyRespawn(deltaTime)) {
        player_.Update(deltaTime, debugUiMode_ ? nullptr : ctx_->systems.input,
                       CalculateCameraForward(), CalculateCameraRight(),
                       enemy_.GetTransform().position);
        UpdateCollisionBodies();
        UpdateOrbitCamera(deltaTime);
        return;
    }

    player_.Update(deltaTime, debugUiMode_ ? nullptr : ctx_->systems.input, CalculateCameraForward(),
                   CalculateCameraRight(), enemy_.GetTransform().position);
    enemy_.Update(deltaTime, player_.GetTransform().position);
    UpdateCollisionBodies();
    ResolveAttackHit();
    UpdateOrbitCamera(deltaTime);
}

void GameScene::Draw() {
    if (!ctx_ || !ctx_->rendering.model || !ctx_->systems.cameraManager) {
        return;
    }

    const Camera* camera = ctx_->systems.cameraManager->GetActiveCamera();
    if (!camera) {
        return;
    }

    ctx_->rendering.model->Draw(floorModelId_, floorTransform_, *camera);
    DrawFighter(player_, playerModelId_);
    if (!enemyRespawnPending_) {
        DrawFighter(enemy_, enemyModelId_);
    }
    DrawAttackDebug();
}

void GameScene::DrawPostProcessOverlay() {
    DrawDebugUi();
}

void GameScene::InitializeAttackData() {
    attackDataPath_ = AssetManager::ResolvePath("resources/combat/player_attacks.json");
    if (player_.LoadAttackData(attackDataPath_)) {
        attackDataStatus_ = "Loaded: " + attackDataPath_.string();
        return;
    }

    player_.ResetAttackDataToDefaults();
    if (player_.SaveAttackData(attackDataPath_)) {
        attackDataStatus_ = "Created defaults: " + attackDataPath_.string();
    } else {
        attackDataStatus_ = "Using built-in defaults; failed to write: " + attackDataPath_.string();
    }
}

void GameScene::InitializeCamera() {
    if (!ctx_ || !ctx_->systems.cameraManager) {
        return;
    }

    float aspect = 16.0f / 9.0f;
    if (ctx_->systems.winApp && ctx_->systems.winApp->GetHeight() > 0) {
        aspect = static_cast<float>(ctx_->systems.winApp->GetWidth()) /
                 static_cast<float>(ctx_->systems.winApp->GetHeight());
    }

    Camera& camera = ctx_->systems.cameraManager->CreateCamera("combat", aspect);
    cameraYaw_ = 0.0f;
    cameraPitch_ = 0.32f;
    cameraDistance_ = 6.6f;
    camera.SetPerspectiveFovDeg(48.0f);
    ctx_->systems.cameraManager->SetActiveCamera("combat");
    UpdateOrbitCamera(0.0f);
}

void GameScene::UpdateOrbitCamera(float deltaTime) {
    if (!ctx_ || !ctx_->systems.cameraManager) {
        return;
    }

    if (Input* input = debugUiMode_ ? nullptr : ctx_->systems.input) {
        float yawInput = 0.0f;
        float pitchInput = 0.0f;
        yawInput += input->GetGamepadRightStickX();
        pitchInput -= input->GetGamepadRightStickY();

        cameraYaw_ += yawInput * kOrbitYawSpeed * deltaTime;
        cameraPitch_ += pitchInput * kOrbitPitchSpeed * deltaTime;

        cameraYaw_ -= static_cast<float>(input->GetMouseDX()) * kOrbitMouseYawSpeed;
        cameraPitch_ += static_cast<float>(input->GetMouseDY()) * kOrbitMousePitchSpeed;
    }

    cameraPitch_ = std::clamp(cameraPitch_, kOrbitMinPitch, kOrbitMaxPitch);
    cameraDistance_ = std::clamp(cameraDistance_, kOrbitMinDistance, kOrbitMaxDistance);

    const DirectX::XMFLOAT3 target = CalculateCameraTarget();
    const float cosPitch = std::cos(cameraPitch_);
    const DirectX::XMFLOAT3 offset{
        std::sin(cameraYaw_) * cosPitch * cameraDistance_,
        std::sin(cameraPitch_) * cameraDistance_,
        -std::cos(cameraYaw_) * cosPitch * cameraDistance_,
    };

    Camera* camera = ctx_->systems.cameraManager->FindCamera("combat");
    if (!camera) {
        camera = ctx_->systems.cameraManager->GetActiveCamera();
    }
    if (!camera) {
        return;
    }

    camera->SetLookAt({target.x + offset.x, target.y + offset.y, target.z + offset.z}, target);
}

DirectX::XMFLOAT3 GameScene::CalculateCameraTarget() const {
    DirectX::XMFLOAT3 target = player_.GetTransform().position;
    target.y += kOrbitTargetHeight;
    return target;
}

DirectX::XMFLOAT3 GameScene::CalculateCameraForward() const {
    return {-std::sin(cameraYaw_), 0.0f, std::cos(cameraYaw_)};
}

DirectX::XMFLOAT3 GameScene::CalculateCameraRight() const {
    return {std::cos(cameraYaw_), 0.0f, std::sin(cameraYaw_)};
}

void GameScene::InitializeModels() {
    if (!ctx_ || !ctx_->rendering.model) {
        return;
    }

    playerModelId_ = ctx_->rendering.model->CreateBox(0, MakeMaterial(0.16f, 0.42f, 0.95f), 0.8f,
                                                      1.75f, 0.55f);
    enemyModelId_ = ctx_->rendering.model->CreateBox(0, MakeMaterial(0.86f, 0.20f, 0.18f), 0.8f,
                                                     1.75f, 0.55f);
    floorModelId_ = ctx_->rendering.model->CreateBox(0, MakeMaterial(0.18f, 0.19f, 0.20f), 9.0f,
                                                     0.08f, 7.0f);
    hitBoxModelId_ = ctx_->rendering.model->CreateBox(0, MakeMaterial(1.0f, 0.78f, 0.12f, 0.38f),
                                                      1.0f, 1.0f, 1.0f);

    floorTransform_.position = {0.0f, -0.04f, 0.0f};
    floorTransform_.scale = {1.0f, 1.0f, 1.0f};
}

void GameScene::ResetCombat() {
    collision_.Clear();

    player_.Reset(kPlayerSpawnPosition, 1.0f, 100.0f);
    enemy_.Reset(kEnemySpawnPosition, -1.0f, 100.0f);
    enemyRespawnTimer_ = 0.0f;
    enemyRespawnPending_ = false;

    CollisionManager::BodyDesc playerBody{};
    playerBody.shape = player_.MakeHurtShape();
    playerBody.filter.layer = 1u << 0;
    playerBody.filter.mask = 1u << 1;
    playerBody.userData = &player_;
    player_.SetHurtBody(collision_.AddBody(playerBody));

    CollisionManager::BodyDesc enemyBody{};
    enemyBody.shape = enemy_.MakeHurtShape();
    enemyBody.filter.layer = 1u << 1;
    enemyBody.filter.mask = 1u << 0;
    enemyBody.userData = &enemy_;
    enemy_.SetHurtBody(collision_.AddBody(enemyBody));
}

bool GameScene::UpdateEnemyRespawn(float deltaTime) {
    if (!enemyRespawnPending_) {
        return false;
    }

    enemyRespawnTimer_ = std::max(0.0f, enemyRespawnTimer_ - deltaTime);
    if (enemyRespawnTimer_ <= 0.0f) {
        RespawnEnemy();
        return false;
    }
    return true;
}

void GameScene::RespawnEnemy() {
    enemy_.Reset(kEnemySpawnPosition, -1.0f, 100.0f);
    enemyRespawnTimer_ = 0.0f;
    enemyRespawnPending_ = false;

    if (enemy_.GetHurtBody() != CollisionManager::kInvalidBodyId) {
        collision_.UpdateShape(enemy_.GetHurtBody(), enemy_.MakeHurtShape());
        collision_.SetActive(enemy_.GetHurtBody(), true);
    }
}

void GameScene::StartEnemyRespawn() {
    if (enemyRespawnPending_) {
        return;
    }

    enemyRespawnTimer_ = kEnemyRespawnSeconds;
    enemyRespawnPending_ = true;
    if (enemy_.GetHurtBody() != CollisionManager::kInvalidBodyId) {
        collision_.SetActive(enemy_.GetHurtBody(), false);
    }
}

void GameScene::UpdateCollisionBodies() {
    if (player_.GetHurtBody() != CollisionManager::kInvalidBodyId) {
        collision_.UpdateShape(player_.GetHurtBody(), player_.MakeHurtShape());
    }
    if (enemy_.GetHurtBody() != CollisionManager::kInvalidBodyId) {
        collision_.UpdateShape(enemy_.GetHurtBody(), enemy_.MakeHurtShape());
    }
}

void GameScene::ResolveAttackHit() {
    if (!player_.IsAttackActive()) {
        return;
    }

    const CollisionManager::Shape attackShape = player_.MakeAttackShape(hitBoxTransform_);
    if (player_.HasCurrentAttackHit() || enemy_.GetHealth() <= 0.0f) {
        return;
    }

    CollisionManager::BodyDesc attackBody{};
    attackBody.shape = attackShape;
    attackBody.filter.layer = 1u << 0;
    attackBody.filter.mask = 1u << 1;
    attackBody.userData = &player_;

    const CollisionManager::BodyId attackBodyId = collision_.AddBody(attackBody);
    CollisionManager::Hit hit{};
    const bool hitEnemy = collision_.Test(attackBodyId, enemy_.GetHurtBody(), &hit);
    collision_.RemoveBody(attackBodyId);

    if (!hitEnemy) {
        return;
    }

    const combat::AttackMove& attack = player_.CurrentAttack();
    enemy_.ApplyHit(attack, player_.GetTransform().position, player_.GetFacingDirection());
    if (enemy_.GetHurtBody() != CollisionManager::kInvalidBodyId) {
        collision_.UpdateShape(enemy_.GetHurtBody(), enemy_.MakeHurtShape());
    }
    if (enemy_.GetHealth() <= 0.0f) {
        StartEnemyRespawn();
    }
    player_.ApplyHitStop(attack.hitStop);
    player_.MarkCurrentAttackHit();
}

void GameScene::DrawFighter(const character::CombatCharacter& fighter, uint32_t modelId) const {
    const Camera* camera = ctx_->systems.cameraManager->GetActiveCamera();
    if (!camera || !ctx_->rendering.model) {
        return;
    }

    ctx_->rendering.model->Draw(modelId, fighter.GetTransform(), *camera);
}

void GameScene::DrawAttackDebug() const {
    if (!player_.IsAttackActive() || !ctx_->rendering.model || !ctx_->systems.cameraManager) {
        return;
    }

    const Camera* camera = ctx_->systems.cameraManager->GetActiveCamera();
    if (!camera) {
        return;
    }

    ctx_->rendering.model->Draw(hitBoxModelId_, hitBoxTransform_, *camera);
}

void GameScene::DrawDebugUi() {
#ifdef _DEBUG
    if (!ctx_) {
        return;
    }

    const char* attackName =
        player_.GetComboIndex() >= 0 ? player_.CurrentAttack().name.c_str() : "None";

    ImGui::SetNextWindowPos(ImVec2(18.0f, 18.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430.0f, 190.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Combat Debug")) {
        ImGui::TextUnformatted("Weak / Strong Combo Prototype");
        ImGui::Separator();
        ImGui::Text("Mode: %s  (F1 toggles Game/UI)", debugUiMode_ ? "UI" : "Game");
        ImGui::TextUnformatted("Move: WASD / Left Stick");
        ImGui::TextUnformatted("Weak: J / Left Click / Pad X");
        ImGui::TextUnformatted("Strong: K / Pad Y");
        ImGui::TextUnformatted("Lock: LShift / Right Click / LB");
        ImGui::TextUnformatted("Camera: Mouse / Right Stick");
        ImGui::TextUnformatted("Reset: R  Exit: Esc");
        ImGui::Separator();
        ImGui::Text("State: %s", combat::CombatStateName(player_.GetState()));
        ImGui::Text("Move: %s", attackName);
        ImGui::Text("Enemy HP: %.0f", enemy_.GetHealth());
        ImGui::Text("WeakBuf: %s  StrongBuf: %s  Lock: %s",
                    player_.HasBufferedAttack() ? "yes" : "no",
                    player_.HasBufferedStrongAttack() ? "yes" : "no",
                    player_.IsLockOnHeld() ? "on" : "off");
        ImGui::Text("Respawn: %.1f", enemyRespawnPending_ ? enemyRespawnTimer_ : 0.0f);
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(18.0f, 230.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520.0f, 650.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Attack Tuning")) {
        ImGui::TextWrapped("%s", attackDataStatus_.c_str());
        if (ImGui::Button("Save JSON")) {
            attackDataStatus_ = player_.SaveAttackData(attackDataPath_)
                                    ? "Saved: " + attackDataPath_.string()
                                    : "Save failed: " + attackDataPath_.string();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload JSON")) {
            attackDataStatus_ = player_.LoadAttackData(attackDataPath_)
                                    ? "Reloaded: " + attackDataPath_.string()
                                    : "Reload failed; restored defaults";
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Defaults")) {
            player_.ResetAttackDataToDefaults();
            attackDataStatus_ = "Reset to built-in defaults";
        }

        if (ImGui::CollapsingHeader("Weak Attacks", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (int i = 0; i < player_.GetWeakAttackCount(); ++i) {
                std::string label = "Weak " + std::to_string(i + 1);
                DrawAttackMoveEditor(player_.GetWeakAttack(i), label.c_str());
            }
        }
        if (ImGui::CollapsingHeader("Strong Attacks", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (int i = 0; i < player_.GetStrongAttackCount(); ++i) {
                std::string label = "Strong " + std::to_string(i + 1);
                DrawAttackMoveEditor(player_.GetStrongAttack(i), label.c_str());
            }
        }
    }
    ImGui::End();
#endif
}

void GameScene::DrawAttackMoveEditor(combat::AttackMove& move, const char* label) {
#ifdef _DEBUG
    if (!ImGui::TreeNode(label)) {
        return;
    }

    char nameBuffer[64]{};
    const size_t copyLength = std::min(move.name.size(), sizeof(nameBuffer) - 1);
    move.name.copy(nameBuffer, copyLength);
    nameBuffer[copyLength] = '\0';
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        move.name = nameBuffer;
    }

    ImGui::DragFloat("Startup", &move.startup, 0.005f, 0.0f, 2.0f, "%.3f");
    ImGui::DragFloat("Active", &move.active, 0.005f, 0.0f, 2.0f, "%.3f");
    ImGui::DragFloat("Recovery", &move.recovery, 0.005f, 0.0f, 2.0f, "%.3f");
    ImGui::DragFloat("Chain Start", &move.chainStart, 0.005f, 0.0f, 2.0f, "%.3f");
    ImGui::DragFloat("Chain End", &move.chainEnd, 0.005f, 0.0f, 2.0f, "%.3f");
    ImGui::DragFloat("Damage", &move.damage, 0.25f, 0.0f, 200.0f, "%.1f");
    ImGui::DragFloat("Hit Stop", &move.hitStop, 0.005f, 0.0f, 1.0f, "%.3f");
    ImGui::DragFloat("Hit Stun", &move.hitStun, 0.005f, 0.0f, 2.0f, "%.3f");
    ImGui::DragFloat("Knockback", &move.knockback, 0.025f, 0.0f, 8.0f, "%.3f");
    ImGui::DragFloat("Pull Distance", &move.pullDistance, 0.025f, 0.0f, 4.0f, "%.3f");
    ImGui::DragFloat("Pull Strength", &move.pullStrength, 0.01f, 0.0f, 1.0f, "%.3f");

    int reaction = move.reaction == combat::HitReaction::Stick ? 0 : 1;
    const char* reactions[] = {"Stick", "Knockback"};
    if (ImGui::Combo("Reaction", &reaction, reactions, 2)) {
        move.reaction =
            reaction == 0 ? combat::HitReaction::Stick : combat::HitReaction::Knockback;
    }

    ImGui::DragFloat3("Hit Box Offset", &move.hitBoxOffset.x, 0.025f, -4.0f, 4.0f, "%.3f");
    ImGui::DragFloat3("Hit Box Size", &move.hitBoxSize.x, 0.025f, 0.05f, 5.0f, "%.3f");
    ImGui::DragFloat("Lunge Distance", &move.lungeDistance, 0.025f, 0.0f, 5.0f, "%.3f");
    ImGui::DragFloat3("Lunge Direction", &move.lungeDirection.x, 0.025f, -1.0f, 1.0f,
                      "%.3f");
    ImGui::DragFloat("Lunge Start", &move.lungeStart, 0.005f, 0.0f, 2.0f, "%.3f");
    ImGui::DragFloat("Lunge End", &move.lungeEnd, 0.005f, 0.0f, 2.0f, "%.3f");

    ImGui::TreePop();
#endif
}
