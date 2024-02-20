#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Shader/ShaderResourceVariable.h>

namespace Diligent
{
  struct IShaderResourceVariable;
}

class XII_GRAPHICSD3D12_DLL xiiGALShaderResourceVariableD3D12 final : public xiiGALShaderResourceVariable
{
public:
  virtual void Set(xiiGALResource* pResource, xiiBitflags<xiiGALSetShaderResourceFlags> flags = xiiGALSetShaderResourceFlags::None) override final;

  virtual void SetArray(xiiArrayPtr<xiiGALResource* const> ppResources, xiiBitflags<xiiGALSetShaderResourceFlags> flags = xiiGALSetShaderResourceFlags::None) override final;

  virtual void SetBufferRange(xiiGALResource* pResource, xiiUInt64 uiOffset, xiiUInt64 uiSize, xiiUInt32 uiArrayIndex = 0U, xiiBitflags<xiiGALSetShaderResourceFlags> flags = xiiGALSetShaderResourceFlags::None) override final;

  virtual void SetBufferOffset(xiiUInt32 uiOffset, xiiUInt32 uiArrayIndex = 0U) override final;

  virtual xiiEnum<xiiGALShaderResourceVariableType> GetType() const override final;

  virtual void GetResourceDescription(xiiGALShaderResourceDescription& resourceDeccription) override final;

  virtual xiiUInt32 GetIndex() const override final;

  virtual xiiArrayPtr<xiiGALResource*> Get(xiiUInt32 uiIndex = 0U) const override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALShaderResourceVariableD3D12();

  virtual ~xiiGALShaderResourceVariableD3D12();

protected:
  Diligent::IShaderResourceVariable* m_pShaderResourceVariable = nullptr;
};

#include <GraphicsD3D12/Shader/Implementation/ShaderResourceVariableD3D12_inl.h>
