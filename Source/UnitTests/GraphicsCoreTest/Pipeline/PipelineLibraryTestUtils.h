/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <TestFramework/Framework/TestFramework.h>

/// Test fixtures use the same authored resources as applications, never a C++ graph factory.
inline xiiRenderPipelineResourceHandle xiiLoadPipelineLibraryResource(xiiStringView name)
{
  if (!xiiFileSystem::FindDataDirectoryWithRoot("PipelineTestResources"))
    if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "PipelineTestResources", "PipelineTestResources", xiiDataDirUsage::ReadOnly)))
      return {};
  const xiiStringBuilder path(":PipelineTestResources/RenderPipelines/", name, ".xiiBinRenderPipeline");
  return xiiResourceManager::LoadResource<xiiRenderPipelineResource>(path);
}

/// Give mutation fixtures a readable topological order. This is an explicit remapping of
/// flattened document occurrence indices, not a reconstruction of the rendering stack.
inline xiiResult xiiLoadPipelineLibraryDescriptor(xiiRenderPipelineResourceDescriptor& out_definition, xiiStringView name)
{
  const auto                                 resource = xiiLoadPipelineLibraryResource(name);
  xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return XII_FAILURE;
  auto       definition = loaded->GetPipeline()->GetDescriptor();
  const auto order      = loaded->GetPipeline()->GetExecutionOrder();
  if (!XII_TEST_INT(order.GetCount(), definition.m_Passes.GetCount()))
    return XII_FAILURE;
  xiiDynamicArray<xiiRenderPipelinePassDescriptor> passes;
  xiiDynamicArray<xiiUInt32>                       remap;
  remap.SetCount(order.GetCount());
  for (const auto old : order)
  {
    remap[old] = passes.GetCount();
    passes.PushBack(definition.m_Passes[old]);
  }
  definition.m_Passes = std::move(passes);
  for (auto& connection : definition.m_Connections)
  {
    connection.m_uiSource = remap[connection.m_uiSource];
    connection.m_uiTarget = remap[connection.m_uiTarget];
  }
  out_definition = std::move(definition);
  return XII_SUCCESS;
}
