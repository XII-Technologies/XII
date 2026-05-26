/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <DearImguiPlugin/DearImguiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <DearImguiPlugin/DearImguiSingleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Clock.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GraphicsCore/Pipeline/View.h>

#include <Imgui/imgui_internal.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(DearImguiPlugin, DearImgui)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiImguiSingleton);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiImguiSingleton* pSingleton = xiiImguiSingleton::GetSingleton();

    XII_DEFAULT_DELETE(pSingleton);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  void* xiiImguiAllocate(size_t uiSize, void* pUserData)
  {
    xiiAllocator* pAllocator = static_cast<xiiAllocator*>(pUserData);
    return pAllocator->Allocate(uiSize, XII_ALIGNMENT_MINIMUM);
  }

  void xiiImguiDeallocate(void* pPtr, void* pUserData)
  {
    if (pPtr != nullptr)
    {
      xiiAllocator* pAllocator = static_cast<xiiAllocator*>(pUserData);
      pAllocator->Deallocate(pPtr);
    }
  }
} // namespace

XII_IMPLEMENT_SINGLETON(xiiImguiSingleton);

xiiImguiSingleton::xiiImguiSingleton() :
  m_SingletonRegistrar(this), m_Allocator("ImGui", xiiFoundation::GetDefaultAllocator())
{
  Startup();
}

xiiImguiSingleton::~xiiImguiSingleton()
{
  Shutdown();
}

void xiiImguiSingleton::Startup()
{
  ImGui::SetAllocatorFunctions(&xiiImguiAllocate, &xiiImguiDeallocate, &m_Allocator);

  m_pSharedFontAtlas = XII_DEFAULT_NEW(ImFontAtlas);

  unsigned char* pPixels;
  xiiInt32       iWidth, iHeight;
  m_pSharedFontAtlas->GetTexDataAsRGBA32(&pPixels, &iWidth, &iHeight); // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders.
                                                                       // If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

  xiiTexture2DResourceHandle hFont = xiiResourceManager::GetExistingResource<xiiTexture2DResource>("ImguiFont");

  if (!hFont.IsValid())
  {
    xiiGALTextureSubResourceData memoryDesc;
    memoryDesc.m_pData         = xiiMakeByteBlobPtr(pPixels, xiiUInt64(iWidth) * iHeight * 4ull);
    memoryDesc.m_uiStride      = iWidth * 4;
    memoryDesc.m_uiDepthStride = iWidth * iHeight * 4;

    xiiTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_Type        = xiiGALResourceDimension::Texture2D;
    desc.m_DescGAL.m_Size.width  = iWidth;
    desc.m_DescGAL.m_Size.height = iHeight;
    desc.m_DescGAL.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
    desc.m_DescGAL.m_Usage       = xiiGALResourceUsage::Immutable;
    desc.m_InitialContent        = xiiMakeArrayPtr(&memoryDesc, 1);

    hFont = xiiResourceManager::GetOrCreateResource<xiiTexture2DResource>("ImguiFont", std::move(desc));
  }

  m_Textures.PushBack(hFont);

  const size_t id           = (size_t)m_Textures.GetCount() - 1;
  m_pSharedFontAtlas->TexID = reinterpret_cast<void*>(id);

  xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(xiiMakeDelegate(&xiiImguiSingleton ::GameApplicationEventHandler, this));
}

void xiiImguiSingleton::Shutdown()
{
  xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(xiiMakeDelegate(&xiiImguiSingleton ::GameApplicationEventHandler, this));

  m_Textures.Clear();

  m_pSharedFontAtlas = nullptr;
}

ImGuiContext* xiiImguiSingleton::CreateContext()
{
  // ImGui reads the global context pointer WHILE creating a new context so if we don't reset it to null here, it will try to access it, and crash if imgui was active on the same thread before.
  ImGui::SetCurrentContext(nullptr);
  ImGuiContext* context = ImGui::CreateContext(m_pSharedFontAtlas.Borrow());
  ImGui::SetCurrentContext(context);

  ImGuiIO& cfg = ImGui::GetIO();

  cfg.DisplaySize.x = 960;
  cfg.DisplaySize.y = 540;

  return context;
}

void xiiImguiSingleton::BeginFrame(const xiiView* pView)
{
  if (!pView)
    return;

  auto viewport             = pView->GetViewport();
  m_CurrentWindowResolution = xiiSizeU32(static_cast<xiiUInt32>(viewport.width), static_cast<xiiUInt32>(viewport.height));

  ImGuiIO& cfg = ImGui::GetIO();

  cfg.DisplaySize.x = viewport.width;
  cfg.DisplaySize.y = viewport.height;
  cfg.DeltaTime     = (float)xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds();

  if (m_bPassInputToImgui)
  {
    char  szUtf8[8] = "";
    char* pChar     = szUtf8;
    xiiUnicodeUtils::EncodeUtf32ToUtf8(xiiInputManager::RetrieveLastCharacter(false), pChar);
    cfg.AddInputCharactersUTF8(szUtf8);

    float fMouseX, fMouseY;
    if (xiiInputManager::GetInputSlotState(xiiInputSlot_TouchPoint0) != xiiKeyState::Up)
    {
      xiiInputManager::GetInputSlotState(xiiInputSlot_TouchPoint0_PositionX, &fMouseX);
      xiiInputManager::GetInputSlotState(xiiInputSlot_TouchPoint0_PositionY, &fMouseY);
      cfg.AddMousePosEvent(cfg.DisplaySize.x * fMouseX, cfg.DisplaySize.y * fMouseY);
      cfg.AddMouseButtonEvent(0, xiiInputManager::GetInputSlotState(xiiInputSlot_TouchPoint0) >= xiiKeyState::Pressed);
      cfg.AddMouseButtonEvent(1, false);
      cfg.AddMouseButtonEvent(2, false);
    }
    else
    {
      xiiInputManager::GetInputSlotState(xiiInputSlot_MousePositionX, &fMouseX);
      xiiInputManager::GetInputSlotState(xiiInputSlot_MousePositionY, &fMouseY);
      cfg.AddMousePosEvent(cfg.DisplaySize.x * fMouseX, cfg.DisplaySize.y * fMouseY);
      cfg.AddMouseButtonEvent(0, xiiInputManager::GetInputSlotState(xiiInputSlot_MouseButton0) >= xiiKeyState::Pressed);
      cfg.AddMouseButtonEvent(1, xiiInputManager::GetInputSlotState(xiiInputSlot_MouseButton1) >= xiiKeyState::Pressed);
      cfg.AddMouseButtonEvent(2, xiiInputManager::GetInputSlotState(xiiInputSlot_MouseButton2) >= xiiKeyState::Pressed);
    }

    float fMouseWheelY = 0.0f;
    if (xiiInputManager::GetInputSlotState(xiiInputSlot_MouseWheelDown) == xiiKeyState::Pressed)
    {
      fMouseWheelY = -1.0f;
    }
    if (xiiInputManager::GetInputSlotState(xiiInputSlot_MouseWheelUp) == xiiKeyState::Pressed)
    {
      fMouseWheelY = +1.0f;
    }
    cfg.AddMouseWheelEvent(0.0f, fMouseWheelY);

    cfg.AddKeyEvent(ImGuiKey_LeftAlt, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftAlt) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightAlt, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightAlt) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftCtrl, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftCtrl) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightCtrl, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightCtrl) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftShift, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftShift) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightShift, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightShift) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftSuper, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftWin) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightSuper, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightWin) >= xiiKeyState::Pressed);

    cfg.AddKeyEvent(ImGuiKey_Tab, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyTab) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftArrow, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeft) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightArrow, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRight) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_UpArrow, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyUp) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_DownArrow, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyDown) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_PageUp, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyPageUp) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_PageDown, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyPageDown) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Home, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyHome) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_End, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyEnd) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Delete, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyDelete) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Backspace, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyBackspace) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Enter, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyReturn) >= xiiKeyState::Pressed || xiiInputManager::GetInputSlotState(xiiInputSlot_KeyNumpadEnter) >= xiiKeyState::Pressed);

    cfg.AddKeyEvent(ImGuiKey_Escape, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyEscape) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_A, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyA) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_C, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyC) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_V, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyV) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_X, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyX) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Y, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyY) >= xiiKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Z, xiiInputManager::GetInputSlotState(xiiInputSlot_KeyZ) >= xiiKeyState::Pressed);
  }
  else
  {
    cfg.ClearInputKeys();
  }

  ImGui::NewFrame();

  m_bImguiWantsInput = cfg.WantCaptureKeyboard || cfg.WantCaptureMouse;
}

void xiiImguiSingleton::GameApplicationEventHandler(const xiiGameApplicationExecutionEvent& e)
{
  if (e.m_Type == xiiGameApplicationExecutionEvent::Type::AfterUpdatePlugins)
  {
    ImGui::EndFrame();
  }
}

XII_STATICLINK_FILE(DearImguiPlugin, DearImguiPlugin_Implementation_DearImgui);
