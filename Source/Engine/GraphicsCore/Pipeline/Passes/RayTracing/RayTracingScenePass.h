/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Builds an occurrence-local TLAS from world-derived GPU instances. Device BLAS
/// residency and world deformation caches remain shared through existing services.
class XII_GRAPHICSCORE_DLL xiiRayTracingScenePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRayTracingScenePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineAccelerationStructureOutputPin m_OutputScene{true};
  xiiRenderPipelineBufferOutputPin                m_OutputMaterials{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin                m_OutputGeometry{true, xiiGALBindFlags::ShaderResource};
  /// Optional GPU emissive proposal tables built from this occurrence's exact hit geometry.
  xiiRenderPipelineBufferOutputPin m_EmissiveTriangles{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_EmissiveHeader{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_TriangleAliases{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_BlockAliases{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_AliasHeader{true, xiiGALBindFlags::ShaderResource};
  bool                             m_bBuildEmissiveLights = false;
  xiiRenderPipelineTextureInputPin m_Activation{xiiGALResourceFormat::Unknown, true};
  /// When enabled, absence of this token skips all occurrence-local TLAS work.
  bool m_bRequireActivation         = false;
  bool m_bUseViewSelection          = true;
  bool m_bRequireHardwareRayTracing = false;
  /// Skip TLAS construction outside explicit ray-tracing diagnostic view modes.
  bool m_bOnlyDiagnosticViews = false;
  struct BuildStatistics
  {
    xiiUInt32 m_uiInstanceCount = 0, m_uiDeformationDispatchCount = 0, m_uiDynamicBLASBuildCount = 0;
  };
  const BuildStatistics& GetLastBuildStatistics() const { return m_Statistics; }
  xiiRayTracingScenePass();
  ~xiiRayTracingScenePass();
  xiiBitflags<xiiRenderExtractionCategory>    GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Geometry; }
  xiiBitflags<xiiRenderPipelineFeature>       GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::GpuScene | xiiRenderPipelineFeature::RayTracing; }
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bRequireHardwareRayTracing ? xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::RayTracing | xiiRenderPipelineDeviceFeature::BindlessResources : xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  BuildStatistics m_Statistics;
  struct State;
  xiiUniquePtr<State> m_pState;
};
