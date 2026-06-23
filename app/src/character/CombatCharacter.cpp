#include "character/CombatCharacter.h"

#include <cmath>

namespace character {
namespace {
constexpr float kHurtBoxHeight = 1.78f;
constexpr float kFacingEpsilon = 0.0001f;

DirectX::XMFLOAT4 MakeRotationFromForward(const DirectX::XMFLOAT3& forward) {
    const float yaw = std::atan2(-forward.z, forward.x);
    const float halfYaw = yaw * 0.5f;
    return {0.0f, std::sin(halfYaw), 0.0f, std::cos(halfYaw)};
}
} // namespace

const Transform& CombatCharacter::GetTransform() const {
    return transform_;
}

Transform& CombatCharacter::GetTransform() {
    return transform_;
}

combat::CombatState CombatCharacter::GetState() const {
    return state_;
}

float CombatCharacter::GetHealth() const {
    return health_;
}

float CombatCharacter::GetFacing() const {
    return facing_;
}

DirectX::XMFLOAT3 CombatCharacter::GetFacingDirection() const {
    return facingDirection_;
}

CollisionManager::BodyId CombatCharacter::GetHurtBody() const {
    return hurtBody_;
}

void CombatCharacter::SetHurtBody(CollisionManager::BodyId bodyId) {
    hurtBody_ = bodyId;
}

void CombatCharacter::SetFacingToward(const DirectX::XMFLOAT3& targetPosition) {
    SetFacingDirection({targetPosition.x - transform_.position.x, 0.0f,
                        targetPosition.z - transform_.position.z});
}

void CombatCharacter::SetFacing(float facing) {
    facing_ = combat::SignOrOne(facing);
    facingDirection_ = {facing_, 0.0f, 0.0f};
    transform_.rotation = combat::MakeFacingRotation(facing_);
}

void CombatCharacter::SetFacingDirection(const DirectX::XMFLOAT3& direction) {
    const float length = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    if (length <= kFacingEpsilon) {
        SetFacing(facing_);
        return;
    }

    facingDirection_ = {direction.x / length, 0.0f, direction.z / length};
    if (std::fabs(facingDirection_.x) > kFacingEpsilon) {
        facing_ = combat::SignOrOne(facingDirection_.x);
    }
    transform_.rotation = MakeRotationFromForward(facingDirection_);
}

CollisionManager::Shape CombatCharacter::MakeHurtShape() const {
    OBB box{};
    box.center = transform_.position;
    box.center.y = transform_.position.y + kHurtBoxHeight * 0.5f;
    box.size = {0.82f, kHurtBoxHeight, 0.62f};
    box.rotation = transform_.rotation;
    return CollisionManager::Shape::FromOBB(box);
}

} // namespace character
