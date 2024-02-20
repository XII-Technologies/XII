#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

namespace Diligent
{
  struct IPipelineState;
  struct IShaderResourceBinding;
} // namespace Diligent

class XII_GRAPHICSD3D12_DLL xiiGALPipelineStateD3D12 final : public xiiGALPipelineState
{
public:
  Diligent::IPipelineState* GetPipelineState() const;

  Diligent::IShaderResourceBinding* GetShaderResourceBinding() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateD3D12(const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::IPipelineState*         m_pPipelineState         = nullptr;
  Diligent::IShaderResourceBinding* m_pShaderResourceBinding = nullptr;
};


#include <GraphicsD3D12/States/Implementation/PipelineStateD3D12_inl.h>
