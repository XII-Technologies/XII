#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#include <EditorEngineProcess/EngineProcGameAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <GameEngine/XR/XRInputDevice.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

xiiEngineProcessGameApplicationUWP::xiiEngineProcessGameApplicationUWP() {}

xiiEngineProcessGameApplicationUWP::~xiiEngineProcessGameApplicationUWP() {}

xiiUniquePtr<xiiEditorEngineProcessApp> xiiEngineProcessGameApplicationUWP::CreateEngineProcessApp()
{
  xiiUniquePtr<xiiEditorEngineProcessApp> ptr = XII_DEFAULT_NEW(xiiEditorEngineProcessAppUWP);
  m_pEngineProcessApp                         = static_cast<xiiEditorEngineProcessAppUWP*>(ptr.Borrow());
  return ptr;
}

void xiiEngineProcessGameApplicationUWP::Init_ConfigureInput()
{
  xiiEngineProcessGameApplication::Init_ConfigureInput();

  // Set Anchor
  {
    xiiInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_XR_Hand_Left_Trigger;
    cfg.m_bApplyTimeScaling    = false;
    xiiInputManager::SetInputActionConfig("RemoteProcess", "AirTap", cfg, true);
  }
}

bool xiiEngineProcessGameApplicationUWP::Run_ProcessApplicationInput()
{
  return SUPER::Run_ProcessApplicationInput();
}

#endif
