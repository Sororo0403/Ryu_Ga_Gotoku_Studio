#include "scene/GameScene.h"

#include "EngineCore.h"

#include <Windows.h>
#include <memory>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    EngineRuntimeConfig config{};
    config.title = L"Ryu Ga Gotoku Studio";
    config.logPath = L"generated/logs/app.log";

    EngineRuntime runtime;
    return runtime.Run(instance, showCommand, std::make_unique<GameScene>(), config);
}
