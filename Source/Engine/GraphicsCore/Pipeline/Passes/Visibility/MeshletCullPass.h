/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Compacts supplied meshlet work using LOD routing, primitive ranges, frustum and cone tests.
/// InitialCount/InitialOverflow must be zero. Consumers must handle VisibleMeshletOverflow.
class XII_GRAPHICSCORE_DLL xiiMeshletCullPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshletCullPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin  m_Scene{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin  m_View{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin  m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin  m_Meshlets{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin  m_Visible{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin  m_Count{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin  m_Lod{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin  m_Arguments{true, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferInputPin  m_InitialCount{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferInputPin  m_InitialOverflow{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin m_Output{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin m_OutputCount{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  xiiRenderPipelineBufferOutputPin m_Overflow{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  xiiUInt32                        m_uiMaxVisibleMeshlets = 1024U * 1024U;
  /// Reject an insufficient authored capacity before dispatch rather than publishing partial caster coverage.
  bool m_bRequireComplete = false;
  bool m_bAsyncCompute    = true;
  /// Retain pre-compaction frustum/cone decisions only for diagnostic views.
  bool                                        m_bVisualizeView = false;
  xiiRenderPipelineBufferOutputPin            m_DiagnosticMeshlets{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_DiagnosticCount{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiBitflags<xiiRenderExtractionCategory>    GetExtractionRequirements() const override { return m_bRequireComplete ? xiiRenderExtractionCategory::Geometry : xiiRenderExtractionCategory::None; }
  xiiBitflags<xiiRenderPipelineFeature>       GetRuntimeFeatures() const override { return m_bRequireComplete ? xiiRenderPipelineFeature::GpuScene : xiiRenderPipelineFeature::None; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_DiagnosticPipeline;
};
