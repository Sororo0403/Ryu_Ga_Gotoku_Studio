#pragma once

#include <DirectXMath.h>

namespace combat {

/// <summary>
/// 戦闘キャラクターの現在状態。
/// </summary>
enum class CombatState {
    Idle,
    Move,
    AttackStartup,
    AttackActive,
    AttackRecovery,
    HitStun,
};

/// <summary>
/// 攻撃命中時に敵へ適用する位置反応。
/// </summary>
enum class HitReaction {
    Stick,
    Knockback,
};

/// <summary>
/// ラッシュコンボ 1 段分の攻撃性能。
/// </summary>
struct AttackMove {
    const char* name = "";
    float startup = 0.0f;
    float active = 0.0f;
    float recovery = 0.0f;
    float chainStart = 0.0f;
    float chainEnd = 0.0f;
    float damage = 0.0f;
    float hitStop = 0.0f;
    float knockback = 0.0f;
    float pullDistance = 0.0f;
    float pullStrength = 0.0f;
    HitReaction reaction = HitReaction::Knockback;
    DirectX::XMFLOAT3 hitBoxOffset{0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 hitBoxSize{1.0f, 1.0f, 1.0f};
    float lungeDistance = 0.0f;
    DirectX::XMFLOAT3 lungeDirection{1.0f, 0.0f, 0.0f};
    float lungeStart = 0.0f;
    float lungeEnd = 0.0f;
};

/// <summary>
/// 向きから描画用の回転クォータニオンを作成する。
/// </summary>
/// <param name="facing">X 方向の向き。0 以上なら右向き、負なら左向き。</param>
/// <returns>指定方向を向く回転クォータニオン。</returns>
[[nodiscard]] DirectX::XMFLOAT4 MakeFacingRotation(float facing);

/// <summary>
/// 符号を -1 または 1 に正規化する。
/// </summary>
/// <param name="value">判定する値。</param>
/// <returns>value が負なら -1、それ以外なら 1。</returns>
[[nodiscard]] float SignOrOne(float value);

/// <summary>
/// 戦闘状態をデバッグ表示用の文字列に変換する。
/// </summary>
/// <param name="state">変換する戦闘状態。</param>
/// <returns>状態名の文字列。</returns>
[[nodiscard]] const char* CombatStateName(CombatState state);

} // namespace combat
