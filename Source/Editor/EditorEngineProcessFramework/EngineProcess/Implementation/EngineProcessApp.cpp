/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>

XII_IMPLEMENT_SINGLETON(xiiEditorEngineProcessApp);

xiiEditorEngineProcessApp::xiiEditorEngineProcessApp() :
  m_SingletonRegistrar(this)
{
}

xiiEditorEngineProcessApp::~xiiEditorEngineProcessApp()
{
  DestroyRemoteWindow();
}

void xiiEditorEngineProcessApp::SetRemoteMode()
{
  m_Mode = xiiEditorEngineProcessMode::Remote;

  CreateRemoteWindow();
}

xiiRegisteredWindowHandle xiiEditorEngineProcessApp::CreateRemoteWindow()
{
  XII_ASSERT_DEV(IsRemoteMode(), "Incorrect application mode.");

  if (!m_hWindow.IsInvalidated())
    return m_hWindow;

  xiiUniquePtr<xiiWindow> pWindow = XII_DEFAULT_NEW(xiiWindow);

  xiiWindowCreationDescription description;
  description.m_uiWindowNumber   = 0;
  description.m_bClipMouseCursor = false;
  description.m_bShowMouseCursor = true;
  description.m_Resolution       = xiiSizeU32(960, 540);
  description.m_WindowMode       = xiiWindowMode::WindowFixedResolution;
  description.m_Title            = "Engine View";

  pWindow->Initialize(description).AssertSuccess("Failed to initialize remote window.");

  m_hWindow = xiiWindowManager::GetSingleton()->Register("Engine View", this, std::move(pWindow));

  return m_hWindow;
}

void xiiEditorEngineProcessApp::DestroyRemoteWindow()
{
  if (xiiWindowManager* pWindowManager = xiiWindowManager::GetSingleton())
  {
    pWindowManager->CloseAll(this);
  }
  m_hWindow.Invalidate();
}
