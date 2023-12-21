#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/Progress.h>

struct XII_GRAPHICSCORE_DLL xiiBakingSettings
{
  xiiVec3   m_vProbeSpacing        = xiiVec3(4);
  xiiUInt32 m_uiNumSamplesPerProbe = 128;
  float     m_fMaxRayDistance      = 1000.0f;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiBakingSettings);

class xiiWorld;

class xiiBakingInterface
{
public:
  /// \brief Renders a debug view of the baking scene
  virtual xiiResult RenderDebugView(const xiiWorld& world, const xiiMat4& mInverseViewProjection, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiDynamicArray<xiiColorGammaUB>& out_pixels, xiiProgress& ref_progress) const = 0;
};
