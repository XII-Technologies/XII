
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <RendererFoundation/Shader/Shader.h>

#include <ShaderCompiler/ShaderMetadata.h>

class XII_RENDERERDILIGENT_DLL xiiGALShaderDiligent : public xiiGALShader
{
public:
  XII_ALWAYS_INLINE Diligent::IShader* GetVertexShader();

  XII_ALWAYS_INLINE Diligent::IShader* GetHullShader();

  XII_ALWAYS_INLINE Diligent::IShader* GetDomainShader();

  XII_ALWAYS_INLINE Diligent::IShader* GetGeometryShader();

  XII_ALWAYS_INLINE Diligent::IShader* GetPixelShader();

  XII_ALWAYS_INLINE Diligent::IShader* GetComputeShader();

  XII_ALWAYS_INLINE Diligent::IShader* GetAmplificationShader();

  XII_ALWAYS_INLINE Diligent::IShader* GetMeshShader();

  XII_ALWAYS_INLINE xiiDynamicArray<xiiShaderDescriptorSetLayout>& GetDescriptorSets(xiiGALShaderStage::Enum stage);

  XII_ALWAYS_INLINE xiiHybridArray<xiiShaderVertexInputAttribute, 8>& GetVertexInputAttributes();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALShaderDiligent(const xiiGALShaderCreationDescription& description);

  virtual ~xiiGALShaderDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::IShader* m_pShaderStages[xiiGALShaderStage::ENUM_COUNT] = {nullptr};

  xiiDynamicArray<xiiShaderDescriptorSetLayout> m_DescriptorSets[xiiGALShaderStage::ENUM_COUNT];

  xiiHybridArray<xiiShaderVertexInputAttribute, 8> m_VertexInputAttributes;
};

#include <RendererDiligent/Shader/Implementation/ShaderDiligent_inl.h>
