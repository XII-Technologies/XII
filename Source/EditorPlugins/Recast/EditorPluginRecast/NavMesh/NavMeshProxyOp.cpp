#include <EditorPluginRecast/EditorPluginRecastPCH.h>

#include <EditorPluginRecast/NavMesh/NavMeshProxyOp.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpProxy_BuildNavMesh, 1, xiiRTTIDefaultAllocator<xiiLongOpProxy_BuildNavMesh>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiLongOpProxy_BuildNavMesh::InitializeRegistered(const xiiUuid& documentGuid, const xiiUuid& componentGuid)
{
  m_DocumentGuid  = documentGuid;
  m_ComponentGuid = componentGuid;
}

void xiiLongOpProxy_BuildNavMesh::GetReplicationInfo(xiiStringBuilder& out_sReplicationOpType, xiiStreamWriter& description)
{
  out_sReplicationOpType = "xiiLongOpWorker_BuildNavMesh";

  {
    xiiStringBuilder sComponentGuid, sOutputFile;
    xiiConversionUtils::ToString(m_ComponentGuid, sComponentGuid);

    sOutputFile.Format(":project/AssetCache/Generated/{0}.xiiRecastNavMesh", sComponentGuid);

    description << sOutputFile;
  }

  const xiiDocument*       pDoc       = xiiDocumentManager::GetDocumentByGuid(m_DocumentGuid);
  const xiiDocumentObject* pObject    = pDoc->GetObjectManager()->GetObject(m_ComponentGuid);
  xiiVariant               configGuid = pObject->GetTypeAccessor().GetValue("NavMeshConfig");

  const xiiDocumentObject* pConfig = pDoc->GetObjectManager()->GetObject(configGuid.Get<xiiUuid>());
  auto&                    cfg     = pConfig->GetTypeAccessor();

  xiiRecastConfig rcCfg;
  rcCfg.m_fAgentHeight                    = cfg.GetValue("AgentHeight").Get<float>();
  rcCfg.m_fAgentRadius                    = cfg.GetValue("AgentRadius").Get<float>();
  rcCfg.m_fAgentClimbHeight               = cfg.GetValue("AgentClimbHeight").Get<float>();
  rcCfg.m_WalkableSlope                   = cfg.GetValue("WalkableSlope").Get<xiiAngle>();
  rcCfg.m_fCellSize                       = cfg.GetValue("CellSize").Get<float>();
  rcCfg.m_fCellHeight                     = cfg.GetValue("CellHeight").Get<float>();
  rcCfg.m_fMinRegionSize                  = cfg.GetValue("MinRegionSize").Get<float>();
  rcCfg.m_fRegionMergeSize                = cfg.GetValue("RegionMergeSize").Get<float>();
  rcCfg.m_fDetailMeshSampleDistanceFactor = cfg.GetValue("SampleDistanceFactor").Get<float>();
  rcCfg.m_fDetailMeshSampleErrorFactor    = cfg.GetValue("SampleErrorFactor").Get<float>();
  rcCfg.m_fMaxSimplificationError         = cfg.GetValue("MaxSimplification").Get<float>();
  rcCfg.m_fMaxEdgeLength                  = cfg.GetValue("MaxEdgeLength").Get<float>();
  rcCfg.Serialize(description).IgnoreResult();
}

void xiiLongOpProxy_BuildNavMesh::Finalize(xiiResult result, const xiiDataBuffer& resultData)
{
  if (result.Succeeded())
  {
    xiiQtEditorApp::GetSingleton()->ReloadEngineResources();
  }
}
