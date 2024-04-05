#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

struct ID3D11InputLayout;
struct D3D11_INPUT_ELEMENT_DESC;

XII_DEFINE_AS_POD_TYPE(D3D11_INPUT_ELEMENT_DESC);

class XII_GRAPHICSD3D11_DLL xiiGALInputLayoutD3D11 final : public xiiGALInputLayout
{
public:
  ID3D11InputLayout* GetInputLayout() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALInputLayoutD3D11(const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  ID3D11InputLayout* m_pInputLayout = {};
};

#include <GraphicsD3D11/Shader/Implementation/InputLayoutD3D11_inl.h>
