/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/DebugOverlayPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/ViewRenderModePass.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>

XII_CREATE_SIMPLE_TEST(Pipeline, IndependentVisualizationComposition)
{
  xiiViewRenderModePass visualization;
  visualization.m_Mode = xiiViewRenderMode::BaseColor;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(visualization.GetProperties(properties));
  xiiViewRenderModePass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_BOOL(copy.m_Mode == xiiViewRenderMode::BaseColor);
  properties.Insert("Mode", static_cast<xiiInt64>(xiiViewRenderMode::Overdraw));
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  XII_TEST_BOOL(copy.GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(copy.GetRuntimeFeatures().IsNoFlagSet());
  xiiDebugOverlayPass overlay;
  XII_TEST_BOOL(overlay.GetExtractionRequirements() == xiiRenderExtractionCategory::Custom);
  overlay.m_bEnabled = false;
  XII_TEST_BOOL(overlay.GetExtractionRequirements().IsNoFlagSet());
  xiiSurfaceQueryPass surface;
  surface.m_bUseMeshShaders = false;
  surface.m_FillMode        = xiiGALFillMode::Wireframe;
  XII_TEST_RESULT(surface.GetProperties(properties));
  xiiSurfaceQueryPass surfaceCopy;
  XII_TEST_RESULT(surfaceCopy.Configure(properties));
  XII_TEST_BOOL(!surfaceCopy.m_bUseMeshShaders && surfaceCopy.m_FillMode == xiiGALFillMode::Wireframe);

  xiiGALMeshShaderProperties limits;
  limits.m_uiMaxThreadGroupCountX     = 5;
  limits.m_uiMaxThreadGroupCountY     = 3;
  limits.m_uiMaxThreadGroupCountZ     = 1;
  limits.m_uiMaxThreadGroupTotalCount = 8;
  xiiUInt32 width = 99, height = 99;
  XII_TEST_RESULT(xiiGpuVisibilitySystem::ComputeMeshDispatchShape(limits, 7, width, height));
  XII_TEST_INT(width, 4);
  XII_TEST_INT(height, 2);
  XII_TEST_BOOL(xiiGpuVisibilitySystem::ComputeMeshDispatchShape(limits, 9, width, height).Failed());
  XII_TEST_INT(width, 0);
  XII_TEST_INT(height, 0);
  limits.m_uiMaxThreadGroupCountY = 1;
  XII_TEST_BOOL(xiiGpuVisibilitySystem::ComputeMeshDispatchShape(limits, 7, width, height).Failed());

  const auto                                 resource = xiiLoadPipelineLibraryResource("DebugVisualization");
  xiiResourceLock<xiiRenderPipelineResource> child(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(child.IsValid() && child->GetPipeline()))
    return;
  XII_TEST_INT(child->GetPipeline()->GetDescriptor().m_Passes.GetCount(), 2);
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({child->GetAuthoring()->m_Descriptor.m_Id, resource.GetResourceID()});
  root.AddSubgraph("First", child->GetAuthoring()->m_Descriptor.m_Id);
  root.AddSubgraph("Second", child->GetAuthoring()->m_Descriptor.m_Id);
  for (const auto& input : child->GetAuthoring()->m_Descriptor.m_Interface)
    if (input.m_bInput)
      for (const auto& occurrence : root.m_Passes)
      {
        bool exists = false;
        for (const auto& boundary : root.m_Interface)
          exists |= boundary.m_sName == input.m_sName && boundary.m_Node == occurrence.m_Id;
        if (!exists)
          root.m_Interface.PushBack({input.m_sName, true, occurrence.m_Id, input.m_sName});
      }
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (!XII_TEST_BOOL(compiled != nullptr))
    return;
  XII_TEST_INT(compiled->GetDescriptor().m_Passes.GetCount(), 4);
  XII_TEST_BOOL(compiled->GetExtractionRequirements() == xiiRenderExtractionCategory::Custom);
  XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet());
  XII_TEST_BOOL(compiled->GetProvenance()[0].m_InstancePath[0] != compiled->GetProvenance()[2].m_InstancePath[0]);
}
