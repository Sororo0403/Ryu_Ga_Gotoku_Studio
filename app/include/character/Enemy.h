#pragma once

#include "character/CombatCharacter.h"

namespace character {

/// <summary>
/// ラッシュコンボの相手になる敵キャラクター。
/// </summary>
class Enemy final : public CombatCharacter {
public:
    /// <summary>
    /// 初期位置、向き、体力を設定して敵を初期状態へ戻す。
    /// </summary>
    /// <param name="position">初期位置。</param>
    /// <param name="facing">初期の X 方向の向き。</param>
    /// <param name="health">初期体力。</param>
    void Reset(const DirectX::XMFLOAT3& position, float facing, float health);

    /// <summary>
    /// 敵のスタン時間、向き、復帰位置を更新する。
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過秒数。</param>
    /// <param name="playerPosition">プレイヤーの現在位置。</param>
    void Update(float deltaTime, const DirectX::XMFLOAT3& playerPosition);

    /// <summary>
    /// 攻撃命中時のダメージ、スタン、位置反応を適用する。
    /// </summary>
    /// <param name="attack">命中した攻撃性能。</param>
    /// <param name="attackerPosition">攻撃したキャラクターの位置。</param>
    /// <param name="attackerForward">攻撃したキャラクターの XZ 平面上の前方向。</param>
    void ApplyHit(const combat::AttackMove& attack, const DirectX::XMFLOAT3& attackerPosition,
                  const DirectX::XMFLOAT3& attackerForward);
};

} // namespace character
