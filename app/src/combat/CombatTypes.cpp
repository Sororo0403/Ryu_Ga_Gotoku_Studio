#include "combat/CombatTypes.h"

namespace combat {

DirectX::XMFLOAT4 MakeFacingRotation(float facing) {
    if (facing >= 0.0f) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    return {0.0f, 1.0f, 0.0f, 0.0f};
}

float SignOrOne(float value) {
    return value < 0.0f ? -1.0f : 1.0f;
}

const char* CombatStateName(CombatState state) {
    switch (state) {
    case CombatState::Idle:
        return "Idle";
    case CombatState::Move:
        return "Move";
    case CombatState::AttackStartup:
        return "AttackStartup";
    case CombatState::AttackActive:
        return "AttackActive";
    case CombatState::AttackRecovery:
        return "AttackRecovery";
    case CombatState::HitStun:
        return "HitStun";
    default:
        return "Unknown";
    }
}

} // namespace combat
