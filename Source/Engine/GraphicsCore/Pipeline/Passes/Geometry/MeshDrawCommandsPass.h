/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Converts a bounded compact meshlet stream to one portable DrawMeshIndirect command.
/// Raster constants must use the same reflected MeshGroupCountX/Y strides.
class XII_GRAPHICSCORE_DLL xiiMeshDrawCommandsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshDrawCommandsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin             m_Visible{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Count{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin            m_Commands{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferOutputPin            m_CommandCount{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments};
  xiiUInt32                                   m_uiMeshGroupCountX = 65535;
  xiiUInt32                                   m_uiMeshGroupCountY = 1;
  bool                                        m_bAsyncCompute     = true;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::MeshShaders; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
