#include <GameEngine/GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/Input/InputManager.h>
#  include <Foundation/Configuration/Startup.h>
#  include <Foundation/Time/Clock.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  include <GameEngine/GameApplication/GameApplication.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererCore/Textures/Texture2DResource.h>

namespace
{
  void* xiiImguiAllocate(size_t uiSize, void* pUserData)
  {
    xiiAllocatorBase* pAllocator = static_cast<xiiAllocatorBase*>(pUserData);
    return pAllocator->Allocate(uiSize, XII_ALIGNMENT_MINIMUM);
  }

  void xiiImguiDeallocate(void* pPtr, void* pUserData)
  {
    if (pPtr != nullptr)
    {
      xiiAllocatorBase* pAllocator = static_cast<xiiAllocatorBase*>(pUserData);
      pAllocator->Deallocate(pPtr);
    }
  }
} // namespace

XII_IMPLEMENT_SINGLETON(xiiImgui);

xiiImgui::xiiImgui(xiiImguiConfigFontCallback configFontCallback, xiiImguiConfigStyleCallback configStyleCallback) :
  m_SingletonRegistrar(this), m_Allocator("ImGui", xiiFoundation::GetDefaultAllocator()), m_ConfigStyleCallback(configStyleCallback)
{
  Startup(configFontCallback);
}

xiiImgui::~xiiImgui()
{
  Shutdown();
}

void xiiImgui::SetCurrentContextForView(const xiiViewHandle& hView)
{
  XII_LOCK(m_ViewToContextTableMutex);

  Context& context = m_ViewToContextTable[hView];
  if (context.m_pImGuiContext == nullptr)
  {
    context.m_pImGuiContext = CreateContext();
  }

  ImGui::SetCurrentContext(context.m_pImGuiContext);

  xiiUInt64 uiCurrentFrameCounter = xiiRenderWorld::GetFrameCounter();
  if (context.m_uiFrameBeginCounter != uiCurrentFrameCounter)
  {
    // Last frame was not rendered. This can happen if a render pipeline with dear imgui renderer is used.
    if (context.m_uiFrameRenderCounter != context.m_uiFrameBeginCounter)
    {
      ImGui::EndFrame();
    }

    BeginFrame(hView);
    context.m_uiFrameBeginCounter = uiCurrentFrameCounter;
  }
}

void xiiImgui::Startup(xiiImguiConfigFontCallback configFontCallback)
{
  ImGui::SetAllocatorFunctions(&xiiImguiAllocate, &xiiImguiDeallocate, &m_Allocator);

  m_pSharedFontAtlas = XII_DEFAULT_NEW(ImFontAtlas);

  if (configFontCallback.IsValid())
  {
    configFontCallback(*m_pSharedFontAtlas);
  }

  unsigned char* pixels;
  int            width, height;
  m_pSharedFontAtlas->GetTexDataAsRGBA32(&pixels, &width, &height); // Load as RGBA 32-bits (75% of the memory is wasted, but default font
                                                                    // is so small) because it is more likely to be compatible with user's
                                                                    // existing shaders. If your ImTextureId represent a higher-level
                                                                    // concept than just a GL texture id, consider calling
                                                                    // GetTexDataAsAlpha8() instead to save on GPU memory.

  xiiTexture2DResourceHandle hFont = xiiResourceManager::GetExistingResource<xiiTexture2DResource>("ImguiFont");

  if (!hFont.IsValid())
  {
    xiiGALSystemMemoryDescription memoryDesc;
    memoryDesc.m_pData        = pixels;
    memoryDesc.m_uiRowPitch   = width * 4;
    memoryDesc.m_uiSlicePitch = width * height * 4;

    xiiTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth  = width;
    desc.m_DescGAL.m_uiHeight = height;
    desc.m_DescGAL.m_Format   = xiiGALResourceFormat::RGBAUByteNormalized;
    desc.m_InitialContent     = xiiMakeArrayPtr(&memoryDesc, 1);

    hFont = xiiResourceManager::GetOrCreateResource<xiiTexture2DResource>("ImguiFont", std::move(desc));
  }

  m_Textures.PushBack(hFont);

  const size_t id           = (size_t)m_Textures.GetCount() - 1;
  m_pSharedFontAtlas->TexID = reinterpret_cast<void*>(id);
}

void xiiImgui::Shutdown()
{
  m_Textures.Clear();

  m_pSharedFontAtlas = nullptr;

  for (auto it = m_ViewToContextTable.GetIterator(); it.IsValid(); ++it)
  {
    Context& context = it.Value();
    ImGui::DestroyContext(context.m_pImGuiContext);
    context.m_pImGuiContext = nullptr;
  }
  m_ViewToContextTable.Clear();
}

ImGuiContext* xiiImgui::CreateContext()
{
  // imgui reads the global context pointer WHILE creating a new context
  // so if we don't reset it to null here, it will try to access it, and crash
  // if imgui was active on the same thread before
  ImGui::SetCurrentContext(nullptr);
  ImGuiContext* context = ImGui::CreateContext(m_pSharedFontAtlas.Borrow());
  ImGui::SetCurrentContext(context);

  ImGuiIO& cfg = ImGui::GetIO();

  cfg.DisplaySize.x = 1650;
  cfg.DisplaySize.y = 1080;

  cfg.KeyMap[ImGuiKey_Tab]        = ImGuiKey_Tab;
  cfg.KeyMap[ImGuiKey_LeftArrow]  = ImGuiKey_LeftArrow;
  cfg.KeyMap[ImGuiKey_RightArrow] = ImGuiKey_RightArrow;
  cfg.KeyMap[ImGuiKey_UpArrow]    = ImGuiKey_UpArrow;
  cfg.KeyMap[ImGuiKey_DownArrow]  = ImGuiKey_DownArrow;
  cfg.KeyMap[ImGuiKey_PageUp]     = ImGuiKey_PageUp;
  cfg.KeyMap[ImGuiKey_PageDown]   = ImGuiKey_PageDown;
  cfg.KeyMap[ImGuiKey_Home]       = ImGuiKey_Home;
  cfg.KeyMap[ImGuiKey_End]        = ImGuiKey_End;
  cfg.KeyMap[ImGuiKey_Delete]     = ImGuiKey_Delete;
  cfg.KeyMap[ImGuiKey_Backspace]  = ImGuiKey_Backspace;
  cfg.KeyMap[ImGuiKey_Enter]      = ImGuiKey_Enter;
  cfg.KeyMap[ImGuiKey_Escape]     = ImGuiKey_Escape;
  cfg.KeyMap[ImGuiKey_A]          = ImGuiKey_A;
  cfg.KeyMap[ImGuiKey_C]          = ImGuiKey_C;
  cfg.KeyMap[ImGuiKey_V]          = ImGuiKey_V;
  cfg.KeyMap[ImGuiKey_X]          = ImGuiKey_X;
  cfg.KeyMap[ImGuiKey_Y]          = ImGuiKey_Y;
  cfg.KeyMap[ImGuiKey_Z]          = ImGuiKey_Z;

  if (m_ConfigStyleCallback.IsValid())
  {
    m_ConfigStyleCallback(ImGui::GetStyle());
  }

  return context;
}

void xiiImgui::BeginFrame(const xiiViewHandle& hView)
{
  xiiView* pView = nullptr;
  if (!xiiRenderWorld::TryGetView(hView, pView))
  {
    return;
  }

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

    float mousex, mousey;
    xiiInputManager::GetInputSlotState(xiiInputSlot_MousePositionX, &mousex);
    xiiInputManager::GetInputSlotState(xiiInputSlot_MousePositionY, &mousey);
    cfg.MousePos.x   = cfg.DisplaySize.x * mousex;
    cfg.MousePos.y   = cfg.DisplaySize.y * mousey;
    cfg.MouseDown[0] = xiiInputManager::GetInputSlotState(xiiInputSlot_MouseButton0) >= xiiKeyState::Pressed;
    cfg.MouseDown[1] = xiiInputManager::GetInputSlotState(xiiInputSlot_MouseButton1) >= xiiKeyState::Pressed;
    cfg.MouseDown[2] = xiiInputManager::GetInputSlotState(xiiInputSlot_MouseButton2) >= xiiKeyState::Pressed;

    cfg.MouseWheel = 0;
    if (xiiInputManager::GetInputSlotState(xiiInputSlot_MouseWheelDown) == xiiKeyState::Pressed)
      cfg.MouseWheel = -1;
    if (xiiInputManager::GetInputSlotState(xiiInputSlot_MouseWheelUp) == xiiKeyState::Pressed)
      cfg.MouseWheel = +1;

    cfg.KeyAlt = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftAlt) >= xiiKeyState::Pressed ||
      xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightAlt) >= xiiKeyState::Pressed;
    cfg.KeyCtrl = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftCtrl) >= xiiKeyState::Pressed ||
      xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightCtrl) >= xiiKeyState::Pressed;
    cfg.KeyShift = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftShift) >= xiiKeyState::Pressed ||
      xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightShift) >= xiiKeyState::Pressed;
    cfg.KeySuper = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftWin) >= xiiKeyState::Pressed ||
      xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightWin) >= xiiKeyState::Pressed;

    cfg.KeysDown[ImGuiKey_Tab]        = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyTab) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_LeftArrow]  = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeft) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_RightArrow] = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRight) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_UpArrow]    = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyUp) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_DownArrow]  = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyDown) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_PageUp]     = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyPageUp) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_PageDown]   = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyPageDown) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Home]       = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyHome) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_End]        = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyEnd) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Delete]     = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyDelete) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Backspace]  = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyBackspace) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Enter]      = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyReturn) >= xiiKeyState::Pressed ||
      xiiInputManager::GetInputSlotState(xiiInputSlot_KeyNumpadEnter) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Escape] = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyEscape) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_A]      = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyA) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_C]      = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyC) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_V]      = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyV) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_X]      = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyX) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Y]      = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyY) >= xiiKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Z]      = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyZ) >= xiiKeyState::Pressed;
  }
  else
  {
    cfg.ClearInputCharacters();

    cfg.MousePos.x = -1;
    cfg.MousePos.y = -1;

    cfg.MouseDown[0] = false;
    cfg.MouseDown[1] = false;
    cfg.MouseDown[2] = false;

    cfg.MouseWheel = 0;

    cfg.KeyAlt   = false;
    cfg.KeyCtrl  = false;
    cfg.KeyShift = false;
    cfg.KeySuper = false;

    for (int i = 0; i <= ImGuiKey_COUNT; ++i)
      cfg.KeysDown[i] = false;
  }

  ImGui::NewFrame();

  m_bImguiWantsInput = cfg.WantCaptureKeyboard || cfg.WantCaptureMouse;
}

#endif

XII_STATICLINK_FILE(GameEngine, GameEngine_DearImgui_Implementation_DearImgui);
