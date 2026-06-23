#pragma once

#include "EngineScene.h"
#include "character/Enemy.h"
#include "character/Player.h"
#include "collision/CollisionManager.h"
#include "model/Transform.h"

#include <cstdint>
#include <filesystem>
#include <string>

/// <summary>
/// アプリケーションで使用するメインゲームシーン。
/// </summary>
class GameScene final : public BaseScene {
public:
    /// <summary>
    /// エンジンがシーンコンテキストを準備した後、シーンの初期化処理を行う。
    /// </summary>
    /// <param name="ctx">シーンから利用するエンジン機能やサービスをまとめたコンテキスト。</param>
    void Initialize(const SceneContext& ctx) override;

    /// <summary>
    /// 1 フレームごとのゲーム状態を更新する。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 1 フレームごとの描画処理を行う。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// ポストプロセス後の ImGui デバッグ表示を描画する。
    /// </summary>
    void DrawPostProcessOverlay() override;

private:
    /// <summary>
    /// 攻撃性能 JSON を読み込み、存在しなければ既定値で保存する。
    /// </summary>
    void InitializeAttackData();

    /// <summary>
    /// 戦闘シーン用のカメラを作成してアクティブにする。
    /// </summary>
    void InitializeCamera();

    /// <summary>
    /// 入力と戦闘状況からオービットカメラを更新する。
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過秒数。</param>
    void UpdateOrbitCamera(float deltaTime);

    /// <summary>
    /// オービットカメラの注視点を計算する。
    /// </summary>
    /// <returns>プレイヤー位置から求めた注視点。</returns>
    [[nodiscard]] DirectX::XMFLOAT3 CalculateCameraTarget() const;

    /// <summary>
    /// プレイヤー移動に使うカメラ基準の水平前方向を計算する。
    /// </summary>
    /// <returns>カメラが向いている水平前方向。</returns>
    [[nodiscard]] DirectX::XMFLOAT3 CalculateCameraForward() const;

    /// <summary>
    /// プレイヤー移動に使うカメラ基準の水平右方向を計算する。
    /// </summary>
    /// <returns>カメラから見た水平右方向。</returns>
    [[nodiscard]] DirectX::XMFLOAT3 CalculateCameraRight() const;

    /// <summary>
    /// プレイヤー、敵、床、攻撃判定デバッグ用のモデルを作成する。
    /// </summary>
    void InitializeModels();

    /// <summary>
    /// プレイヤー、敵、衝突 Body を初期状態へ戻す。
    /// </summary>
    void ResetCombat();

    /// <summary>
    /// 敵撃破後の再生成タイマーを更新する。
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過秒数。</param>
    /// <returns>再生成待ち中なら true。</returns>
    bool UpdateEnemyRespawn(float deltaTime);

    /// <summary>
    /// 敵を初期スポーン位置へ再生成する。
    /// </summary>
    void RespawnEnemy();

    /// <summary>
    /// 敵の再生成待ちを開始する。
    /// </summary>
    void StartEnemyRespawn();

    /// <summary>
    /// プレイヤーと敵の被弾 Body 形状を現在位置に同期する。
    /// </summary>
    void UpdateCollisionBodies();

    /// <summary>
    /// プレイヤー攻撃判定と敵被弾判定の衝突を解決する。
    /// </summary>
    void ResolveAttackHit();

    /// <summary>
    /// 戦闘キャラクターのモデルを描画する。
    /// </summary>
    /// <param name="fighter">描画対象の戦闘キャラクター。</param>
    /// <param name="modelId">描画に使うモデル ID。</param>
    void DrawFighter(const character::CombatCharacter& fighter, uint32_t modelId) const;

    /// <summary>
    /// 攻撃アクティブ中の判定ボックスをデバッグ描画する。
    /// </summary>
    void DrawAttackDebug() const;

    /// <summary>
    /// 戦闘状態と攻撃性能調整用の ImGui UI を描画する。
    /// </summary>
    void DrawDebugUi();

    /// <summary>
    /// 攻撃性能 1 段分の ImGui 編集 UI を描画する。
    /// </summary>
    /// <param name="move">編集対象の攻撃性能。</param>
    /// <param name="label">ImGui 表示用ラベル。</param>
    void DrawAttackMoveEditor(combat::AttackMove& move, const char* label);

private:
    character::Player player_{};
    character::Enemy enemy_{};
    CollisionManager collision_{};

    uint32_t playerModelId_ = 0;
    uint32_t enemyModelId_ = 0;
    uint32_t floorModelId_ = 0;
    uint32_t hitBoxModelId_ = 0;

    Transform floorTransform_{};
    Transform hitBoxTransform_{};

    float cameraYaw_ = 0.0f;
    float cameraPitch_ = 0.55f;
    float cameraDistance_ = 9.5f;

    float enemyRespawnTimer_ = 0.0f;
    bool enemyRespawnPending_ = false;

    std::filesystem::path attackDataPath_{};
    std::string attackDataStatus_{};
    bool debugUiMode_ = false;
};
