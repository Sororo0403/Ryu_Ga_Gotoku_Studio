#pragma once

#include "character/CombatCharacter.h"

class Input;

namespace character {

/// <summary>
/// プレイヤーの移動、入力、ラッシュコンボ状態を管理するクラス。
/// </summary>
class Player final : public CombatCharacter {
public:
    static constexpr int kMaxRushCombo = 4;

    /// <summary>
    /// 初期位置、向き、体力を設定してプレイヤーを初期状態へ戻す。
    /// </summary>
    /// <param name="position">初期位置。</param>
    /// <param name="facing">初期の X 方向の向き。</param>
    /// <param name="health">初期体力。</param>
    void Reset(const DirectX::XMFLOAT3& position, float facing, float health);

    /// <summary>
    /// 攻撃入力、入力バッファ、リセット入力を更新する。
    /// </summary>
    /// <param name="input">現在フレームの入力状態。</param>
    /// <param name="deltaTime">前フレームからの経過秒数。</param>
    void UpdateInput(const Input& input, float deltaTime);

    /// <summary>
    /// 移動、攻撃状態、コンボ派生を更新する。
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過秒数。</param>
    /// <param name="input">移動入力に使う入力状態。入力がない場合は nullptr。</param>
    /// <param name="cameraForward">カメラが向いている水平前方向。</param>
    /// <param name="cameraRight">カメラから見た水平右方向。</param>
    /// <param name="enemyPosition">向きや攻撃開始方向の基準になる敵位置。</param>
    void Update(float deltaTime, const Input* input, const DirectX::XMFLOAT3& cameraForward,
                const DirectX::XMFLOAT3& cameraRight, const DirectX::XMFLOAT3& enemyPosition);

    /// <summary>
    /// ヒットストップ残り時間を経過時間分だけ減らす。
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過秒数。</param>
    void NotifyHitStopConsumed(float deltaTime);

    /// <summary>
    /// 攻撃命中時のヒットストップを設定する。
    /// </summary>
    /// <param name="seconds">適用するヒットストップ秒数。</param>
    void ApplyHitStop(float seconds);

    /// <summary>
    /// リセット入力が要求されたかを取得する。
    /// </summary>
    /// <returns>リセット入力があった場合は true。</returns>
    [[nodiscard]] bool ShouldResetRequested() const;

    /// <summary>
    /// リセット要求フラグを解除する。
    /// </summary>
    void ClearResetRequest();

    /// <summary>
    /// 攻撃入力がバッファ中かを取得する。
    /// </summary>
    /// <returns>攻撃入力がバッファ中なら true。</returns>
    [[nodiscard]] bool HasBufferedAttack() const;

    /// <summary>
    /// 現在の攻撃がアクティブフレーム中かを取得する。
    /// </summary>
    /// <returns>攻撃判定を出す時間内なら true。</returns>
    [[nodiscard]] bool IsAttackActive() const;

    /// <summary>
    /// 現在の攻撃がすでに命中済みかを取得する。
    /// </summary>
    /// <returns>現在の攻撃が命中済みなら true。</returns>
    [[nodiscard]] bool HasCurrentAttackHit() const;

    /// <summary>
    /// 現在の攻撃を命中済みとして記録する。
    /// </summary>
    void MarkCurrentAttackHit();

    /// <summary>
    /// 攻撃開始前のクールダウン中かを取得する。
    /// </summary>
    /// <returns>クールダウン中なら true。</returns>
    [[nodiscard]] bool IsAttackCooldownActive() const;

    /// <summary>
    /// ロックオン入力が押されているかを取得する。
    /// </summary>
    /// <returns>ロックオン中なら true。</returns>
    [[nodiscard]] bool IsLockOnHeld() const;

    /// <summary>
    /// 現在再生中のコンボ段数インデックスを取得する。
    /// </summary>
    /// <returns>コンボ中なら 0 始まりの段数。攻撃中でなければ -1。</returns>
    [[nodiscard]] int GetComboIndex() const;

    /// <summary>
    /// 残りヒットストップ時間を取得する。
    /// </summary>
    /// <returns>残りヒットストップ秒数。</returns>
    [[nodiscard]] float GetHitStopTimer() const;

    /// <summary>
    /// 現在の攻撃性能を取得する。
    /// </summary>
    /// <returns>現在のコンボ段に対応する攻撃性能。</returns>
    [[nodiscard]] const combat::AttackMove& CurrentAttack() const;

    /// <summary>
    /// 現在の攻撃判定形状を作成し、デバッグ描画用 Transform も更新する。
    /// </summary>
    /// <param name="outDebugTransform">攻撃判定ボックスの描画に使う Transform。</param>
    /// <returns>攻撃判定用 OBB を保持した CollisionManager::Shape。</returns>
    [[nodiscard]] CollisionManager::Shape MakeAttackShape(Transform& outDebugTransform) const;

private:
    /// <summary>
    /// 指定したコンボ段の攻撃を開始する。
    /// </summary>
    /// <param name="comboIndex">開始するコンボ段数インデックス。</param>
    /// <param name="enemyPosition">攻撃開始時に向く敵位置。</param>
    void StartAttack(int comboIndex, const DirectX::XMFLOAT3& enemyPosition);

    /// <summary>
    /// バッファ入力があり、派生可能なら次のコンボ段へ進める。
    /// </summary>
    /// <param name="enemyPosition">次段開始時に向く敵位置。</param>
    void AdvanceComboIfBuffered(const DirectX::XMFLOAT3& enemyPosition);

    /// <summary>
    /// 現在の攻撃を終了して待機状態へ戻す。
    /// </summary>
    void FinishAttack();

    /// <summary>
    /// 攻撃開始前のクールダウンを更新する。
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過秒数。</param>
    void UpdateAttackCooldown(float deltaTime);

    /// <summary>
    /// 入力バッファを空にする。
    /// </summary>
    void ClearAttackBuffer();

    /// <summary>
    /// 現在攻撃状態かを取得する。
    /// </summary>
    /// <returns>攻撃の開始、持続、硬直中なら true。</returns>
    [[nodiscard]] bool IsAttacking() const;

    /// <summary>
    /// 現在のタイミングで次段へ派生可能かを取得する。
    /// </summary>
    /// <returns>派生受付時間内なら true。</returns>
    [[nodiscard]] bool CanChainNow() const;

    /// <summary>
    /// 攻撃全体の再生時間を計算する。
    /// </summary>
    /// <param name="move">計算対象の攻撃性能。</param>
    /// <returns>startup、active、recovery を合計した秒数。</returns>
    [[nodiscard]] float AttackDuration(const combat::AttackMove& move) const;

    combat::AttackMove rushCombo_[kMaxRushCombo]{};
    int currentComboIndex_ = -1;
    float attackTimer_ = 0.0f;
    float inputBufferTimer_ = 0.0f;
    float hitStopTimer_ = 0.0f;
    float attackCooldownTimer_ = 0.0f;
    bool attackInputBuffered_ = false;
    bool currentAttackHit_ = false;
    bool resetRequested_ = false;
    bool lockOnHeld_ = false;
};

} // namespace character
