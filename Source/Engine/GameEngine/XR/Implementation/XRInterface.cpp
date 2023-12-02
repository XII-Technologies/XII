#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRInterface.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiXRStageSpace, 1)
  XII_BITFLAGS_CONSTANTS(xiiXRStageSpace::Seated, xiiXRStageSpace::Standing)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInterface);
