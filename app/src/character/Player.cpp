#include "character/Player.h"

#include "input/Input.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kInputBufferSeconds = 0.18f;
constexpr float kPostComboCooldownSeconds = 0.55f;
constexpr float kPlayerMoveSpeed = 2.8f;
constexpr float kStageHalfWidth = 4.5f;
constexpr float kStageHalfDepth = 3.5f;
constexpr float kPlayerHalfWidth = 0.4f;
constexpr float kPlayerHalfDepth = 0.275f;
constexpr float kAttackLungeMinEnemySpacing = 0.58f;
} // namespace

namespace character {

void Player::Reset(const DirectX::XMFLOAT3& position, float facing, float health) {
    rushCombo_[0] = {"Rush 1", 0.14f, 0.15f, 0.28f, 0.24f, 0.48f, 8.0f, 0.06f, 0.45f,
                     0.9f, 0.72f, combat::HitReaction::Stick, {0.65f, 0.9f, 0.0f},
                     {0.78f, 0.75f, 0.92f}, 0.0f, {1.0f, 0.0f, 0.0f}, 0.0f, 0.0f};
    rushCombo_[1] = {"Rush 2", 0.13f, 0.16f, 0.30f, 0.24f, 0.50f, 9.0f, 0.065f, 0.55f,
                     0.95f, 0.74f, combat::HitReaction::Stick, {0.72f, 0.9f, 0.0f},
                     {0.82f, 0.78f, 0.92f}, 0.0f, {1.0f, 0.0f, 0.0f}, 0.0f, 0.0f};
    rushCombo_[2] = {"Rush 3", 0.16f, 0.17f, 0.34f, 0.28f, 0.56f, 11.0f, 0.07f, 0.72f,
                     1.0f, 0.78f, combat::HitReaction::Stick, {0.76f, 0.82f, 0.0f},
                     {0.9f, 0.78f, 1.0f}, 0.55f, {1.0f, 0.0f, 0.0f}, 0.05f, 0.20f};
    rushCombo_[3] = {"Rush Finish", 0.22f, 0.20f, 0.52f, 0.0f, 0.0f, 18.0f, 0.09f, 1.45f,
                     0.0f, 0.0f, combat::HitReaction::Knockback, {0.86f, 0.95f, 0.0f},
                     {1.05f, 0.95f, 1.08f}, 0.78f, {1.0f, 0.0f, 0.0f}, 0.04f, 0.25f};

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
    if (IsAttackCooldownActive()) {
        ClearAttackBuffer();
    } else if (attackTriggered) {
        attackInputBuffered_ = true;
        inputBufferTimer_ = kInputBufferSeconds;
    } else if (inputBufferTimer_ > 0.0f) {
        inputBufferTimer_ = std::max(0.0f, inputBufferTimer_ - deltaTime);
        if (inputBufferTimer_ <= 0.0f) {
            attackInputBuffered_ = false;
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

float Player::GetHitStopTimer() const {
    return hitStopTimer_;
}

const combat::AttackMove& Player::CurrentAttack() const {
    const int index = std::clamp(currentComboIndex_, 0, kMaxRushCombo - 1);
    return rushCombo_[index];
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
    currentComboIndex_ = std::clamp(comboIndex, 0, kMaxRushCombo - 1);
    attackTimer_ = 0.0f;
    currentAttackHit_ = false;
    attackInputBuffered_ = false;
    inputBufferTimer_ = 0.0f;
    state_ = combat::CombatState::AttackStartup;
    if (attackDirection) {
        SetFacingDirection(*attackDirection);
    } else if (lockOnHeld_) {
        SetFacingToward(enemyPosition);
    }
}

void Player::AdvanceComboIfBuffered(const DirectX::XMFLOAT3& enemyPosition,
                                    const DirectX::XMFLOAT3* attackDirection) {
    if (!attackInputBuffered_) {
        return;
    }

    if (currentComboIndex_ + 1 >= kMaxRushCombo) {
        attackInputBuffered_ = false;
        inputBufferTimer_ = 0.0f;
        return;
    }

    StartAttack(currentComboIndex_ + 1, enemyPosition, attackDirection);
}

void Player::FinishAttack() {
    const bool finishedCombo = currentComboIndex_ + 1 >= kMaxRushCombo;
    currentComboIndex_ = -1;
    attackTimer_ = 0.0f;
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
    if (currentComboIndex_ < 0 || currentComboIndex_ + 1 >= kMaxRushCombo) {
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
