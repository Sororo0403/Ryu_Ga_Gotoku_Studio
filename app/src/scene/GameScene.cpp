#include "scene/GameScene.h"

void GameScene::Initialize(const SceneContext& ctx) {
    BaseScene::Initialize(ctx);
    if (ctx_->systems.log) {
        ctx_->systems.log("GameScene initialized");
    }
}

void GameScene::Update() {}

void GameScene::Draw() {}
