/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Math/Vec4.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Status.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>

/// Immutable physical lookup images. Initialization submits once inside the active shared GPU frame.
/// Consumers retain this object and wait for m_Ready value 1 on every consuming queue.
class XII_GRAPHICSCORE_DLL xiiAtmosphereLookup final : public xiiRefCounted
{
public:
  static xiiStatus Create(const xiiAtmosphereSettings& settings, xiiSharedPtr<const xiiAtmosphereLookup>& out_lookup);
  bool             Matches(const xiiAtmosphereSettings& settings) const;

  xiiAtmosphereLookup(const xiiAtmosphereConstants& constants, xiiSharedPtr<xiiGALTexture> transmittance, xiiSharedPtr<xiiGALTexture> multiScatter, xiiSharedPtr<xiiGALFence> ready);

  const xiiAtmosphereConstants      m_Constants;
  const xiiSharedPtr<xiiGALTexture> m_Transmittance, m_MultiScatter;
  const xiiSharedPtr<xiiGALFence>   m_Ready;
};
