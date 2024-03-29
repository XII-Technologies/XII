#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSD3D11_DLL xiiGALShaderD3D11 final : public xiiGALShader
{
public:
  Diligent::IShader* GetVertexShader() const;
  Diligent::IShader* GetPixelShader() const;
  Diligent::IShader* GetGeometryShader() const;
  Diligent::IShader* GetHullShader() const;
  Diligent::IShader* GetDomainShader() const;
  Diligent::IShader* GetComputeShader() const;
  Diligent::IShader* GetAmplificationShader() const;
  Diligent::IShader* GetMeshShader() const;
  Diligent::IShader* GetRayGenerationShader() const;
  Diligent::IShader* GetRayMissShader() const;
  Diligent::IShader* GetRayClosestHitShader() const;
  Diligent::IShader* GetRayAnyHitShader() const;
  Diligent::IShader* GetRayIntersectionShader() const;
  Diligent::IShader* GetCallableShader() const;
  Diligent::IShader* GetTileShader() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALShaderD3D11(const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::IShader* m_pShaderStages[xiiGALShaderStage::ENUM_COUNT] = {};
};

#include <GraphicsD3D11/Shader/Implementation/ShaderD3D11_inl.h>
