#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSD3D12_DLL xiiGALShaderD3D12 : public xiiGALShader
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

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALShaderD3D12(const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::RefCntAutoPtr<Diligent::IShader>                m_pShaderStages[xiiGALShaderStage::ENUM_COUNT];
  xiiHybridArray<Diligent::IPipelineResourceSignature*, 3U> m_PipelineResourceSignatures;

  xiiHybridArray<xiiGALVertexInputLayout, 8U>  m_VertexInputLayouts;
  xiiDynamicArray<xiiGALShaderResourceBinding> m_ShaderResourceBindings[xiiGALShaderStage::ENUM_COUNT];
};

#include <GraphicsD3D12/Shader/Implementation/ShaderD3D12_inl.h>
