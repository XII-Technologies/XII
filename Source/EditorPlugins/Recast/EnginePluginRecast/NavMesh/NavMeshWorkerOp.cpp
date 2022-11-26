#include <EnginePluginRecast/EnginePluginRecastPCH.h>

#include <EnginePluginRecast/NavMesh/NavMeshWorkerOp.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/Progress.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpWorker_BuildNavMesh, 1, xiiRTTIDefaultAllocator<xiiLongOpWorker_BuildNavMesh>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiLongOpWorker_BuildNavMesh::InitializeExecution(xiiStreamReader& config, const xiiUuid& DocumentGuid)
{
  xiiEngineProcessDocumentContext* pDocContext = xiiEngineProcessDocumentContext::GetDocumentContext(DocumentGuid);

  if (pDocContext == nullptr)
    return XII_FAILURE;

  config >> m_sOutputPath;
  XII_SUCCEED_OR_RETURN(m_NavMeshConfig.Deserialize(config));

  XII_SUCCEED_OR_RETURN(xiiRecastNavMeshBuilder::ExtractWorldGeometry(*pDocContext->GetWorld(), m_ExtractedObjects));

  return XII_SUCCESS;
}

xiiResult xiiLongOpWorker_BuildNavMesh::Execute(xiiProgress& progress, xiiStreamWriter& proxydata)
{
  xiiProgressRange pgRange("Generating NavMesh", 2, true, &progress);
  pgRange.SetStepWeighting(0, 0.95f);
  pgRange.SetStepWeighting(1, 0.05f);

  xiiRecastNavMeshBuilder            NavMeshBuilder;
  xiiRecastNavMeshResourceDescriptor desc;

  if (!pgRange.BeginNextStep("Building NavMesh"))
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(NavMeshBuilder.Build(m_NavMeshConfig, m_ExtractedObjects, desc, progress));

  if (!pgRange.BeginNextStep("Writing Result"))
    return XII_FAILURE;

  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(m_sOutputPath));

  // not really used for navmeshes, as they are not strictly linked to a specific version of a scene document
  // thus the scene hash and document version are irrelevant and should just stay static for now
  xiiAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);
  XII_SUCCEED_OR_RETURN(header.Write(file));

  XII_SUCCEED_OR_RETURN(desc.Serialize(file));

  return XII_SUCCESS;
}
