#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Pipeline/Extractor.h>
#include <GraphicsCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineContext, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "RenderPipeline"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRenderPipelineContext::xiiRenderPipelineContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

void xiiRenderPipelineContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  xiiEngineProcessDocumentContext::HandleMessage(pMsg);
}

void xiiRenderPipelineContext::OnInitialize()
{
}

xiiEngineProcessViewContext* xiiRenderPipelineContext::CreateViewContext()
{
  XII_ASSERT_DEV(false, "Should not be called");
  return nullptr;
}

void xiiRenderPipelineContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_ASSERT_DEV(false, "Should not be called");
}

xiiStatus xiiRenderPipelineContext::ExportDocument(const xiiExportDocumentMsgToEngine* pMsg)
{
  xiiDynamicArray<xiiRenderPipelinePassBase*>                passes;
  xiiDynamicArray<xiiExtractor*>                             extractors;
  xiiDynamicArray<xiiRenderPipelineResourceLoaderConnection> connections;

  xiiDynamicArray<xiiUuid>                           passUuids;
  xiiDynamicArray<xiiDocumentObject_ConnectionBase*> toolConnections;

  m_Context.GetObjectsByType(passes, &passUuids);
  m_Context.GetObjectsByType(extractors);
  m_Context.GetObjectsByType(toolConnections);

  xiiHashTable<xiiUuid, xiiUInt32> passUuidToIndex;
  for (xiiUInt32 i = 0; i < passUuids.GetCount(); ++i)
  {
    passUuidToIndex.Insert(passUuids[i], i);
  }
  connections.SetCount(toolConnections.GetCount());
  for (xiiUInt32 i = 0; i < toolConnections.GetCount(); i++)
  {
    xiiDocumentObject_ConnectionBase*          pConnection      = toolConnections[i];
    xiiRenderPipelineResourceLoaderConnection& engineConnection = connections[i];
    XII_VERIFY(passUuidToIndex.TryGetValue(pConnection->m_Source, engineConnection.m_uiSource), "");
    XII_VERIFY(passUuidToIndex.TryGetValue(pConnection->m_Target, engineConnection.m_uiTarget), "");
    engineConnection.m_sSourcePin = pConnection->m_SourcePin;
    engineConnection.m_sTargetPin = pConnection->m_TargetPin;
  }

  xiiDefaultMemoryStreamStorage storage;
  {
    // Export Resource Data
    xiiMemoryStreamWriter writer(&storage);
    XII_SUCCEED_OR_RETURN(xiiRenderPipelineResourceLoader::ExportPipeline(passes.GetArrayPtr(), extractors.GetArrayPtr(), connections.GetArrayPtr(), writer));
  }

  xiiDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  {
    // File Header
    xiiAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(file).IgnoreResult();

    xiiUInt8 uiVersion = 2;
    file << uiVersion;
  }

  {
    // Resource Data
    xiiUInt32 uiSize = storage.GetStorageSize32();
    file << uiSize;
    if (storage.CopyToStream(file).Failed())
      return xiiStatus(xiiFmt("Failed to copy content to file writer for '{}'", pMsg->m_sOutputFile));
  }

  // do the actual file writing
  if (file.Close().Failed())
    return xiiStatus(xiiFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return XII_SUCCESS;
}
