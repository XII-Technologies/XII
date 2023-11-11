#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_GRAPHICSD3D12_DLL xiiGALInputLayoutD3D12 : public xiiGALInputLayout
{
public:
  XII_ALWAYS_INLINE const Diligent::InputLayoutDesc* GetLayout() const;

  XII_ALWAYS_INLINE xiiArrayPtr<const Diligent::LayoutElement> GetElements() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALInputLayoutD3D12(const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::InputLayoutDesc                   m_InputLayout = {};
  xiiHybridArray<Diligent::LayoutElement, 8U> m_InputElements;
};

#include <GraphicsD3D12/Shader/Implementation/InputLayoutD3D12_inl.h>
