/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
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
  xiiDynamicArray<xiiUuid>                           connectionUuids;
  m_Context.GetObjectsByType(passes, &passUuids);
  m_Context.GetObjectsByType(toolConnections, &connectionUuids);

  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.m_Id = GetDocumentGuid();
  xiiHashTable<xiiUuid, xiiUInt32>                             passUuidToIndex;
  xiiHashTable<xiiUuid, const xiiRenderPipelineInterfacePass*> interfaces;
  xiiDynamicArray<xiiUInt32>                                   sortedPasses;
  for (xiiUInt32 i = 0; i < passes.GetCount(); ++i)
    sortedPasses.PushBack(i);
  sortedPasses.Sort([&](xiiUInt32 a, xiiUInt32 b) { return passUuids[a] < passUuids[b]; });
  for (auto i : sortedPasses)
  {
    xiiStringBuilder name;
    xiiConversionUtils::ToString(passUuids[i], name);
    if (passes[i]->IsInstanceOf<xiiRenderPipelineInterfacePass>())
    {
      interfaces.Insert(passUuids[i], static_cast<const xiiRenderPipelineInterfacePass*>(passes[i]));
      continue;
    }
    if (passes[i]->IsInstanceOf<xiiRenderPipelineSubgraphPass>())
    {
      const auto* instance = static_cast<const xiiRenderPipelineSubgraphPass*>(passes[i]);
      if (instance->m_sResource.IsEmpty())
        return xiiStatus("Subgraph instance has no definition resource.");
      const auto                                 handle = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(instance->m_sResource);
      xiiResourceLock<xiiRenderPipelineResource> acquired(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      const auto                                 snapshot = acquired.IsValid() ? acquired->GetAuthoring() : xiiSharedPtr<const xiiRenderPipelineAuthoring>();
      if (!snapshot)
        return xiiStatus("Subgraph instance references a missing definition resource.");
      bool referenced = false;
      for (const auto& reference : descriptor.m_References)
        referenced |= reference.m_Id == snapshot->m_Descriptor.m_Id;
      if (!referenced)
        descriptor.m_References.PushBack({snapshot->m_Descriptor.m_Id, instance->m_sResource});
      const auto index  = descriptor.AddSubgraph(name, snapshot->m_Descriptor.m_Id);
      auto&      node   = descriptor.m_Passes[index];
      node.m_Id         = passUuids[i];
      node.m_Parameters = instance->m_Parameters;
      node.m_Overrides  = instance->m_Overrides;
      if (!instance->m_PinConfigurations.IsEmpty())
      {
        xiiVariantDictionary properties;
        if (instance->GetProperties(properties).Failed())
          return xiiStatus("Cannot serialize subgraph interface pin settings.");
        if (const auto* pins = properties.GetValue("PinConfigurations"))
          node.m_Properties.Insert("PinConfigurations", *pins);
      }
      passUuidToIndex.Insert(passUuids[i], index);
      continue;
    }
    xiiVariantDictionary settings;
    if (passes[i]->GetProperties(settings).Failed())
      return xiiStatus(xiiFmt("Cannot serialize reflected configuration for pass '{}'.", name));
    const auto index                        = descriptor.AddPass(name, passes[i]->GetDynamicRTTI()->GetTypeName(), settings);
    descriptor.m_Passes[index].m_Id         = passUuids[i];
    descriptor.m_Passes[index].m_Parameters = passes[i]->m_ParameterOverrides;
    passUuidToIndex.Insert(passUuids[i], index);
  }
  xiiDynamicArray<xiiUInt32> sortedConnections;
  for (xiiUInt32 i = 0; i < toolConnections.GetCount(); ++i)
    sortedConnections.PushBack(i);
  sortedConnections.Sort([&](xiiUInt32 a, xiiUInt32 b) { return connectionUuids[a] < connectionUuids[b]; });
  for (auto connectionIndex : sortedConnections)
  {
    const auto*                           connection = toolConnections[connectionIndex];
    const xiiRenderPipelineInterfacePass* boundary   = nullptr;
    if (interfaces.TryGetValue(connection->m_Source, boundary))
    {
      if (!boundary->m_bInput || !passUuidToIndex.Contains(connection->m_Target))
        return xiiStatus("External input must connect to an internal pass input.");
      descriptor.m_Interface.PushBack({boundary->m_sName, true, connection->m_Target, connection->m_TargetPin});
      continue;
    }
    if (interfaces.TryGetValue(connection->m_Target, boundary))
    {
      if (boundary->m_bInput || !passUuidToIndex.Contains(connection->m_Source))
        return xiiStatus("External output must connect to an internal pass output.");
      descriptor.m_Interface.PushBack({boundary->m_sName, false, connection->m_Source, connection->m_SourcePin});
      continue;
    }
    xiiUInt32 source, target;
    if (!passUuidToIndex.TryGetValue(connection->m_Source, source) || !passUuidToIndex.TryGetValue(connection->m_Target, target))
      return xiiStatus("Render pipeline connection references a missing pass.");
    XII_SUCCEED_OR_RETURN(descriptor.Connect(source, connection->m_SourcePin, target, connection->m_TargetPin));
    descriptor.m_Connections.PeekBack().m_Id = connectionUuids[connectionIndex];
  }

  xiiDynamicArray<xiiRenderPipelinePin> externalPins;
  xiiStringBuilder                      interfaceError;
  if (xiiGetRenderPipelineInterface(descriptor, externalPins, &interfaceError).Failed())
    return xiiStatus(interfaceError.GetView());
  for (auto it = interfaces.GetIterator(); it.IsValid(); ++it)
  {
    const auto* boundary = it.Value();
    bool        found    = false;
    for (const auto& pin : externalPins)
      if (pin.m_sName == boundary->m_sName && pin.m_bInput == boundary->m_bInput)
      {
        if (pin.m_Type != boundary->m_uiResourceType || (pin.m_Format != xiiGALResourceFormat::Unknown && boundary->m_Format != xiiGALResourceFormat::Unknown && pin.m_Format != boundary->m_Format))
          return xiiStatus("Interface schema does not match its reflected internal resource pin.");
        found = true;
      }
    if (!found)
      return xiiStatus("Every external interface node must have an internal connection.");
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
