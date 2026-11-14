/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngineTest/GameEngineTestPCH.h>

#include <Core/Input/InputDevice.h>
#include <Core/Input/InputManager.h>
#include <Core/World/World.h>
#include <GameEngine/GameState/FallbackGameState.h>

namespace
{
  class FallbackInputDevice final : public xiiInputDevice
  {
    void InitializeDevice() override {}
    void UpdateInputSlotValues() override {}
    void RegisterInputSlots() override
    {
      RegisterInputSlot(xiiInputSlot_MouseMovePosX, "Mouse Move Right", xiiInputSlotFlags::IsMouseAxisMove);
      RegisterInputSlot(xiiInputSlot_MouseMoveNegY, "Mouse Move Up", xiiInputSlotFlags::IsMouseAxisMove);
      RegisterInputSlot(xiiInputSlot_KeyRight, "Right", xiiInputSlotFlags::IsButton);
      RegisterInputSlot(xiiInputSlot_KeyLeftWin, "Left Win", xiiInputSlotFlags::IsButton);
      RegisterInputSlot(xiiInputSlot_KeyRightWin, "Right Win", xiiInputSlotFlags::IsButton);
    }
  };

  class FallbackInputState final : public xiiFallbackGameState
  {
  public:
    explicit FallbackInputState(xiiWorld& world)
    {
      m_pMainWorld = &world;
      ConfigureInputActions();
    }

    void ResetCamera()
    {
      m_MainCamera.LookAt(xiiVec3::MakeZero(), xiiVec3::MakeAxisX(), xiiVec3::MakeAxisZ());
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(GameState);

XII_CREATE_SIMPLE_TEST(GameState, FallbackMouseLook)
{
  xiiWorldDescription description("Fallback mouse input");
  xiiWorld            world(description);
  FallbackInputState  state(world);
  FallbackInputDevice device;
  xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

  for (double fFrameTime : {1.0 / 30.0, 1.0 / 120.0})
  {
    const auto step = xiiTime::MakeFromSeconds(fFrameTime);
    state.ResetCamera();
    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      xiiInputManager::InjectInputSlotValue(xiiInputSlot_MouseMovePosX, 10.0f);
      xiiInputManager::Update(step);
      state.ProcessInput();
      const auto angle = xiiAngle::MakeFromDegree((i + 1) * 3.6f);
      XII_TEST_VEC3(state.GetMainCamera()->GetCenterDirForwards(), xiiVec3(xiiMath::Cos(angle), xiiMath::Sin(angle), 0), 0.0001f);
    }
    const auto direction = state.GetMainCamera()->GetCenterDirForwards();
    xiiInputManager::Update(step);
    state.ProcessInput();
    XII_TEST_VEC3(state.GetMainCamera()->GetCenterDirForwards(), direction, 0.0001f);

    state.ResetCamera();
    xiiInputManager::InjectInputSlotValue(xiiInputSlot_MouseMoveNegY, 10.0f);
    xiiInputManager::Update(step);
    state.ProcessInput();
    const auto pitch = xiiAngle::MakeFromDegree(3.6f);
    XII_TEST_VEC3(state.GetMainCamera()->GetCenterDirForwards(), xiiVec3(xiiMath::Cos(pitch), 0, xiiMath::Sin(pitch)), 0.0001f);

    state.ResetCamera();
    xiiInputManager::InjectInputSlotValue(xiiInputSlot_KeyRight, 1.0f);
    xiiInputManager::Update(step);
    state.ProcessInput();
    const auto yaw = xiiAngle::MakeFromDegree(180.0f * static_cast<float>(fFrameTime));
    XII_TEST_VEC3(state.GetMainCamera()->GetCenterDirForwards(), xiiVec3(xiiMath::Cos(yaw), xiiMath::Sin(yaw), 0), 0.0001f);
    xiiInputManager::Update(step);
    xiiInputManager::Update(step);
  }
}
