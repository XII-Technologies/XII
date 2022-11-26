
#pragma once

xiiUInt8 xiiGALRenderTargetSetup::GetRenderTargetCount() const
{
  return m_uiRTCount;
}

xiiGALRenderTargetViewHandle xiiGALRenderTargetSetup::GetRenderTarget(xiiUInt8 uiIndex) const
{
  XII_ASSERT_DEBUG(uiIndex < m_uiRTCount, "Render target index out of range");

  return m_hRTs[uiIndex];
}

xiiGALRenderTargetViewHandle xiiGALRenderTargetSetup::GetDepthStencilTarget() const
{
  return m_hDSTarget;
}
