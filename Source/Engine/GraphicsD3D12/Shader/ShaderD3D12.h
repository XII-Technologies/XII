#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSD3D12_DLL xiiGALShaderD3D12 final : public xiiGALShader
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
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALShaderD3D12(const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::IShader* m_pShaderStages[xiiGALShaderStage::ENUM_COUNT] = {};
};

#include <GraphicsD3D12/Shader/Implementation/ShaderD3D12_inl.h>
