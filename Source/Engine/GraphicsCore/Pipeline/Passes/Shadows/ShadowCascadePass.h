/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/ShadowCascade.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Stabilized directional cascades from a setup-time view camera snapshot.
/// Owns neither light components nor shadow atlas residency/raster work.
class XII_GRAPHICSCORE_DLL xiiShadowCascadePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowCascadePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiShadowCascadeSettings         m_Settings;
  xiiVec3                          m_LightDirection = xiiVec3(0, 0, -1);

  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
  /// Derived CPU queries for sparse residency/invalidation during this graph setup only.
  /// No retained camera/world pointers; execution owns its own immutable GPU snapshot.
  xiiArrayPtr<const xiiShadowCascadeDescription> GetCascadeSnapshot(const xiiRenderGraph& graph) const;

private:
  xiiStaticArray<xiiShadowCascadeDescription, 4> m_Cascades;
  xiiUInt64                                      m_uiSetupSerial = 0;
};
