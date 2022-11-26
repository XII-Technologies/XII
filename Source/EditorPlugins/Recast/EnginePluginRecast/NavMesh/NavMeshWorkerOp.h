#include <EnginePluginRecast/EnginePluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class xiiLongOpWorker_BuildNavMesh : public xiiLongOpWorker
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpWorker_BuildNavMesh, xiiLongOpWorker);

public:
  virtual xiiResult InitializeExecution(xiiStreamReader& config, const xiiUuid& DocumentGuid) override;
  virtual xiiResult Execute(xiiProgress& progress, xiiStreamWriter& proxydata) override;

  xiiString                                 m_sOutputPath;
  xiiRecastConfig                           m_NavMeshConfig;
  xiiWorldGeoExtractionUtil::MeshObjectList m_ExtractedObjects;
};
