#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSVULKAN_DLL xiiGALShaderVulkan final : public xiiGALShader
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
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALShaderVulkan(const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::IShader* m_pShaderStages[xiiGALShaderStage::ENUM_COUNT] = {};
};

#include <GraphicsVulkan/Shader/Implementation/ShaderVulkan_inl.h>
