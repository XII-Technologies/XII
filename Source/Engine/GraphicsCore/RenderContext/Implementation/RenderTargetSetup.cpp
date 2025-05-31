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

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_RenderTargetSetup);
