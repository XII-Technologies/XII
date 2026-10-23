/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineResource, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiRenderPipelineResource);

xiiRenderPipelineResource::xiiRenderPipelineResource() : xiiResource(DoUpdate::OnAnyThread, 1) {}
xiiSharedPtr<const xiiRenderPipeline> xiiRenderPipelineResource::GetPipeline() const
{
  xiiSharedPtr<const xiiRenderPipelineAuthoring> authoring;
  xiiSharedPtr<const xiiRenderPipeline>          cached;
  xiiDynamicArray<DefinitionDependency>          dependencies;
  {
    XII_LOCK(m_Mutex);
    authoring    = m_pAuthoring;
    cached       = m_pPipeline;
    dependencies = m_DefinitionDependencies;
  }
  if (!authoring)
    return nullptr;
  bool current = cached != nullptr;
  for (const auto& dependency : dependencies)
  {
    xiiResourceLock<xiiRenderPipelineResource> acquired(dependency.m_Resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    current &= acquired.IsValid() && acquired->GetAuthoring() == dependency.m_Snapshot;
  }
  if (current)
    return cached;
  xiiRenderPipelineResourceDescriptor   resolved;
  xiiSharedPtr<const xiiRenderPipeline> pipeline;
  xiiStringBuilder                      error;
  dependencies.Clear();
  if (ResolveDefinitions(authoring->m_Descriptor, resolved, &dependencies, &error).Failed() || xiiRenderPipeline::Compile(resolved, pipeline, &error).Failed())
  {
    xiiLog::Error("Render pipeline '{}': {}", GetResourceIdOrDescription(), error);
    XII_LOCK(m_Mutex);
    if (m_pAuthoring == authoring)
      m_pPipeline = nullptr;
    return nullptr;
  }
  XII_LOCK(m_Mutex);
  if (m_pAuthoring != authoring)
    return nullptr;
  m_pPipeline              = pipeline;
  m_DefinitionDependencies = std::move(dependencies);
  return pipeline;
}

xiiSharedPtr<const xiiRenderPipelineAuthoring> xiiRenderPipelineResource::GetAuthoring() const
{
  XII_LOCK(m_Mutex);
  return m_pAuthoring;
}

xiiResult xiiRenderPipelineResource::ResolveDefinitions(const xiiRenderPipelineResourceDescriptor& descriptor, xiiRenderPipelineResourceDescriptor& out_resolved, xiiDynamicArray<DefinitionDependency>* out_dependencies, xiiStringBuilder* out_error, ResourceResolver resourceResolver)
{
  const auto fail = [&](xiiStringView message) {
    if (out_error)
      out_error->Set(message);
    return XII_FAILURE;
  };
  xiiRenderPipelineResourceDescriptor resolved = descriptor;
  resolved.m_References.Clear();
  xiiGraphDefinition root;
  if (descriptor.MakeGraphDefinition(root).Failed())
    return fail("Invalid root graph definition.");
  xiiHashTable<xiiUuid, xiiUInt32> definitions;
  definitions.Insert(root.m_Id, xiiInvalidIndex);
  for (xiiUInt32 i = 0; i < resolved.m_Definitions.GetCount(); ++i)
  {
    if (definitions.Contains(resolved.m_Definitions[i].m_Id))
      return fail("Duplicate graph definition identity.");
    definitions.Insert(resolved.m_Definitions[i].m_Id, i);
  }
  xiiHashTable<xiiUuid, xiiString> references;
  const auto                       addReferences = [&](const xiiRenderPipelineResourceDescriptor& source) -> xiiResult {
    for (const auto& reference : source.m_References)
    {
      xiiString existing;
      if (!reference.m_Id.IsValid() || reference.m_sResource.IsEmpty() || (references.TryGetValue(reference.m_Id, existing) && existing != reference.m_sResource))
        return fail("Invalid or conflicting graph definition resource reference.");
      references.Insert(reference.m_Id, reference.m_sResource);
    }
    return XII_SUCCESS;
  };
  XII_SUCCEED_OR_RETURN(addReferences(descriptor));
  xiiDynamicArray<DefinitionDependency> dependencies;
  xiiDynamicArray<xiiUuid>              queue;
  xiiHashSet<xiiUuid>                   visited;
  queue.PushBack(root.m_Id);
  for (xiiUInt32 next = 0; next < queue.GetCount(); ++next)
  {
    const auto id = queue[next];
    if (visited.Contains(id))
      continue;
    if (visited.GetCount() >= 4096)
      return fail("Pipeline exceeds definition dependency limit.");
    visited.Insert(id);
    xiiUInt32 index;
    if (!definitions.TryGetValue(id, index))
    {
      xiiString resourceName;
      if (!references.TryGetValue(id, resourceName))
        return fail("Missing referenced render pipeline definition.");
      if (resourceResolver.IsValid())
        resourceName = resourceResolver(resourceName);
      const auto                                 handle = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(resourceName);
      xiiResourceLock<xiiRenderPipelineResource> acquired(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      const auto                                 snapshot = acquired.IsValid() ? acquired->GetAuthoring() : xiiSharedPtr<const xiiRenderPipelineAuthoring>();
      if (!snapshot || snapshot->m_Descriptor.m_Id != id)
        return fail("Referenced pipeline is missing or its document identity does not match.");
      XII_SUCCEED_OR_RETURN(snapshot->m_Descriptor.m_Settings.Validate(out_error));
      // A reusable definition's device requirements constrain every containing pipeline.
      // Its final presentation contract applies only when it is itself the root.
      resolved.m_Settings.m_RequiredDeviceFeatures.Add(snapshot->m_Descriptor.m_Settings.m_RequiredDeviceFeatures);
      xiiGraphDefinition definition;
      XII_SUCCEED_OR_RETURN(snapshot->m_Descriptor.MakeGraphDefinition(definition));
      index = resolved.m_Definitions.GetCount();
      resolved.m_Definitions.PushBack(std::move(definition));
      definitions.Insert(id, index);
      for (const auto& embedded : snapshot->m_Descriptor.m_Definitions)
      {
        if (definitions.Contains(embedded.m_Id))
          return fail("Conflicting embedded graph definition identity; use shared asset references.");
        definitions.Insert(embedded.m_Id, resolved.m_Definitions.GetCount());
        resolved.m_Definitions.PushBack(embedded);
      }
      XII_SUCCEED_OR_RETURN(addReferences(snapshot->m_Descriptor));
      dependencies.PushBack({handle, snapshot});
    }
    const auto& definition = index == xiiInvalidIndex ? root : resolved.m_Definitions[index];
    for (const auto& node : definition.m_Nodes)
      if (node.m_Definition.IsValid())
        queue.PushBack(node.m_Definition);
  }
  out_resolved = std::move(resolved);
  if (out_dependencies)
    *out_dependencies = std::move(dependencies);
  return XII_SUCCESS;
}
xiiResourceLoadDescription xiiRenderPipelineResource::UnloadData(Unload)
{
  XII_LOCK(m_Mutex);
  m_pPipeline  = nullptr;
  m_pAuthoring = nullptr;
  m_DefinitionDependencies.Clear();
  return {xiiResourceState::Unloaded, 0, 0};
}
xiiResourceLoadDescription xiiRenderPipelineResource::UpdateContent(xiiStreamReader* stream)
{
  if (!stream)
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  // The standard file loader prefixes every file stream with its absolute source path.
  xiiString path;
  if (stream->ReadString(path).Failed())
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  if (path.HasExtension("xiiBinRenderPipeline"))
  {
    xiiAssetFileHeader header;
    xiiUInt8           version = 0;
    if (header.Read(*stream).Failed() || stream->ReadBytes(&version, sizeof(version)) != sizeof(version))
      return {xiiResourceState::LoadedResourceMissing, 0, 0};
    if (version != 3)
    {
      xiiLog::Error("Render pipeline '{}' uses obsolete compiled format {}. Re-transform its source asset.", path, version);
      return {xiiResourceState::LoadedResourceMissing, 0, 0};
    }
  }
  xiiRenderPipelineResourceDescriptor descriptor;
  if (descriptor.Deserialize(*stream).Failed())
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  return CreateResource(std::move(descriptor));
}
void xiiRenderPipelineResource::UpdateMemoryUsage(MemoryUsage& usage)
{
  XII_LOCK(m_Mutex);
  usage.m_uiMemoryCPU = sizeof(*this);
  usage.m_uiMemoryCPU += static_cast<xiiUInt32>(m_DefinitionDependencies.GetHeapMemoryUsage());
  if (m_pAuthoring)
    usage.m_uiMemoryCPU += static_cast<xiiUInt32>(sizeof(xiiRenderPipelineAuthoring) + m_pAuthoring->m_Descriptor.GetHeapMemoryUsage());
  usage.m_uiMemoryGPU = 0;
  if (m_pPipeline)
  {
    usage.m_uiMemoryCPU += static_cast<xiiUInt32>(sizeof(xiiRenderPipeline) + m_pPipeline->GetHeapMemoryUsage());
  }
}
XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiRenderPipelineResource, xiiRenderPipelineResourceDescriptor)
{
  xiiSharedPtr<const xiiRenderPipeline> pipeline;
  xiiStringBuilder                      error;
  xiiGraphDefinition                    definition;
  if (descriptor.m_Settings.Validate(&error).Failed() || descriptor.MakeGraphDefinition(definition).Failed() || (descriptor.m_References.IsEmpty() && xiiRenderPipeline::Compile(descriptor, pipeline, &error).Failed()))
  {
    xiiLog::Error("Render pipeline '{}': {}", GetResourceIdOrDescription(), error);
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  }
  XII_LOCK(m_Mutex);
  xiiSharedPtr<xiiRenderPipelineAuthoring> authoring = XII_DEFAULT_NEW(xiiRenderPipelineAuthoring);
  authoring->m_Descriptor                            = descriptor;
  m_pAuthoring                                       = std::move(authoring);
  m_DefinitionDependencies.Clear();
  m_pPipeline = std::move(pipeline);
  return {xiiResourceState::Loaded, 0, 0};
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineResource);
