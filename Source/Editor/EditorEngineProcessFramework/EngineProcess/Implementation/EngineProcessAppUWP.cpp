#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderTargetSetup.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>

xiiEditorEngineProcessAppUWP::xiiEditorEngineProcessAppUWP() = default;

xiiEditorEngineProcessAppUWP::~xiiEditorEngineProcessAppUWP() = default;

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
