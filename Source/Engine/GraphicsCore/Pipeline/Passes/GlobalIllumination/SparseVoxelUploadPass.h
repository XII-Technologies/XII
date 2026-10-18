/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
class XII_GRAPHICSCORE_DLL xiiSparseVoxelUploadPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSparseVoxelUploadPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin    m_Schedule{xiiGetStaticRTTI<xiiSparseVoxelFrameData>()};
  xiiRenderPipelineBufferOutputPin m_Pages{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin m_Levels{false, xiiGALBindFlags::ShaderResource}, m_Updates{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
