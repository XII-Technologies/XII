#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/RenderContext/RenderTargetSetup.h>

bool xiiGALRenderTargets::operator==(const xiiGALRenderTargets& other) const
{
  if (m_pDSTarget != other.m_pDSTarget)
    return false;

  for (xiiUInt8 uiRTIndex = 0; uiRTIndex < XII_GAL_MAX_RENDERTARGET_COUNT; ++uiRTIndex)
  {
    if (m_pRTs[uiRTIndex] != other.m_pRTs[uiRTIndex])
      return false;
  }
  return true;
}

xiiGALRenderTargetSetup::xiiGALRenderTargetSetup() = default;

xiiGALRenderTargetSetup& xiiGALRenderTargetSetup::SetRenderTarget(xiiUInt8 uiIndex, xiiSharedPtr<xiiGALTextureView> pRenderTarget)
{
  XII_ASSERT_DEV(uiIndex < XII_GAL_MAX_RENDERTARGET_COUNT, "Render target index out of bounds - this should be less than XII_GAL_MAX_RENDERTARGET_COUNT.");

  m_pRTs[uiIndex] = pRenderTarget;

  m_uiRTCount = xiiMath::Max(m_uiRTCount, static_cast<xiiUInt8>(uiIndex + 1U));

  return *this;
}

xiiGALRenderTargetSetup& xiiGALRenderTargetSetup::SetDepthStencilTarget(xiiSharedPtr<xiiGALTextureView> pDSTarget)
{
  m_pDSTarget = pDSTarget;

  return *this;
}

bool xiiGALRenderTargetSetup::operator==(const xiiGALRenderTargetSetup& other) const
{
  if (m_pDSTarget != other.m_pDSTarget)
    return false;

  if (m_uiRTCount != other.m_uiRTCount)
    return false;

  for (xiiUInt8 uiRTIndex = 0; uiRTIndex < m_uiRTCount; ++uiRTIndex)
  {
    if (m_pRTs[uiRTIndex] != other.m_pRTs[uiRTIndex])
      return false;
  }

  return true;
}

void xiiGALRenderTargetSetup::DestroyAllAttachedViews()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiArrayPtr<xiiSharedPtr<xiiGALTextureView>> colorViews(m_pRTs);
  for (xiiSharedPtr<xiiGALTextureView>& pView : colorViews)
  {
    pView.Clear();
  }

  m_pDSTarget.Clear();

  m_uiRTCount = 0;
}

bool xiiGALRenderingSetup::operator==(const xiiGALRenderingSetup& other) const
{
  return m_RenderTargetSetup == other.m_RenderTargetSetup && m_uiRenderTargetClearMask == other.m_uiRenderTargetClearMask && m_bClearDepth == other.m_bClearDepth && m_bClearStencil == other.m_bClearStencil && m_bDiscardColor == other.m_bDiscardColor && m_bDiscardDepth == other.m_bDiscardDepth;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_RenderTargetSetup);
