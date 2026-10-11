/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
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
  xiiDynamicArray<xiiRenderPipelinePass*>            passes;
  xiiDynamicArray<xiiUuid>                           passUuids;
  xiiDynamicArray<xiiDocumentObject_ConnectionBase*> toolConnections;
  m_Context.GetObjectsByType(passes, &passUuids);
  m_Context.GetObjectsByType(toolConnections);

  xiiRenderPipelineResourceDescriptor descriptor;
  xiiHashTable<xiiUuid, xiiUInt32>    passUuidToIndex;
  for (xiiUInt32 i = 0; i < passes.GetCount(); ++i)
  {
    xiiStringBuilder name;
    xiiConversionUtils::ToString(passUuids[i], name);
    xiiVariantDictionary                           settings;
    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    passes[i]->GetDynamicRTTI()->GetAllProperties(properties);
    for (const auto* property : properties)
    {
      if (property->GetCategory() == xiiPropertyCategory::Member && !property->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
        settings.Insert(property->GetPropertyName(), xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(property), passes[i]));
    }
    passUuidToIndex.Insert(passUuids[i], descriptor.AddPass(name, passes[i]->GetDynamicRTTI()->GetTypeName(), settings));
  }
  for (const auto* connection : toolConnections)
  {
    xiiUInt32 source, target;
    if (!passUuidToIndex.TryGetValue(connection->m_Source, source) || !passUuidToIndex.TryGetValue(connection->m_Target, target))
      return xiiStatus("Render pipeline connection references a missing pass.");
    XII_SUCCEED_OR_RETURN(descriptor.Connect(source, connection->m_SourcePin, target, connection->m_TargetPin));
  }

  xiiSharedPtr<const xiiRenderPipeline> compiled;
  xiiStringBuilder                      error;
  if (xiiRenderPipeline::Compile(descriptor, compiled, &error).Failed())
    return xiiStatus(error.GetView());

  xiiDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);
  xiiAssetFileHeader header;
  header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
  XII_SUCCEED_OR_RETURN(header.Write(file));
  // Version 3 replaces the obsolete pass/extractor stream with the reflected resource definition.
  const xiiUInt8 version = 3;
  file << version;
  XII_SUCCEED_OR_RETURN(descriptor.Serialize(file));
  if (file.Close().Failed())
    return xiiStatus(xiiFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));
  return xiiStatus(XII_SUCCESS);
}
