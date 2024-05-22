#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/RenderContext/RenderTargetSetup.h>
#include <GraphicsFoundation/Device/Device.h>

bool xiiGALRenderTargets::operator==(const xiiGALRenderTargets& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  for (xiiUInt8 uiRTIndex = 0; uiRTIndex < XII_GAL_MAX_RENDERTARGET_COUNT; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }
  return true;
}

xiiGALRenderTargetSetup::xiiGALRenderTargetSetup() = default;

xiiGALRenderTargetSetup& xiiGALRenderTargetSetup::SetRenderTarget(xiiUInt8 uiIndex, xiiGALTextureViewHandle hRenderTarget)
{
  XII_ASSERT_DEV(uiIndex < XII_GAL_MAX_RENDERTARGET_COUNT, "Render target index out of bounds - this should be less than XII_GAL_MAX_RENDERTARGET_COUNT.");

  m_hRTs[uiIndex] = hRenderTarget;

  m_uiRTCount = xiiMath::Max(m_uiRTCount, static_cast<xiiUInt8>(uiIndex + 1U));

  return *this;
}

xiiGALRenderTargetSetup& xiiGALRenderTargetSetup::SetDepthStencilTarget(xiiGALTextureViewHandle hDSTarget)
{
  m_hDSTarget = hDSTarget;

  return *this;
}

bool xiiGALRenderTargetSetup::operator==(const xiiGALRenderTargetSetup& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  if (m_uiRTCount != other.m_uiRTCount)
    return false;

  for (xiiUInt8 uiRTIndex = 0; uiRTIndex < m_uiRTCount; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }

  return true;
}

bool xiiGALRenderTargetSetup::operator!=(const xiiGALRenderTargetSetup& other) const
{
  return !(*this == other);
}

void xiiGALRenderTargetSetup::DestroyAllAttachedViews()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiArrayPtr<xiiGALTextureViewHandle> colorViews(m_hRTs);
  for (xiiGALTextureViewHandle& hView : colorViews)
  {
    if (!hView.IsInvalidated())
    {
      pDevice->DestroyTextureView(hView);
      hView.Invalidate();
    }
  }

  if (!m_hDSTarget.IsInvalidated())
  {
    pDevice->DestroyTextureView(m_hDSTarget);
    m_hDSTarget.Invalidate();
  }
  m_uiRTCount = 0;
}

bool xiiGALRenderingSetup::operator==(const xiiGALRenderingSetup& other) const
{
  return m_RenderTargetSetup == other.m_RenderTargetSetup && m_uiRenderTargetClearMask == other.m_uiRenderTargetClearMask && m_bClearDepth == other.m_bClearDepth && m_bClearStencil == other.m_bClearStencil && m_bDiscardColor == other.m_bDiscardColor && m_bDiscardDepth == other.m_bDiscardDepth;
}

bool xiiGALRenderingSetup::operator!=(const xiiGALRenderingSetup& other) const
{
  return !(*this == other);
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_RenderTargetSetup);
