#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

xiiEditorEngineProcessAppUWP::xiiEditorEngineProcessAppUWP() {}

xiiEditorEngineProcessAppUWP::~xiiEditorEngineProcessAppUWP() {}

xiiViewHandle xiiEditorEngineProcessAppUWP::CreateRemoteWindowAndView(xiiCamera* pCamera)
{
  XII_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

  return xiiEditorEngineProcessApp::CreateRemoteWindowAndView(pCamera);
}

xiiRenderPipelineResourceHandle xiiEditorEngineProcessAppUWP::CreateDefaultMainRenderPipeline()
{
  return xiiEditorEngineProcessApp::CreateDefaultMainRenderPipeline();
}

xiiRenderPipelineResourceHandle xiiEditorEngineProcessAppUWP::CreateDefaultDebugRenderPipeline()
{
  return CreateDefaultMainRenderPipeline();
}
