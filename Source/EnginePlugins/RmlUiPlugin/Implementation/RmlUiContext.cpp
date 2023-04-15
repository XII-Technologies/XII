#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/Extractor.h>
#include <RmlUiPlugin/RmlUiContext.h>

namespace
{
  static const char* s_szXIIKeys[] = {xiiInputSlot_KeyTab, xiiInputSlot_KeyLeft, xiiInputSlot_KeyUp, xiiInputSlot_KeyRight, xiiInputSlot_KeyDown,
                                      xiiInputSlot_KeyPageUp, xiiInputSlot_KeyPageDown, xiiInputSlot_KeyHome, xiiInputSlot_KeyEnd, xiiInputSlot_KeyDelete, xiiInputSlot_KeyBackspace,
                                      xiiInputSlot_KeyReturn, xiiInputSlot_KeyNumpadEnter, xiiInputSlot_KeyEscape};

  static Rml::Input::KeyIdentifier s_rmlKeys[] = {Rml::Input::KI_TAB, Rml::Input::KI_LEFT, Rml::Input::KI_UP,
                                                  Rml::Input::KI_RIGHT, Rml::Input::KI_DOWN, Rml::Input::KI_PRIOR, Rml::Input::KI_NEXT, Rml::Input::KI_HOME,
                                                  Rml::Input::KI_END, Rml::Input::KI_DELETE, Rml::Input::KI_BACK, Rml::Input::KI_RETURN, Rml::Input::KI_RETURN,
                                                  Rml::Input::KI_ESCAPE};

  XII_CHECK_AT_COMPILETIME(XII_ARRAY_SIZE(s_szXIIKeys) == XII_ARRAY_SIZE(s_rmlKeys));
} // namespace

xiiRmlUiContext::xiiRmlUiContext(const Rml::String& sName) :
  Rml::Context(sName)
{
}

xiiRmlUiContext::~xiiRmlUiContext() = default;

xiiResult xiiRmlUiContext::LoadDocumentFromResource(const xiiRmlUiResourceHandle& hResource)
{
  UnloadDocument();

  if (hResource.IsValid())
  {
    xiiResourceLock<xiiRmlUiResource> pResource(hResource, xiiResourceAcquireMode::BlockTillLoaded);
    if (pResource.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      LoadDocument(pResource->GetRmlFile().GetData());
    }
  }

  return HasDocument() ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiRmlUiContext::LoadDocumentFromString(const xiiStringView& sContent)
{
  UnloadDocument();

  if (!sContent.IsEmpty())
  {
    Rml::String sRmlContent = Rml::String(sContent.GetStartPointer(), sContent.GetElementCount());

    LoadDocumentFromMemory(sRmlContent);
  }

  return HasDocument() ? XII_SUCCESS : XII_FAILURE;
}

void xiiRmlUiContext::UnloadDocument()
{
  if (HasDocument())
  {
    Rml::Context::UnloadDocument(GetDocument(0));
  }
}

xiiResult xiiRmlUiContext::ReloadDocumentFromResource(const xiiRmlUiResourceHandle& hResource)
{
  Rml::Factory::ClearStyleSheetCache();
  Rml::Factory::ClearTemplateCache();

  return LoadDocumentFromResource(hResource);
}

void xiiRmlUiContext::ShowDocument()
{
  if (HasDocument())
  {
    GetDocument(0)->Show();
  }
}

void xiiRmlUiContext::HideDocument()
{
  if (HasDocument())
  {
    GetDocument(0)->Hide();
  }
}

void xiiRmlUiContext::UpdateInput(const xiiVec2& vMousePos)
{
  float width  = static_cast<float>(GetDimensions().x);
  float height = static_cast<float>(GetDimensions().y);

  m_bWantsInput = vMousePos.x >= 0.0f && vMousePos.x <= width && vMousePos.y >= 0.0f && vMousePos.y <= height;

  const bool bCtrlPressed = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftCtrl) >= xiiKeyState::Pressed ||
    xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightCtrl) >= xiiKeyState::Pressed;
  const bool bShiftPressed = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftShift) >= xiiKeyState::Pressed ||
    xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightShift) >= xiiKeyState::Pressed;
  const bool bAltPressed = xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftAlt) >= xiiKeyState::Pressed ||
    xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightAlt) >= xiiKeyState::Pressed;

  int modifierState = 0;
  modifierState |= bCtrlPressed ? Rml::Input::KM_CTRL : 0;
  modifierState |= bShiftPressed ? Rml::Input::KM_SHIFT : 0;
  modifierState |= bAltPressed ? Rml::Input::KM_ALT : 0;

  // Mouse
  {
    ProcessMouseMove(static_cast<int>(vMousePos.x), static_cast<int>(vMousePos.y), modifierState);

    static const char* szMouseButtons[] = {xiiInputSlot_MouseButton0, xiiInputSlot_MouseButton1, xiiInputSlot_MouseButton2};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(szMouseButtons); ++i)
    {
      xiiKeyState::Enum state = xiiInputManager::GetInputSlotState(szMouseButtons[i]);
      if (state == xiiKeyState::Pressed)
      {
        ProcessMouseButtonDown(i, modifierState);
      }
      else if (state == xiiKeyState::Released)
      {
        ProcessMouseButtonUp(i, modifierState);
      }
    }

    if (xiiInputManager::GetInputSlotState(xiiInputSlot_MouseWheelDown) == xiiKeyState::Pressed)
    {
      m_bWantsInput |= !ProcessMouseWheel(1.0f, modifierState);
    }
    if (xiiInputManager::GetInputSlotState(xiiInputSlot_MouseWheelUp) == xiiKeyState::Pressed)
    {
      m_bWantsInput |= !ProcessMouseWheel(-1.0f, modifierState);
    }
  }

  // Keyboard
  {
    xiiUInt32 uiLastChar = xiiInputManager::RetrieveLastCharacter(false);
    if (uiLastChar >= 32) // >= space
    {
      char  szUtf8[8] = "";
      char* pChar     = szUtf8;
      xiiUnicodeUtils::EncodeUtf32ToUtf8(uiLastChar, pChar);
      if (!xiiStringUtils::IsNullOrEmpty(szUtf8))
      {
        m_bWantsInput |= !ProcessTextInput(szUtf8);
      }
    }

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_szXIIKeys); ++i)
    {
      xiiKeyState::Enum state = xiiInputManager::GetInputSlotState(s_szXIIKeys[i]);
      if (state == xiiKeyState::Pressed)
      {
        m_bWantsInput |= !ProcessKeyDown(s_rmlKeys[i], modifierState);
      }
      else if (state == xiiKeyState::Released)
      {
        m_bWantsInput |= !ProcessKeyUp(s_rmlKeys[i], modifierState);
      }
    }
  }
}

void xiiRmlUiContext::SetOffset(const xiiVec2I32& vOffset)
{
  m_vOffset = vOffset;
}

void xiiRmlUiContext::SetSize(const xiiVec2U32& vSize)
{
  SetDimensions(Rml::Vector2i(vSize.x, vSize.y));
}

void xiiRmlUiContext::SetDpiScale(float fScale)
{
  SetDensityIndependentPixelRatio(fScale);
}

void xiiRmlUiContext::RegisterEventHandler(const char* szIdentifier, EventHandler handler)
{
  xiiHashedString sIdentifier;
  sIdentifier.Assign(szIdentifier);

  m_EventHandler.Insert(sIdentifier, std::move(handler));
}

void xiiRmlUiContext::DeregisterEventHandler(const char* szIdentifier)
{
  m_EventHandler.Remove(xiiTempHashedString(szIdentifier));
}

void xiiRmlUiContext::ExtractRenderData(xiiRmlUiInternal::Extractor& extractor)
{
  if (m_uiExtractedFrame != xiiRenderWorld::GetFrameCounter())
  {
    extractor.BeginExtraction(m_vOffset);

    Render();

    extractor.EndExtraction();

    m_uiExtractedFrame = xiiRenderWorld::GetFrameCounter();
    m_pRenderData      = extractor.GetRenderData();
  }
}

void xiiRmlUiContext::ProcessEvent(const xiiHashedString& sIdentifier, Rml::Event& event)
{
  EventHandler* pEventHandler = nullptr;
  if (m_EventHandler.TryGetValue(sIdentifier, pEventHandler))
  {
    (*pEventHandler)(event);
  }
}

//////////////////////////////////////////////////////////////////////////

Rml::ContextPtr xiiRmlUiInternal::ContextInstancer::InstanceContext(const Rml::String& sName)
{
  return Rml::ContextPtr(XII_DEFAULT_NEW(xiiRmlUiContext, sName));
}

void xiiRmlUiInternal::ContextInstancer::ReleaseContext(Rml::Context* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

void xiiRmlUiInternal::ContextInstancer::Release()
{
  // nothing to do here
}
