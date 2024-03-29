#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSD3D11_DLL xiiGALPipelineResourceSignatureD3D11 final : public xiiGALPipelineResourceSignature
{
public:
  virtual bool IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const override final;

  Diligent::IPipelineResourceSignature* GetPipelineResourceSignature() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureD3D11(const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  xiiHybridArray<Diligent::ImmutableSamplerDesc, 2U> m_ImmutableSamplers;
  xiiHybridArray<Diligent::PipelineResourceDesc, 2U> m_PipelineResourceDescriptions;

  Diligent::IPipelineResourceSignature* m_pPipelineResourceSignature = nullptr;
};

#include <GraphicsD3D11/States/Implementation/PipelineResourceSignatureD3D11_inl.h>
