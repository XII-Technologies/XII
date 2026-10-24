/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/Rendering/EditorGizmoPass.h>
#include <EditorEngineProcessFramework/Rendering/InfiniteGridPass.h>
#include <EditorEngineProcessFramework/Rendering/ScenePreviewPipeline.h>
#include <GraphicsCore/Pipeline/Passes/Output/PresentPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/DebugOverlayPass.h>

xiiRenderPipelineResourceHandle xiiScenePreviewPipeline::GetResource(const xiiRenderPipelineResourceHandle& scene)
{
  if (!scene.IsValid())
    return scene;
  xiiResourceLock<xiiRenderPipelineResource> resource(scene, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (resource.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return scene;
  const auto authoring = resource->GetAuthoring();
  const auto compiled  = resource->GetPipeline();
  if (!authoring || !compiled)
    return scene;
  // Specialized pipelines without these interfaces retain their own presentation and behavior.
  if (compiled->FindInterface("GradedColor", false) == xiiInvalidIndex || compiled->FindInterface("SceneDepth", false) == xiiInvalidIndex)
    return scene;
  xiiStringBuilder identity;
  identity.SetFormat("XII.Internal.ScenePreview/{}/{}", scene.GetResourceID(), resource->GetCurrentResourceChangeCounter());
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.m_References.PushBack({authoring->m_Descriptor.m_Id, scene.GetResourceID()});
  const auto child = descriptor.AddSubgraph("Scene", authoring->m_Descriptor.m_Id);
  // Disable presentation at its persistent instance path, including nested renderers.
  // The original resource and other consumers retain their presentation passes.
  const auto& passes     = compiled->GetDescriptor().m_Passes;
  const auto  provenance = compiled->GetProvenance();
  for (xiiUInt32 i = 0; i < passes.GetCount(); ++i)
    if (passes[i].m_pType->IsDerivedFrom<xiiPresentPass>())
    {
      auto& override  = descriptor.m_Passes[child].m_Overrides.ExpandAndGetRef();
      override.m_Path = provenance[i].m_InstancePath;
      override.m_Path.PushBack(provenance[i].m_Element);
      override.m_Properties.Insert("Enabled", false);
    }
  const auto grid    = descriptor.AddPass("Grid", xiiGetStaticRTTI<xiiInfiniteGridPass>());
  const auto gizmos  = descriptor.AddPass("Gizmos", xiiGetStaticRTTI<xiiEditorGizmoPass>());
  const auto debug   = descriptor.AddPass("Debug overlays", xiiGetStaticRTTI<xiiDebugOverlayPass>());
  const auto present = descriptor.AddPass("Present", xiiGetStaticRTTI<xiiPresentPass>());
  descriptor.Connect(child, "GradedColor", grid, "Color").AssertSuccess();
  descriptor.Connect(grid, "Color", gizmos, "Color").AssertSuccess();
  descriptor.Connect(gizmos, "Color", debug, "DisplayLinearColor").AssertSuccess();
  descriptor.Connect(debug, "DisplayLinearColor", present, "GradedColor").AssertSuccess();
  for (auto pass : {grid, gizmos, debug})
    descriptor.Connect(child, "SceneDepth", pass, "SceneDepth").AssertSuccess();
  descriptor.m_Interface.PushBack({"ViewTarget", true, descriptor.m_Passes[present].m_Id, "Target"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(descriptor));
}
