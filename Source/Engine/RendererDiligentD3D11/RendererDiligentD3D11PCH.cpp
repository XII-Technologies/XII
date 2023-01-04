#include <RendererDiligentD3D11/RendererDiligentD3D11PCH.h>

XII_STATICLINK_LIBRARY(RendererDiligentD3D11)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(RendererDiligent_Context_Implementation_ContextDiligentD3D11);
  XII_STATICLINK_REFERENCE(RendererDiligent_Device_Implementation_DeviceDiligentD3D11);
  XII_STATICLINK_REFERENCE(RendererDiligent_Device_Implementation_SwapChainDiligentD3D11);
}
