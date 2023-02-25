#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

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

bool xiiGALRenderTargets::operator!=(const xiiGALRenderTargets& other) const
{
  return !(*this == other);
}

xiiGALRenderTargetSetup::xiiGALRenderTargetSetup()
{
}

xiiGALRenderTargetSetup& xiiGALRenderTargetSetup::SetRenderTarget(xiiUInt8 uiIndex, xiiGALRenderTargetViewHandle hRenderTarget)
{
  XII_ASSERT_DEV(uiIndex < XII_GAL_MAX_RENDERTARGET_COUNT, "Render target index out of bounds - should be less than XII_GAL_MAX_RENDERTARGET_COUNT");

  m_hRTs[uiIndex] = hRenderTarget;

  m_uiRTCount = xiiMath::Max(m_uiRTCount, static_cast<xiiUInt8>(uiIndex + 1u));

  return *this;
}

xiiGALRenderTargetSetup& xiiGALRenderTargetSetup::SetDepthStencilTarget(xiiGALRenderTargetViewHandle hDSTarget)
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

  xiiArrayPtr<xiiGALRenderTargetViewHandle> colorViews(m_hRTs);
  for (xiiGALRenderTargetViewHandle& hView : colorViews)
  {
    if (!hView.IsInvalidated())
    {
      pDevice->DestroyRenderTargetView(hView);
      hView.Invalidate();
    }
  }

  if (!m_hDSTarget.IsInvalidated())
  {
    pDevice->DestroyRenderTargetView(m_hDSTarget);
    m_hDSTarget.Invalidate();
  }
  m_uiRTCount = 0;
}

bool xiiGALRenderingSetup::operator==(const xiiGALRenderingSetup& other) const
{
  return m_RenderTargetSetup == other.m_RenderTargetSetup && m_ClearColor == other.m_ClearColor && m_uiRenderTargetClearMask == other.m_uiRenderTargetClearMask && m_fDepthClear == other.m_fDepthClear && m_uiStencilClear == other.m_uiStencilClear && m_bClearDepth == other.m_bClearDepth && m_bClearStencil == other.m_bClearStencil && m_bDiscardColor == other.m_bDiscardColor && m_bDiscardDepth == other.m_bDiscardDepth;
}

bool xiiGALRenderingSetup::operator!=(const xiiGALRenderingSetup& other) const
{
  return !(*this == other);
}


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RenderTargetSetup);
