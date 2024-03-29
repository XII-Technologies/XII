#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_GRAPHICSD3D11_DLL xiiGALInputLayoutD3D11 final : public xiiGALInputLayout
{
public:
  const Diligent::InputLayoutDesc* GetLayout() const;

  xiiArrayPtr<Diligent::LayoutElement> GetElements();

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALInputLayoutD3D11(const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::InputLayoutDesc                   m_InputLayout = {};
  xiiHybridArray<Diligent::LayoutElement, 8U> m_InputElements;
};

#include <GraphicsD3D11/Shader/Implementation/InputLayoutD3D11_inl.h>
