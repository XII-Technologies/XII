/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/Geometry/MeshDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CounterResetPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletOcclusionPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Visibility/GpuVisibilityResources.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

/// Isolated test composition around the ordinary matrix meshlet asset. No scene state,
/// context registry, shader implementation or legacy visibility lifecycle is retained.
struct xiiTestNativeVisibility
{
  xiiRenderPipelineInstance m_Visibility;
  xiiCounterResetPass       m_Reset;
  xiiMeshletOcclusionPass   m_Occlusion;
  xiiMeshDrawCommandsPass   m_Commands;
  xiiUInt32                 m_uiCapacity = 1024;

  xiiResult Initialize(xiiUInt32 capacity = 1024)
  {
    m_uiCapacity                                        = capacity;
    const auto                                 resource = xiiLoadPipelineLibraryResource("MatrixMeshletVisibility");
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded.IsValid() || !loaded->GetAuthoring())
      return XII_FAILURE;
    auto definition = loaded->GetAuthoring()->m_Descriptor;
    for (auto& pass : definition.m_Passes)
      if (pass.m_sType == "xiiMeshletCullPass")
        pass.m_Properties.Insert("MaxVisibleMeshlets", capacity);
    xiiStringBuilder name;
    name.SetFormat("Native visibility fixture {}", capacity);
    const auto configured = xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(name, std::move(definition));
    return m_Visibility.Synchronize(configured);
  }

  xiiResult Build(xiiView* view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard,
                  const xiiMat4& projection, const xiiVec3& position, bool meshCommands,
                  xiiGpuVisibilityOutputs& result, xiiUInt32 excluded = 0,
                  xiiUInt32 required = xiiGpuInstanceFlags::Enabled, xiiRenderGraphTextureHandle depth = {})
  {
    result = {};
    xiiVariantDictionary parameters;
    parameters.Insert("CullingViewProjection", projection);
    parameters.Insert("CullingCameraPosition", position);
    parameters.Insert("CullingWidth", view->GetRenderResolutionWidth());
    parameters.Insert("CullingHeight", view->GetRenderResolutionHeight());
    parameters.Insert("CullingRequiredFlags", required);
    parameters.Insert("CullingExcludedFlags", excluded);
    if (m_Visibility.Build(view, graph, blackboard, parameters).Failed())
    {
      xiiTestFramework::Output(xiiTestOutput::Details, "Native visibility fixture: %s", xiiString(m_Visibility.GetLastBuildError()).GetData());
      return XII_FAILURE;
    }
    const auto buffer = [&](xiiStringView pin) {
      const auto* binding = m_Visibility.GetInterfaceBinding(m_Visibility.GetPipeline()->FindInterface(pin, false));
      XII_TEST_BOOL(binding != nullptr);
      return binding ? binding->m_hBuffer : xiiRenderGraphBufferHandle{};
    };
    result.m_hSceneInstances       = buffer("SceneInstances");
    result.m_hSkinningMatrices     = buffer("SkinningMatrices");
    result.m_hVisibleInstances     = buffer("VisibleInstances");
    result.m_hVisibleInstanceCount = buffer("VisibleInstanceCount");
    result.m_hVisibleMeshlets      = buffer("VisibleMeshlets");
    result.m_hVisibleMeshletCount  = buffer("VisibleMeshletCount");
    result.m_uiMaxVisibleMeshlets  = m_uiCapacity;
    // Native tail fixtures bind concrete members. RTTI supplies each schema.
    const auto build = [&](auto& pass, xiiStringView name, auto bind) -> xiiResult {
      xiiDynamicArray<xiiRenderPipelinePin> pins;
      pass.GetPins(pins);
      XII_SUCCEED_OR_RETURN(pass.InitializePinBindings(pins));
      xiiDynamicArray<xiiRenderPipelineBinding> bindings;
      bindings.SetCount(pins.GetCount());
      bind(bindings);
      xiiRenderPipelinePassContext context{view, graph, blackboard, parameters, pins, bindings, name};
      XII_SUCCEED_OR_RETURN(pass.Build(context));
      bind(bindings);
      return XII_SUCCESS;
    };
    if (depth.IsValid())
    {
      xiiRenderGraphBufferHandle counter;
      XII_SUCCEED_OR_RETURN(build(m_Reset, "Fixture Occlusion Counter", [&](auto& bindings) {
        counter = bindings[m_Reset.m_Counter.GetBindingIndex()].m_hBuffer;
      }));
      bool inputs = true;
      XII_SUCCEED_OR_RETURN(build(m_Occlusion, "Fixture Explicit Meshlet Occlusion", [&](auto& bindings) {
        if (inputs)
        {
          bindings[m_Occlusion.m_Instances.GetBindingIndex()].m_hBuffer      = result.m_hSceneInstances;
          bindings[m_Occlusion.m_Meshlets.GetBindingIndex()].m_hBuffer       = buffer("MeshletMetadata");
          bindings[m_Occlusion.m_View.GetBindingIndex()].m_hBuffer           = buffer("VisibilityView");
          bindings[m_Occlusion.m_Candidates.GetBindingIndex()].m_hBuffer     = result.m_hVisibleMeshlets;
          bindings[m_Occlusion.m_CandidateCount.GetBindingIndex()].m_hBuffer = result.m_hVisibleMeshletCount;
          bindings[m_Occlusion.m_InitialCount.GetBindingIndex()].m_hBuffer   = counter;
          bindings[m_Occlusion.m_Depth.GetBindingIndex()].m_hTexture         = depth;
          inputs                                                             = false;
        }
        else
        {
          result.m_hVisibleMeshlets     = bindings[m_Occlusion.m_Visible.GetBindingIndex()].m_hBuffer;
          result.m_hVisibleMeshletCount = bindings[m_Occlusion.m_Count.GetBindingIndex()].m_hBuffer;
        }
      }));
    }
    if (meshCommands)
    {
      bool inputs = true;
      XII_SUCCEED_OR_RETURN(build(m_Commands, "Fixture Mesh Commands", [&](auto& bindings) {
        if (inputs)
        {
          bindings[m_Commands.m_Visible.GetBindingIndex()].m_hBuffer = result.m_hVisibleMeshlets;
          bindings[m_Commands.m_Count.GetBindingIndex()].m_hBuffer   = result.m_hVisibleMeshletCount;
          inputs                                                     = false;
        }
        else
        {
          result.m_hIndirectCommands     = bindings[m_Commands.m_Commands.GetBindingIndex()].m_hBuffer;
          result.m_hIndirectCommandCount = bindings[m_Commands.m_CommandCount.GetBindingIndex()].m_hBuffer;
        }
      }));
    }
    return XII_SUCCESS;
  }
};
