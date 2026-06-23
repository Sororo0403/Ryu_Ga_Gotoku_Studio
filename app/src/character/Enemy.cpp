#include "character/Enemy.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kEnemyFriction = 7.5f;
constexpr float kEnemyHitStun = 0.32f;

float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

DirectX::XMFLOAT3 NormalizeForward(const DirectX::XMFLOAT3& forward, float fallbackFacing) {
    const float length = std::sqrt(forward.x * forward.x + forward.z * forward.z);
    if (length <= 0.0001f) {
        return {combat::SignOrOne(fallbackFacing), 0.0f, 0.0f};
    }
    return {forward.x / length, 0.0f, forward.z / length};
}
} // namespace

namespace character {

void Enemy::Reset(const DirectX::XMFLOAT3& position, float facing, float health) {
    transform_ = {};
    transform_.position = position;
    health_ = health;
    stunTimer_ = 0.0f;
    state_ = combat::CombatState::Idle;
    SetFacing(facing);
}

void Enemy::Update(float deltaTime, const DirectX::XMFLOAT3& playerPosition) {
    if (health_ <= 0.0f) {
        health_ = 0.0f;
        state_ = combat::CombatState::HitStun;
        return;
    }

    if (stunTimer_ > 0.0f) {
        stunTimer_ = std::max(0.0f, stunTimer_ - deltaTime);
        state_ = combat::CombatState::HitStun;
    } else {
        state_ = combat::CombatState::Idle;
    }

    SetFacingToward(playerPosition);

    const float drift = std::clamp(stunTimer_, 0.0f, 1.0f) * facing_ * -0.08f;
    transform_.position.x += drift;
    transform_.position.x += (1.35f - transform_.position.x) * Clamp01(kEnemyFriction * deltaTime) *
                             0.12f;
    transform_.position.z +=
        (0.0f - transform_.position.z) * Clamp01(kEnemyFriction * deltaTime) * 0.35f;
}

void Enemy::ApplyHit(const combat::AttackMove& attack, const DirectX::XMFLOAT3& attackerPosition,
                     const DirectX::XMFLOAT3& attackerForward) {
    health_ = std::max(0.0f, health_ - attack.damage);
    stunTimer_ = kEnemyHitStun;
    state_ = combat::CombatState::HitStun;

    const DirectX::XMFLOAT3 forward = NormalizeForward(attackerForward, -facing_);
    if (attack.reaction == combat::HitReaction::Stick) {
        const float targetX = attackerPosition.x + forward.x * attack.pullDistance;
        const float targetZ = attackerPosition.z + forward.z * attack.pullDistance;
        transform_.position.x += (targetX - transform_.position.x) * attack.pullStrength;
        transform_.position.z += (targetZ - transform_.position.z) * attack.pullStrength;
        return;
    }

    transform_.position.x += forward.x * attack.knockback;
    transform_.position.z += forward.z * attack.knockback;
}

} // namespace character
