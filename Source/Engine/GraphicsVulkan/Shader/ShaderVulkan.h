#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSVULKAN_DLL xiiGALShaderVulkan final : public xiiGALShader
{
public:
  /// \brief This returns the total number of shader resources.
  virtual xiiUInt32 GetResourceCount() const override;

  /// \brief This returns a pointer to the array of shader resources.
  virtual void GetResourceDescription(xiiUInt32 uiIndex, xiiGALShaderResourceDescription& out_ResourceDescription) const override;

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

  xiiArrayPtr<Diligent::IPipelineResourceSignature*> GetResourceSignatures();
  xiiArrayPtr<xiiGALVertexInputLayout>               GetInputLayouts();
  xiiArrayPtr<xiiGALShaderResourceBinding>           GetShaderResourceBinding(xiiBitflags<xiiGALShaderStage> e);

public:
  struct ShaderEvent
  {
    XII_DECLARE_POD_TYPE();

    enum Type
    {
      BeforeDeletion = 0
    };

    xiiGALShaderVulkan* m_pShader = nullptr;
    Type                m_Type    = Type::BeforeDeletion;
  };

  xiiCopyOnBroadcastEvent<const ShaderEvent&> m_Events;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALShaderVulkan(const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::IShader*                                        m_pShaderStages[xiiGALShaderStage::ENUM_COUNT] = {};
  xiiHybridArray<Diligent::IPipelineResourceSignature*, 3U> m_PipelineResourceSignatures;

  xiiHybridArray<xiiGALVertexInputLayout, 8U>  m_VertexInputLayouts;
  xiiDynamicArray<xiiGALShaderResourceBinding> m_ShaderResourceBindings[xiiGALShaderStage::ENUM_COUNT];
};

#include <GraphicsVulkan/Shader/Implementation/ShaderVulkan_inl.h>
