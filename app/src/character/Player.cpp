#include "character/Player.h"

#include "input/Input.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace {
constexpr float kInputBufferSeconds = 0.18f;
constexpr float kPostComboCooldownSeconds = 0.55f;
constexpr float kPlayerMoveSpeed = 2.8f;
constexpr float kStageHalfWidth = 4.5f;
constexpr float kStageHalfDepth = 3.5f;
constexpr float kPlayerHalfWidth = 0.4f;
constexpr float kPlayerHalfDepth = 0.275f;
constexpr float kAttackLungeMinEnemySpacing = 0.58f;

nlohmann::json Vec3ToJson(const DirectX::XMFLOAT3& value) {
    return nlohmann::json::array({value.x, value.y, value.z});
}

DirectX::XMFLOAT3 JsonToVec3(const nlohmann::json& json, const DirectX::XMFLOAT3& fallback) {
    if (!json.is_array() || json.size() != 3) {
        return fallback;
    }

    return {
        json[0].get<float>(),
        json[1].get<float>(),
        json[2].get<float>(),
    };
}

const char* HitReactionToString(combat::HitReaction reaction) {
    switch (reaction) {
    case combat::HitReaction::Stick:
        return "Stick";
    case combat::HitReaction::Knockback:
    default:
        return "Knockback";
    }
}

combat::HitReaction HitReactionFromString(const std::string& value,
                                          combat::HitReaction fallback) {
    if (value == "Stick") {
        return combat::HitReaction::Stick;
    }
    if (value == "Knockback") {
        return combat::HitReaction::Knockback;
    }
    return fallback;
}

nlohmann::json AttackMoveToJson(const combat::AttackMove& move) {
    return {
        {"name", move.name},
        {"startup", move.startup},
        {"active", move.active},
        {"recovery", move.recovery},
        {"chainStart", move.chainStart},
        {"chainEnd", move.chainEnd},
        {"damage", move.damage},
        {"hitStop", move.hitStop},
        {"knockback", move.knockback},
        {"pullDistance", move.pullDistance},
        {"pullStrength", move.pullStrength},
        {"reaction", HitReactionToString(move.reaction)},
        {"hitBoxOffset", Vec3ToJson(move.hitBoxOffset)},
        {"hitBoxSize", Vec3ToJson(move.hitBoxSize)},
        {"lungeDistance", move.lungeDistance},
        {"lungeDirection", Vec3ToJson(move.lungeDirection)},
        {"lungeStart", move.lungeStart},
        {"lungeEnd", move.lungeEnd},
        {"hitStun", move.hitStun},
    };
}

void ApplyAttackMoveJson(const nlohmann::json& json, combat::AttackMove& move) {
    if (!json.is_object()) {
        return;
    }

    move.name = json.value("name", move.name);
    move.startup = json.value("startup", move.startup);
    move.active = json.value("active", move.active);
    move.recovery = json.value("recovery", move.recovery);
    move.chainStart = json.value("chainStart", move.chainStart);
    move.chainEnd = json.value("chainEnd", move.chainEnd);
    move.damage = json.value("damage", move.damage);
    move.hitStop = json.value("hitStop", move.hitStop);
    move.knockback = json.value("knockback", move.knockback);
    move.pullDistance = json.value("pullDistance", move.pullDistance);
    move.pullStrength = json.value("pullStrength", move.pullStrength);
    move.reaction = HitReactionFromString(json.value("reaction", std::string{}), move.reaction);
    if (json.contains("hitBoxOffset")) {
        move.hitBoxOffset = JsonToVec3(json["hitBoxOffset"], move.hitBoxOffset);
    }
    if (json.contains("hitBoxSize")) {
        move.hitBoxSize = JsonToVec3(json["hitBoxSize"], move.hitBoxSize);
    }
    move.lungeDistance = json.value("lungeDistance", move.lungeDistance);
    if (json.contains("lungeDirection")) {
        move.lungeDirection = JsonToVec3(json["lungeDirection"], move.lungeDirection);
    }
    move.lungeStart = json.value("lungeStart", move.lungeStart);
    move.lungeEnd = json.value("lungeEnd", move.lungeEnd);
    move.hitStun = json.value("hitStun", move.hitStun);
}
} // namespace

namespace character {

void Player::ResetAttackDataToDefaults() {
    weakCombo_[0] = {"Weak 1", 0.14f, 0.15f, 0.28f, 0.24f, 0.48f, 8.0f, 0.06f, 0.45f,
                     0.9f, 0.72f, combat::HitReaction::Stick, {0.65f, 0.9f, 0.0f},
                     {0.78f, 0.75f, 0.92f}, 0.0f, {1.0f, 0.0f, 0.0f}, 0.0f, 0.0f};
    weakCombo_[1] = {"Weak 2", 0.13f, 0.16f, 0.30f, 0.24f, 0.50f, 9.0f, 0.065f, 0.55f,
                     0.95f, 0.74f, combat::HitReaction::Stick, {0.72f, 0.9f, 0.0f},
                     {0.82f, 0.78f, 0.92f}, 0.0f, {1.0f, 0.0f, 0.0f}, 0.0f, 0.0f};
    weakCombo_[2] = {"Weak 3", 0.16f, 0.17f, 0.34f, 0.28f, 0.56f, 11.0f, 0.07f, 0.72f,
                     1.0f, 0.78f, combat::HitReaction::Stick, {0.76f, 0.82f, 0.0f},
                     {0.9f, 0.78f, 1.0f}, 0.55f, {1.0f, 0.0f, 0.0f}, 0.05f, 0.20f};
    weakCombo_[3] = {"Weak 4", 0.22f, 0.20f, 0.52f, 0.0f, 0.0f, 18.0f, 0.09f, 1.45f,
                     0.0f, 0.0f, combat::HitReaction::Knockback, {0.86f, 0.95f, 0.0f},
                     {1.05f, 0.95f, 1.08f}, 0.78f, {1.0f, 0.0f, 0.0f}, 0.04f, 0.25f};
    strongCombo_[0] = {"Strong 1", 0.26f, 0.18f, 0.48f, 0.34f, 0.66f, 18.0f, 0.10f,
                       1.20f, 0.0f, 0.0f, combat::HitReaction::Knockback,
                       {0.88f, 0.95f, 0.0f}, {1.12f, 0.98f, 1.08f}, 0.82f,
                       {1.0f, 0.0f, 0.0f}, 0.05f, 0.27f, 0.42f};
    strongCombo_[1] = {"Strong 2", 0.34f, 0.20f, 0.70f, 0.0f, 0.0f, 32.0f, 0.15f,
                       2.35f, 0.0f, 0.0f, combat::HitReaction::Knockback,
                       {1.02f, 0.98f, 0.0f}, {1.34f, 1.08f, 1.18f}, 1.10f,
                       {1.0f, 0.0f, 0.0f}, 0.05f, 0.34f, 0.60f};
}

bool Player::LoadAttackData(const std::filesystem::path& path) {
    ResetAttackDataToDefaults();
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    try {
        nlohmann::json root{};
        file >> root;
        if (const auto weak = root.find("weak"); weak != root.end() && weak->is_array()) {
            const size_t count =
                std::min(weak->size(), static_cast<size_t>(kMaxWeakCombo));
            for (size_t i = 0; i < count; ++i) {
                ApplyAttackMoveJson((*weak)[i], weakCombo_[i]);
            }
        }
        if (const auto strong = root.find("strong");
            strong != root.end() && strong->is_array()) {
            const size_t count =
                std::min(strong->size(), static_cast<size_t>(kMaxStrongCombo));
            for (size_t i = 0; i < count; ++i) {
                ApplyAttackMoveJson((*strong)[i], strongCombo_[i]);
            }
        }
        return true;
    } catch (const std::exception&) {
        ResetAttackDataToDefaults();
        return false;
    }
}

bool Player::SaveAttackData(const std::filesystem::path& path) const {
    try {
        std::filesystem::create_directories(path.parent_path());

        nlohmann::json root{};
        root["weak"] = nlohmann::json::array();
        for (const combat::AttackMove& move : weakCombo_) {
            root["weak"].push_back(AttackMoveToJson(move));
        }
        root["strong"] = nlohmann::json::array();
        for (const combat::AttackMove& move : strongCombo_) {
            root["strong"].push_back(AttackMoveToJson(move));
        }

        std::ofstream file(path);
        if (!file) {
            return false;
        }
        file << root.dump(2) << '\n';
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void Player::Reset(const DirectX::XMFLOAT3& position, float facing, float health) {
    if (weakCombo_[0].name.empty() || strongCombo_[0].name.empty()) {
        ResetAttackDataToDefaults();
    }

    transform_ = {};
    transform_.position = position;
    health_ = health;
    stunTimer_ = 0.0f;
    currentComboIndex_ = -1;
    attackTimer_ = 0.0f;
    inputBufferTimer_ = 0.0f;
    hitStopTimer_ = 0.0f;
    attackCooldownTimer_ = 0.0f;
    attackInputBuffered_ = false;
    strongAttackInputBuffered_ = false;
    strongAttackActive_ = false;
    currentAttackHit_ = false;
    resetRequested_ = false;
    lockOnHeld_ = false;
    state_ = combat::CombatState::Idle;
    SetFacing(facing);
}

void Player::UpdateInput(const Input& input, float deltaTime) {
    lockOnHeld_ = input.IsKeyPress(DIK_LSHIFT) || input.IsMousePress(1) ||
                  input.IsGamepadButtonPress(XINPUT_GAMEPAD_LEFT_SHOULDER);

    const bool attackTriggered =
        input.IsKeyTrigger(DIK_J) || input.IsMouseTrigger(0) ||
        input.IsGamepadButtonTrigger(XINPUT_GAMEPAD_X);
    const bool strongAttackTriggered =
        input.IsKeyTrigger(DIK_K) || input.IsGamepadButtonTrigger(XINPUT_GAMEPAD_Y);
    if (IsAttackCooldownActive()) {
        ClearAttackBuffer();
    } else if (strongAttackTriggered) {
        strongAttackInputBuffered_ = true;
        inputBufferTimer_ = kInputBufferSeconds;
    } else if (attackTriggered) {
        attackInputBuffered_ = true;
        inputBufferTimer_ = kInputBufferSeconds;
    } else if (inputBufferTimer_ > 0.0f) {
        inputBufferTimer_ = std::max(0.0f, inputBufferTimer_ - deltaTime);
        if (inputBufferTimer_ <= 0.0f) {
            ClearAttackBuffer();
        }
    }

    if (input.IsKeyTrigger(DIK_R)) {
        resetRequested_ = true;
    }
}

void Player::Update(float deltaTime, const Input* input, const DirectX::XMFLOAT3& cameraForward,
                    const DirectX::XMFLOAT3& cameraRight,
                    const DirectX::XMFLOAT3& enemyPosition) {
    UpdateAttackCooldown(deltaTime);

    DirectX::XMFLOAT3 inputMoveDirection{};
    const bool hasInputMoveDirection = TryCalculateCameraRelativeMoveDirection(
        input, cameraForward, cameraRight, inputMoveDirection);
    const DirectX::XMFLOAT3* attackDirection =
        hasInputMoveDirection ? &inputMoveDirection : nullptr;

    if (IsAttacking()) {
        const combat::AttackMove& attack = CurrentAttack();
        if (attackTimer_ < attack.startup) {
            if (hasInputMoveDirection) {
                SetFacingDirection(inputMoveDirection);
            } else if (lockOnHeld_) {
                SetFacingToward(enemyPosition);
            }
        }

        const float previousAttackTimer = attackTimer_;
        attackTimer_ += deltaTime;
        ApplyAttackLunge(attack, previousAttackTimer, attackTimer_, enemyPosition);

        if (attackTimer_ < attack.startup) {
            state_ = combat::CombatState::AttackStartup;
        } else if (attackTimer_ <= attack.startup + attack.active) {
            state_ = combat::CombatState::AttackActive;
        } else {
            state_ = combat::CombatState::AttackRecovery;
        }

        if (CanChainNow()) {
            AdvanceComboIfBuffered(enemyPosition, attackDirection);
            return;
        }

        if (attackTimer_ >= AttackDuration(attack)) {
            FinishAttack();
        }
        return;
    }

    if (strongAttackInputBuffered_ && !IsAttackCooldownActive()) {
        StartStrongAttack(0, enemyPosition, attackDirection);
        return;
    }

    if (attackInputBuffered_ && !IsAttackCooldownActive()) {
        StartAttack(0, enemyPosition, attackDirection);
        return;
    }

    if (hasInputMoveDirection) {
        transform_.position.x += inputMoveDirection.x * kPlayerMoveSpeed * deltaTime;
        transform_.position.z += inputMoveDirection.z * kPlayerMoveSpeed * deltaTime;
        state_ = combat::CombatState::Move;
        if (lockOnHeld_) {
            SetFacingToward(enemyPosition);
        } else {
            SetFacingDirection(inputMoveDirection);
        }
    } else {
        state_ = combat::CombatState::Idle;
        if (lockOnHeld_) {
            SetFacingToward(enemyPosition);
        }
    }

    ClampToStage();
}

void Player::NotifyHitStopConsumed(float deltaTime) {
    hitStopTimer_ = std::max(0.0f, hitStopTimer_ - deltaTime);
}

void Player::ApplyHitStop(float seconds) {
    hitStopTimer_ = std::max(hitStopTimer_, seconds);
}

bool Player::ShouldResetRequested() const {
    return resetRequested_;
}

void Player::ClearResetRequest() {
    resetRequested_ = false;
}

bool Player::HasBufferedAttack() const {
    return attackInputBuffered_;
}

bool Player::HasBufferedStrongAttack() const {
    return strongAttackInputBuffered_;
}

bool Player::IsAttackActive() const {
    if (currentComboIndex_ < 0) {
        return false;
    }

    const combat::AttackMove& attack = CurrentAttack();
    return attackTimer_ >= attack.startup && attackTimer_ <= attack.startup + attack.active;
}

bool Player::HasCurrentAttackHit() const {
    return currentAttackHit_;
}

void Player::MarkCurrentAttackHit() {
    currentAttackHit_ = true;
}

bool Player::IsAttackCooldownActive() const {
    return attackCooldownTimer_ > 0.0f;
}

bool Player::IsLockOnHeld() const {
    return lockOnHeld_;
}

int Player::GetComboIndex() const {
    return currentComboIndex_;
}

bool Player::IsStrongAttackActive() const {
    return strongAttackActive_;
}

float Player::GetHitStopTimer() const {
    return hitStopTimer_;
}

const combat::AttackMove& Player::CurrentAttack() const {
    if (strongAttackActive_) {
        const int index = std::clamp(currentComboIndex_, 0, kMaxStrongCombo - 1);
        return strongCombo_[index];
    }

    const int index = std::clamp(currentComboIndex_, 0, kMaxWeakCombo - 1);
    return weakCombo_[index];
}

int Player::GetWeakAttackCount() const {
    return kMaxWeakCombo;
}

int Player::GetStrongAttackCount() const {
    return kMaxStrongCombo;
}

combat::AttackMove& Player::GetWeakAttack(int index) {
    return weakCombo_[std::clamp(index, 0, kMaxWeakCombo - 1)];
}

combat::AttackMove& Player::GetStrongAttack(int index) {
    return strongCombo_[std::clamp(index, 0, kMaxStrongCombo - 1)];
}

CollisionManager::Shape Player::MakeAttackShape(Transform& outDebugTransform) const {
    const combat::AttackMove& attack = CurrentAttack();
    const DirectX::XMFLOAT3 forward = GetFacingDirection();
    const DirectX::XMFLOAT3 right{-forward.z, 0.0f, forward.x};

    OBB box{};
    box.center = transform_.position;
    box.center.x += forward.x * attack.hitBoxOffset.x + right.x * attack.hitBoxOffset.z;
    box.center.y += attack.hitBoxOffset.y;
    box.center.z += forward.z * attack.hitBoxOffset.x + right.z * attack.hitBoxOffset.z;
    box.size = attack.hitBoxSize;
    box.rotation = transform_.rotation;

    outDebugTransform.position = box.center;
    outDebugTransform.rotation = box.rotation;
    outDebugTransform.scale = box.size;
    return CollisionManager::Shape::FromOBB(box);
}

void Player::StartAttack(int comboIndex, const DirectX::XMFLOAT3& enemyPosition,
                         const DirectX::XMFLOAT3* attackDirection) {
    currentComboIndex_ = std::clamp(comboIndex, 0, kMaxWeakCombo - 1);
    attackTimer_ = 0.0f;
    currentAttackHit_ = false;
    strongAttackActive_ = false;
    ClearAttackBuffer();
    state_ = combat::CombatState::AttackStartup;
    if (attackDirection) {
        SetFacingDirection(*attackDirection);
    } else if (lockOnHeld_) {
        SetFacingToward(enemyPosition);
    }
}

void Player::StartStrongAttack(int comboIndex, const DirectX::XMFLOAT3& enemyPosition,
                               const DirectX::XMFLOAT3* attackDirection) {
    currentComboIndex_ = std::clamp(comboIndex, 0, kMaxStrongCombo - 1);
    attackTimer_ = 0.0f;
    currentAttackHit_ = false;
    strongAttackActive_ = true;
    ClearAttackBuffer();
    state_ = combat::CombatState::AttackStartup;
    if (attackDirection) {
        SetFacingDirection(*attackDirection);
    } else {
        SetFacingToward(enemyPosition);
    }
}

void Player::AdvanceComboIfBuffered(const DirectX::XMFLOAT3& enemyPosition,
                                    const DirectX::XMFLOAT3* attackDirection) {
    if (!attackInputBuffered_) {
        if (strongAttackActive_ && strongAttackInputBuffered_) {
            if (currentComboIndex_ + 1 < kMaxStrongCombo) {
                StartStrongAttack(currentComboIndex_ + 1, enemyPosition, attackDirection);
            } else {
                ClearAttackBuffer();
            }
        }
        return;
    }

    if (strongAttackActive_) {
        return;
    }

    if (currentComboIndex_ + 1 >= kMaxWeakCombo) {
        attackInputBuffered_ = false;
        inputBufferTimer_ = 0.0f;
        return;
    }

    StartAttack(currentComboIndex_ + 1, enemyPosition, attackDirection);
}

void Player::FinishAttack() {
    const bool finishedCombo =
        strongAttackActive_ ? currentComboIndex_ + 1 >= kMaxStrongCombo
                            : currentComboIndex_ + 1 >= kMaxWeakCombo;
    currentComboIndex_ = -1;
    attackTimer_ = 0.0f;
    strongAttackActive_ = false;
    currentAttackHit_ = false;
    ClearAttackBuffer();
    if (finishedCombo) {
        attackCooldownTimer_ = kPostComboCooldownSeconds;
    }
    state_ = combat::CombatState::Idle;
}

void Player::UpdateAttackCooldown(float deltaTime) {
    if (attackCooldownTimer_ <= 0.0f) {
        return;
    }

    attackCooldownTimer_ = std::max(0.0f, attackCooldownTimer_ - deltaTime);
}

void Player::ClearAttackBuffer() {
    attackInputBuffered_ = false;
    strongAttackInputBuffered_ = false;
    inputBufferTimer_ = 0.0f;
}

bool Player::TryCalculateCameraRelativeMoveDirection(
    const Input* input, const DirectX::XMFLOAT3& cameraForward,
    const DirectX::XMFLOAT3& cameraRight, DirectX::XMFLOAT3& outMoveDirection) const {
    outMoveDirection = {};
    if (!input) {
        return false;
    }

    float moveX = 0.0f;
    float moveZ = 0.0f;
    if (input->IsKeyPress(DIK_A)) {
        moveX -= 1.0f;
    }
    if (input->IsKeyPress(DIK_D)) {
        moveX += 1.0f;
    }
    if (input->IsKeyPress(DIK_W)) {
        moveZ += 1.0f;
    }
    if (input->IsKeyPress(DIK_S)) {
        moveZ -= 1.0f;
    }
    moveX += input->GetGamepadLeftStickX();
    moveZ += input->GetGamepadLeftStickY();

    const float inputLength = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (inputLength <= 0.05f) {
        return false;
    }

    moveX /= std::max(inputLength, 1.0f);
    moveZ /= std::max(inputLength, 1.0f);
    const DirectX::XMFLOAT3 moveDirection{
        cameraRight.x * moveX + cameraForward.x * moveZ,
        0.0f,
        cameraRight.z * moveX + cameraForward.z * moveZ,
    };
    const float moveDirectionLength =
        std::sqrt(moveDirection.x * moveDirection.x + moveDirection.z * moveDirection.z);
    if (moveDirectionLength <= 0.0001f) {
        return false;
    }

    outMoveDirection = {
        moveDirection.x / std::max(moveDirectionLength, 1.0f),
        0.0f,
        moveDirection.z / std::max(moveDirectionLength, 1.0f),
    };
    return true;
}

bool Player::IsAttacking() const {
    return state_ == combat::CombatState::AttackStartup ||
           state_ == combat::CombatState::AttackActive ||
           state_ == combat::CombatState::AttackRecovery;
}

bool Player::CanChainNow() const {
    if (currentComboIndex_ < 0) {
        return false;
    }

    if (strongAttackActive_ && currentComboIndex_ + 1 >= kMaxStrongCombo) {
        return false;
    }
    if (!strongAttackActive_ && currentComboIndex_ + 1 >= kMaxWeakCombo) {
        return false;
    }

    const combat::AttackMove& attack = CurrentAttack();
    return attackTimer_ >= attack.chainStart && attackTimer_ <= attack.chainEnd;
}

void Player::ApplyAttackLunge(const combat::AttackMove& move, float previousTimer,
                              float currentTimer, const DirectX::XMFLOAT3& enemyPosition) {
    const float previousProgress = CalculateLungeProgress(move, previousTimer);
    const float currentProgress = CalculateLungeProgress(move, currentTimer);
    float lungeDelta = (currentProgress - previousProgress) * move.lungeDistance;
    if (lungeDelta <= 0.0f) {
        return;
    }

    const DirectX::XMFLOAT3 lungeDirection = CalculateLungeDirection(move);
    const float lungeForwardAmount =
        lungeDirection.x * GetFacingDirection().x + lungeDirection.z * GetFacingDirection().z;
    if (lungeDirection.x == 0.0f && lungeDirection.z == 0.0f) {
        return;
    }

    const DirectX::XMFLOAT3 forward = GetFacingDirection();
    const float enemyOffsetX = enemyPosition.x - transform_.position.x;
    const float enemyOffsetZ = enemyPosition.z - transform_.position.z;
    const float enemyDistanceAlongForward =
        enemyOffsetX * forward.x + enemyOffsetZ * forward.z;
    if (enemyDistanceAlongForward > 0.0f && lungeForwardAmount > 0.0f) {
        const float allowedDistance =
            std::max(0.0f, enemyDistanceAlongForward - kAttackLungeMinEnemySpacing);
        lungeDelta = std::min(lungeDelta, allowedDistance / lungeForwardAmount);
    }

    transform_.position.x += lungeDirection.x * lungeDelta;
    transform_.position.z += lungeDirection.z * lungeDelta;
    ClampToStage();
}

float Player::CalculateLungeProgress(const combat::AttackMove& move, float timer) const {
    if (move.lungeDistance <= 0.0f || move.lungeEnd <= move.lungeStart) {
        return 0.0f;
    }

    if (timer <= move.lungeStart) {
        return 0.0f;
    }
    if (timer >= move.lungeEnd) {
        return 1.0f;
    }

    return (timer - move.lungeStart) / (move.lungeEnd - move.lungeStart);
}

DirectX::XMFLOAT3 Player::CalculateLungeDirection(const combat::AttackMove& move) const {
    const DirectX::XMFLOAT3 forward = GetFacingDirection();
    const DirectX::XMFLOAT3 right{-forward.z, 0.0f, forward.x};
    DirectX::XMFLOAT3 direction{
        forward.x * move.lungeDirection.x + right.x * move.lungeDirection.z,
        0.0f,
        forward.z * move.lungeDirection.x + right.z * move.lungeDirection.z,
    };

    const float length = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    if (length <= 0.0001f) {
        return {};
    }

    direction.x /= length;
    direction.z /= length;
    return direction;
}

float Player::AttackDuration(const combat::AttackMove& move) const {
    return move.startup + move.active + move.recovery;
}

void Player::ClampToStage() {
    transform_.position.x =
        std::clamp(transform_.position.x, -kStageHalfWidth + kPlayerHalfWidth,
                   kStageHalfWidth - kPlayerHalfWidth);
    transform_.position.z =
        std::clamp(transform_.position.z, -kStageHalfDepth + kPlayerHalfDepth,
                   kStageHalfDepth - kPlayerHalfDepth);
}

} // namespace character
