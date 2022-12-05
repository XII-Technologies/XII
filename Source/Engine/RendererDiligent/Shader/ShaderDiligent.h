
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

class XII_RENDERERDILIGENT_DLL xiiGALShaderDiligent : public xiiGALShader
{
public:
  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::IShader>& GetVertexShader();

  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::IShader>& GetHullShader();

  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::IShader>& GetDomainShader();

  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::IShader>& GetGeometryShader();

  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::IShader>& GetPixelShader();

  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::IShader>& GetComputeShader();


protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALShaderDiligent(const xiiGALShaderCreationDescription& description);

  virtual ~xiiGALShaderDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RefCntAutoPtr<Diligent::IShader> m_pVertexShader;
  Diligent::RefCntAutoPtr<Diligent::IShader> m_pHullShader;
  Diligent::RefCntAutoPtr<Diligent::IShader> m_pDomainShader;
  Diligent::RefCntAutoPtr<Diligent::IShader> m_pGeometryShader;
  Diligent::RefCntAutoPtr<Diligent::IShader> m_pPixelShader;
  Diligent::RefCntAutoPtr<Diligent::IShader> m_pComputeShader;
};

#include <RendererDiligent/Shader/Implementation/ShaderDiligent_inl.h>
