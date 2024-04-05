#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/States/PipelineResourceSignatureD3D11.h>

xiiGALPipelineResourceSignatureD3D11::xiiGALPipelineResourceSignatureD3D11(const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALPipelineResourceSignature(creationDescription)
{
}

xiiGALPipelineResourceSignatureD3D11::~xiiGALPipelineResourceSignatureD3D11() = default;

xiiResult xiiGALPipelineResourceSignatureD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALPipelineResourceSignatureD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

bool xiiGALPipelineResourceSignatureD3D11::IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const
{
  const xiiGALPipelineResourceSignatureD3D11* pPipelineResourceSignatureD3D11 = static_cast<const xiiGALPipelineResourceSignatureD3D11*>(pPipelineResourceSignature);

  ///\todo GraphicsD3D11: Implement pipeline resource signature compatibility.
  return false;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_States_Implementation_PipelineResourceSignatureD3D11);
