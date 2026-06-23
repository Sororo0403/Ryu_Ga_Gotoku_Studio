#pragma once

#include "EngineScene.h"

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
};
