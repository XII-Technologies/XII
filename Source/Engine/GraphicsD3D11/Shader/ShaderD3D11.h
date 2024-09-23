#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

struct ID3D11DeviceChild;

class XII_GRAPHICSD3D11_DLL xiiGALShaderD3D11 final : public xiiGALShader
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALShaderD3D11, xiiGALShader);

public:
  XII_ALWAYS_INLINE ID3D11DeviceChild* GetD3D11Shader() const { return m_pD3D11Shader; };

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALShaderD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderD3D11();

  virtual xiiResult InitPlatform();

  virtual xiiResult DeInitPlatform();

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  ID3D11DeviceChild* m_pD3D11Shader = nullptr;
};
