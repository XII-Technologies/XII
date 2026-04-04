#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief Pass 25 â€” Atmosphere LUT Update.
///
/// Async Compute. Generates transmittance and multiple-scattering LUTs using
/// physically-based atmosphere parameters (Rayleigh + Mie scattering, ozone absorption).
/// Only re-dispatched when sun/view parameters exceed a change threshold â€” LUTs are
/// cached across frames to minimize compute cost.
struct XII_GRAPHICSCORE_DLL xiiAtmosphereLUTPass
{
  // Atmosphere parameters
  float m_fRayleighScaleHeight  = 8.0f;   ///< km
  float m_fMieScaleHeight       = 1.2f;   ///< km
  float m_fMieAnisotropy        = 0.8f;   ///< Henyey-Greenstein g parameter
  float m_fPlanetRadius         = 6360.0f; ///< km
  float m_fAtmosphereRadius     = 6460.0f; ///< km
  xiiSharedPtr<xiiGALTexture> m_pTransmittanceLUT; ///< 256x64 R16G16B16A16F â€” persistent across frames.
  xiiSharedPtr<xiiGALTexture> m_pMultiScatterLUT;  ///< 32x32  R16G16B16A16F â€” persistent across frames.

  // Previous-frame sun direction to detect when LUTs need recomputation.
  float m_fPrevSunDirX = 0.0f, m_fPrevSunDirY = 0.0f, m_fPrevSunDirZ = 0.0f;
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "AtmosphereLUTPass";
  bool      m_bActive = true;
};

void xiiPopulateAtmosphereLUTPass(xiiAtmosphereLUTPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

