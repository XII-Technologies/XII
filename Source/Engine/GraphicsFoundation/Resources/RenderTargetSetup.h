#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

// \brief This class can be used to define the render targets to be used by a xiiView.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRenderTargets
{
  bool operator==(const xiiGALRenderTargets& other) const;
  bool operator!=(const xiiGALRenderTargets& other) const;

  xiiGALTextureHandle m_hRTs[XII_GAL_MAX_RENDERTARGET_COUNT];
  xiiGALTextureHandle m_hDSTarget;
};

#include <GraphicsFoundation/Resources/Implementation/RenderTargetSetup_inl.h>
