/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/RasterizerState.h>

/// Queries nearest surfaces from supplied canonical GPU visibility and residency buffers.
class XII_GRAPHICSCORE_DLL xiiSurfaceQueryPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSurfaceQueryPass, xiiRenderPipelinePass);

public:
  bool                                        m_bOnlyViewWireframe = false;
  bool                                        m_bUseMeshShaders    = true;
  xiiEnum<xiiGALFillMode>                     m_FillMode           = xiiGALFillMode::Solid;
  void                                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources; }
};
