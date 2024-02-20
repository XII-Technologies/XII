#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

namespace Diligent
{
  struct IPipelineResourceSignature;
  struct PipelineResourceDesc;
  struct ImmutableSamplerDesc;
} // namespace Diligent

class XII_GRAPHICSD3D12_DLL xiiGALPipelineResourceSignatureD3D12 final : public xiiGALPipelineResourceSignature
{
public:
  virtual bool IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const override final;

  Diligent::IPipelineResourceSignature* GetPipelineResourceSignature() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureD3D12(const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  xiiHybridArray<Diligent::ImmutableSamplerDesc, 2U> m_ImmutableSamplers;
  xiiHybridArray<Diligent::PipelineResourceDesc, 2U> m_PipelineResourceDescriptions;

  Diligent::IPipelineResourceSignature* m_pPipelineResourceSignature = nullptr;
};

#include <GraphicsD3D12/States/Implementation/PipelineResourceSignatureD3D12_inl.h>
