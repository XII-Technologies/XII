
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;

class XII_RENDERERDX11_DLL xiiGALShaderDX11 : public xiiGALShader
{
public:
  void SetDebugName(const char* szName) const override;

  XII_ALWAYS_INLINE ID3D11VertexShader* GetDXVertexShader() const;

  XII_ALWAYS_INLINE ID3D11HullShader* GetDXHullShader() const;

  XII_ALWAYS_INLINE ID3D11DomainShader* GetDXDomainShader() const;

  XII_ALWAYS_INLINE ID3D11GeometryShader* GetDXGeometryShader() const;

  XII_ALWAYS_INLINE ID3D11PixelShader* GetDXPixelShader() const;

  XII_ALWAYS_INLINE ID3D11ComputeShader* GetDXComputeShader() const;


protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALShaderDX11(const xiiGALShaderCreationDescription& description);

  virtual ~xiiGALShaderDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  ID3D11VertexShader*   m_pVertexShader;
  ID3D11HullShader*     m_pHullShader;
  ID3D11DomainShader*   m_pDomainShader;
  ID3D11GeometryShader* m_pGeometryShader;
  ID3D11PixelShader*    m_pPixelShader;
  ID3D11ComputeShader*  m_pComputeShader;
};

#include <RendererDX11/Shader/Implementation/ShaderDX11_inl.h>
