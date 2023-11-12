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

  XII_ALWAYS_INLINE Diligent::IShader* GetVertexShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetPixelShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetGeometryShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetHullShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetDomainShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetComputeShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetAmplificationShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetMeshShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetRayGenerationShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetRayMissShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetRayClosestHitShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetRayAnyHitShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetRayIntersectionShader() const;
  XII_ALWAYS_INLINE Diligent::IShader* GetCallableShader() const;

  XII_ALWAYS_INLINE xiiArrayPtr<Diligent::RefCntAutoPtr<Diligent::IPipelineResourceSignature>> GetResourceSignatures();
  XII_ALWAYS_INLINE xiiArrayPtr<xiiGALVertexInputLayout> GetInputLayouts();
  XII_ALWAYS_INLINE xiiArrayPtr<xiiGALShaderResourceBinding> GetShaderResourceBinding();

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALShaderD3D12(const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::RefCntAutoPtr<Diligent::IShader>                                        m_pShaderStages[xiiGALShaderStage::ENUM_COUNT];
  xiiHybridArray<Diligent::RefCntAutoPtr<Diligent::IPipelineResourceSignature>, 3U> m_PipelineResourceSignatures;

  xiiHybridArray<xiiGALVertexInputLayout, 8U>  m_VertexInputLayouts;
  xiiDynamicArray<xiiGALShaderResourceBinding> m_ShaderResourceBindings;
};

#include <GraphicsD3D12/Shader/Implementation/ShaderD3D12_inl.h>
