#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/RendererCoreDLL.h>

struct XII_RENDERERCORE_DLL xiiBakingSettings
{
  xiiVec3   m_vProbeSpacing        = xiiVec3(4);
  xiiUInt32 m_uiNumSamplesPerProbe = 128;
  float     m_fMaxRayDistance      = 1000.0f;

  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiBakingSettings);

class xiiWorld;

class xiiBakingInterface
{
public:
  /// \brief Renders a debug view of the baking scene
  virtual xiiResult RenderDebugView(const xiiWorld& world, const xiiMat4& InverseViewProjection, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiDynamicArray<xiiColorGammaUB>& out_Pixels, xiiProgress& progress) const = 0;
};
