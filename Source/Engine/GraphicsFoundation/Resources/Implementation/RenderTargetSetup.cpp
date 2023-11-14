#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/RenderTargetSetup.h>

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

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_RenderTargetSetup);
