
#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// \brief This class can be used to define the render targets to be used by an xiiView.
struct XII_RENDERERFOUNDATION_DLL xiiGALRenderTargets
{
  bool operator==(const xiiGALRenderTargets& other) const;
  bool operator!=(const xiiGALRenderTargets& other) const;

  xiiGALTextureHandle m_hRTs[XII_GAL_MAX_RENDERTARGET_COUNT];
  xiiGALTextureHandle m_hDSTarget;
};

// \brief This class can be used to construct render target setups on the stack.
class XII_RENDERERFOUNDATION_DLL xiiGALRenderTargetSetup
{
public:
  xiiGALRenderTargetSetup();

  xiiGALRenderTargetSetup& SetRenderTarget(xiiUInt8 uiIndex, xiiGALRenderTargetViewHandle hRenderTarget);
  xiiGALRenderTargetSetup& SetDepthStencilTarget(xiiGALRenderTargetViewHandle hDSTarget);

  bool operator==(const xiiGALRenderTargetSetup& other) const;
  bool operator!=(const xiiGALRenderTargetSetup& other) const;

  inline xiiUInt8 GetRenderTargetCount() const;

  inline xiiGALRenderTargetViewHandle GetRenderTarget(xiiUInt8 uiIndex) const;
  inline xiiGALRenderTargetViewHandle GetDepthStencilTarget() const;

  void DestroyAllAttachedViews();

protected:
  xiiGALRenderTargetViewHandle m_hRTs[XII_GAL_MAX_RENDERTARGET_COUNT];
  xiiGALRenderTargetViewHandle m_hDSTarget;

  xiiUInt8 m_uiRTCount = 0;
};

struct XII_RENDERERFOUNDATION_DLL xiiGALRenderingSetup
{
  bool operator==(const xiiGALRenderingSetup& other) const;
  bool operator!=(const xiiGALRenderingSetup& other) const;

  xiiGALRenderTargetSetup m_RenderTargetSetup;
  xiiColor                m_ClearColor              = xiiColor(0, 0, 0, 0);
  xiiUInt32               m_uiRenderTargetClearMask = 0x0;
  float                   m_fDepthClear             = 1.0f;
  xiiUInt8                m_uiStencilClear          = 0;
  bool                    m_bClearDepth             = false;
  bool                    m_bClearStencil           = false;
  bool                    m_bDiscardColor           = false;
  bool                    m_bDiscardDepth           = false;
};

#include <RendererFoundation/Resources/Implementation/RenderTargetSetup_inl.h>
