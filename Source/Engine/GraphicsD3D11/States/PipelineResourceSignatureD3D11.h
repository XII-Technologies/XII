#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSD3D11_DLL xiiGALPipelineResourceSignatureD3D11 final : public xiiGALPipelineResourceSignature
{
public:
  virtual bool IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const override final;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureD3D11();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:

};

#include <GraphicsD3D11/States/Implementation/PipelineResourceSignatureD3D11_inl.h>
