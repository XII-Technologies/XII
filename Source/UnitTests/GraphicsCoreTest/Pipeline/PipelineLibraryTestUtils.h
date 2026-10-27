/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineTestFixtures.h>
#include <TestFramework/Framework/TestFramework.h>

inline xiiString xiiPipelineLibraryTestIdentity(xiiStringView name)
{
  if (name == "Scene")
    return xiiRenderPipelineProfileConfig::s_szScenePipeline;
  if (name == "Editor")
    return xiiRenderPipelineProfileConfig::s_szEditorPipeline;
  xiiStringBuilder identity;
  xiiConversionUtils::ToString(xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name)), identity);
  return identity;
}

/// End-to-end tests use the same canonical authored resources as applications.
inline xiiRenderPipelineResourceHandle xiiLoadPipelineLibraryResource(xiiStringView name)
{
  // RendererAssets exports these authored sources through the production compiler.
  // No generated pipeline bytes are checked into the SDK data directory.
  for (const xiiStringView canonical : {"SceneShadowCascade", "SceneDirectionalShadows", "SceneSpotShadows", "SceneAtmosphere", "SceneScreenSpaceReflections", "SceneTransparency", "SceneParticles", "SceneScreenSpaceIrradiance", "Scene", "Editor"})
  {
    xiiStringBuilder file(xiiFileSystem::GetSdkRootDirectory(), "/Data/Base/AssetCache/Common/RenderPipelines/", canonical, ".xiiBinRenderPipeline");
    file.MakeCleanPath();
    if (!XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(file), "Run EditorFrameworkTest -filter RendererAssets before GPU rendering tests."))
      return {};
    xiiResourceManager::RegisterNamedResource(xiiPipelineLibraryTestIdentity(canonical), file);
  }
  return xiiResourceManager::LoadResource<xiiRenderPipelineResource>(xiiPipelineLibraryTestIdentity(name));
}

/// Resolve a leaf by its reflected operation rather than a retired asset hierarchy.
inline xiiUuid xiiFindPipelineTestPass(const xiiRenderPipelineResourceDescriptor& descriptor, xiiStringView type, xiiUInt32 occurrence = 0)
{
  for (const auto& pass : descriptor.m_Passes)
    if (pass.m_pType && pass.m_pType->GetTypeName() == type && occurrence-- == 0)
      return pass.m_Id;
  XII_TEST_BOOL_MSG(false, "Missing test operation '%s'", type.GetStartPointer());
  return {};
}

/// A loaded memory-backed resource exercises reloads; CreateResource is intentionally non-reloadable.
inline xiiRenderPipelineResourceHandle xiiMakeReloadablePipelineTestResource(const xiiRenderPipelineResourceHandle& source, xiiStringView identity)
{
  xiiResourceLock<xiiRenderPipelineResource> definition(source, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(definition.IsValid() && definition->GetAuthoring()))
    return {};
  xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
  xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
  stream << xiiString("Fixture.xiiBinRenderPipeline");
  xiiAssetFileHeader header;
  if (!XII_TEST_RESULT(header.Write(stream)))
    return {};
  stream << xiiUInt8(3);
  if (!XII_TEST_RESULT(definition->GetAuthoring()->m_Descriptor.Serialize(stream)))
    return {};
  auto resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(identity);
  xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(loader));
  xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring());
  return resource;
}

/// Give mutation fixtures a readable topological order. This is an explicit remapping of
/// flattened document occurrence indices, not a reconstruction of the rendering stack.
inline xiiResult xiiLoadPipelineTestDescriptor(xiiRenderPipelineResourceDescriptor& out_definition, const xiiRenderPipelineResourceHandle& resource)
{
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
