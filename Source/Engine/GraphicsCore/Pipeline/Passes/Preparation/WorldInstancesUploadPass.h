/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Publishes the world's incremental GPU instance snapshot. Repeated occurrences share
/// the upload within a graph; this pass never creates scene objects or copies a hierarchy.
class XII_GRAPHICSCORE_DLL xiiWorldInstancesUploadPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiWorldInstancesUploadPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin         m_Instances{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Skinning{false, xiiGALBindFlags::ShaderResource};
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Geometry; }
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::GpuScene; }
};
