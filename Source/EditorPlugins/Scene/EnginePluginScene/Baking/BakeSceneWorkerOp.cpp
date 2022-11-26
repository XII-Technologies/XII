#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Baking/BakeSceneWorkerOp.h>

#ifdef BUILDSYSTEM_ENABLE_EMBREE_SUPPORT

#  include <BakingPlugin/BakingScene.h>
#  include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#  include <Foundation/Utilities/Progress.h>
#  include <ToolsFoundation/Document/DocumentManager.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpWorker_BakeScene, 1, xiiRTTIDefaultAllocator<xiiLongOpWorker_BakeScene>);
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiLongOpWorker_BakeScene::InitializeExecution(xiiStreamReader& config, const xiiUuid& DocumentGuid)
{
  xiiEngineProcessDocumentContext* pDocContext = xiiEngineProcessDocumentContext::GetDocumentContext(DocumentGuid);

  if (pDocContext == nullptr)
    return XII_FAILURE;

  config >> m_sOutputPath;

  {
    m_pScene = xiiBaking::GetSingleton()->GetOrCreateScene(*pDocContext->GetWorld());

    XII_SUCCEED_OR_RETURN(m_pScene->Extract());
  }

  return XII_SUCCESS;
}

xiiResult xiiLongOpWorker_BakeScene::Execute(xiiProgress& progress, xiiStreamWriter& proxydata)
{
  XII_SUCCEED_OR_RETURN(m_pScene->Bake(m_sOutputPath, progress));

  return XII_SUCCESS;
}

#endif
