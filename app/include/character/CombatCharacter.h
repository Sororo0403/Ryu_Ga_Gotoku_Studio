#pragma once

#include "collision/CollisionManager.h"
#include "combat/CombatTypes.h"
#include "model/Transform.h"

namespace character {

/// <summary>
/// 戦闘キャラクターに共通する位置、向き、体力、被弾判定を管理する基底クラス。
/// </summary>
class CombatCharacter {
public:
    /// <summary>
    /// 派生クラスを基底ポインタ経由で破棄できるようにする。
    /// </summary>
    virtual ~CombatCharacter() = default;

    /// <summary>
    /// 描画や判定に使う Transform を取得する。
    /// </summary>
    /// <returns>読み取り専用の Transform。</returns>
    [[nodiscard]] const Transform& GetTransform() const;

    /// <summary>
    /// 描画や判定に使う Transform を取得する。
    /// </summary>
    /// <returns>更新可能な Transform。</returns>
    [[nodiscard]] Transform& GetTransform();

    /// <summary>
    /// 現在の戦闘状態を取得する。
    /// </summary>
    /// <returns>現在の戦闘状態。</returns>
    [[nodiscard]] combat::CombatState GetState() const;

    /// <summary>
    /// 現在の体力を取得する。
    /// </summary>
    /// <returns>現在の体力。</returns>
    [[nodiscard]] float GetHealth() const;

    /// <summary>
    /// 現在向いている X 方向を取得する。
    /// </summary>
    /// <returns>右向きなら 1、左向きなら -1。</returns>
    [[nodiscard]] float GetFacing() const;

    /// <summary>
    /// 現在向いている XZ 平面上の前方向を取得する。
    /// </summary>
    /// <returns>正規化済みの前方向ベクトル。</returns>
    [[nodiscard]] DirectX::XMFLOAT3 GetFacingDirection() const;

    /// <summary>
    /// CollisionManager に登録済みの被弾 Body ID を取得する。
    /// </summary>
    /// <returns>被弾 Body ID。未登録の場合は CollisionManager::kInvalidBodyId。</returns>
    [[nodiscard]] CollisionManager::BodyId GetHurtBody() const;

    /// <summary>
    /// CollisionManager に登録した被弾 Body ID を保持する。
    /// </summary>
    /// <param name="bodyId">登録済みの被弾 Body ID。</param>
    void SetHurtBody(CollisionManager::BodyId bodyId);

    /// <summary>
    /// 指定位置の方向へキャラクターを向ける。
    /// </summary>
    /// <param name="targetPosition">向きの基準になる対象位置。</param>
    void SetFacingToward(const DirectX::XMFLOAT3& targetPosition);

    /// <summary>
    /// X 方向の向きを設定し、描画回転も更新する。
    /// </summary>
    /// <param name="facing">X 方向の向き。0 以上なら右向き、負なら左向き。</param>
    void SetFacing(float facing);

    /// <summary>
    /// XZ 平面上の前方向を設定し、描画回転も更新する。
    /// </summary>
    /// <param name="direction">前方向にしたいベクトル。</param>
    void SetFacingDirection(const DirectX::XMFLOAT3& direction);

    /// <summary>
    /// 現在位置と向きから被弾判定形状を作成する。
    /// </summary>
    /// <returns>被弾用 OBB を保持した CollisionManager::Shape。</returns>
    [[nodiscard]] CollisionManager::Shape MakeHurtShape() const;

protected:
    Transform transform_{};
    combat::CombatState state_ = combat::CombatState::Idle;
    float health_ = 100.0f;
    float stunTimer_ = 0.0f;
    float facing_ = 1.0f;
    DirectX::XMFLOAT3 facingDirection_{1.0f, 0.0f, 0.0f};
    CollisionManager::BodyId hurtBody_ = CollisionManager::kInvalidBodyId;
};

} // namespace character
