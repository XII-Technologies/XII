#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

/// \brief This class can be used to define the render targets to be used by a xiiView.
struct XII_GRAPHICSCORE_DLL xiiGALRenderTargets
{
  bool operator==(const xiiGALRenderTargets& other) const;

  xiiSharedPtr<xiiGALTextureView> m_pRTs[XII_GAL_MAX_RENDERTARGET_COUNT];
  xiiSharedPtr<xiiGALTextureView> m_pDSTarget;
};

#include <GraphicsCore/RenderContext/Implementation/RenderTargetSetup_inl.h>
