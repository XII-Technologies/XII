#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief Pass 25 — Atmosphere LUT Update.
///
/// Async Compute. Generates transmittance and multiple-scattering LUTs using
/// physically-based atmosphere parameters (Rayleigh + Mie scattering, ozone absorption).
/// Only re-dispatched when sun/view parameters exceed a change threshold — LUTs are
/// cached across frames to minimize compute cost.
class XII_GRAPHICSCORE_DLL xiiAtmosphereLUTPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereLUTPass, xiiRenderPipelinePass);

public:
  xiiAtmosphereLUTPass();
  virtual ~xiiAtmosphereLUTPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  // Atmosphere parameters
  float m_fRayleighScaleHeight  = 8.0f;   ///< km
  float m_fMieScaleHeight       = 1.2f;   ///< km
  float m_fMieAnisotropy        = 0.8f;   ///< Henyey-Greenstein g parameter
  float m_fPlanetRadius         = 6360.0f; ///< km
  float m_fAtmosphereRadius     = 6460.0f; ///< km

private:
  xiiSharedPtr<xiiGALTexture> m_pTransmittanceLUT; ///< 256x64 R16G16B16A16F — persistent across frames.
  xiiSharedPtr<xiiGALTexture> m_pMultiScatterLUT;  ///< 32x32  R16G16B16A16F — persistent across frames.

  // Previous-frame sun direction to detect when LUTs need recomputation.
  float m_fPrevSunDirX = 0.0f, m_fPrevSunDirY = 0.0f, m_fPrevSunDirZ = 0.0f;
};
