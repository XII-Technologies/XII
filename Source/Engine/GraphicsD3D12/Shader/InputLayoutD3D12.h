#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

XII_DEFINE_AS_POD_TYPE(D3D12_INPUT_ELEMENT_DESC);

class XII_GRAPHICSD3D12_DLL xiiGALInputLayoutD3D12 final : public xiiGALInputLayout
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALInputLayoutD3D12, xiiGALInputLayout);

public:
  xiiArrayPtr<const D3D12_INPUT_ELEMENT_DESC> GetD3D12InputLayoutElements() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALInputLayoutD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:

  xiiDynamicArray<D3D12_INPUT_ELEMENT_DESC> m_InputLayoutElements;
};

#include <GraphicsD3D12/Shader/Implementation/InputLayoutD3D12_inl.h>
