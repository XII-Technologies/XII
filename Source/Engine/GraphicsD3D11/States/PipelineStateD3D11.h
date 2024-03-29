#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSD3D11_DLL xiiGALPipelineStateD3D11 final : public xiiGALPipelineState
{
public:
  Diligent::IPipelineState* GetPipelineState() const;

  Diligent::IShaderResourceBinding* GetShaderResourceBinding() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateD3D11(const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::IPipelineState*         m_pPipelineState         = nullptr;
  Diligent::IShaderResourceBinding* m_pShaderResourceBinding = nullptr;
};


#include <GraphicsD3D11/States/Implementation/PipelineStateD3D11_inl.h>
