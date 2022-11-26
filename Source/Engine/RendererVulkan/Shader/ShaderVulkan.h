
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

#include <RendererCore/Shader/ShaderStageBinary.h>
#include <vulkan/vulkan.hpp>

class XII_RENDERERVULKAN_DLL xiiGALShaderVulkan : public xiiGALShader
{
public:
  /// \brief Used as input to xiiResourceCacheVulkan::RequestDescriptorSetLayout to create a vk::DescriptorSetLayout.
  struct DescriptorSetLayoutDesc
  {
    mutable xiiUInt32                                 m_uiHash = 0;
    xiiHybridArray<vk::DescriptorSetLayoutBinding, 6> m_bindings;
    void                                              ComputeHash();
  };

  /// \brief Remaps high level resource binding to the descriptor layout used by this shader.
  struct BindingMapping
  {
    enum Type : xiiUInt8
    {
      ConstantBuffer,
      ResourceView,
      UAV,
      Sampler,
    };
    vk::DescriptorType          m_descriptorType = vk::DescriptorType::eSampler;   ///< Descriptor slot type.
    xiiShaderResourceType::Enum m_xiiType        = xiiShaderResourceType::Unknown; ///< XII resource type. We need this to find a compatible fallback resource is a descriptor slot is empty.
    Type                        m_type           = Type::ConstantBuffer;           ///< Source resource type in the high level binding model.
    xiiGALShaderStage::Enum     m_stage          = xiiGALShaderStage::ENUM_COUNT;  ///< Source stage in the high level resource binding model.
    xiiUInt8                    m_uiSource       = 0;                              ///< Source binding index in the high level resource binding model.
    xiiUInt8                    m_uiTarget       = 0;                              ///< Target binding index in the descriptor set layout.
    vk::PipelineStageFlags      m_targetStages;                                    ///< Target stages that this mapping is used in.
    xiiStringView               m_sName;
  };

  struct VertexInputAttribute
  {
    xiiGALVertexAttributeSemantic::Enum m_eSemantic  = xiiGALVertexAttributeSemantic::Position;
    xiiUInt8                            m_uiLocation = 0;
    xiiGALResourceFormat::Enum          m_eFormat    = xiiGALResourceFormat::XYZFloat;
  };

  void SetDebugName(const char* szName) const override;

  XII_ALWAYS_INLINE vk::ShaderModule GetShader(xiiGALShaderStage::Enum stage) const;
  XII_ALWAYS_INLINE const DescriptorSetLayoutDesc& GetDescriptorSetLayout() const;
  XII_ALWAYS_INLINE const xiiArrayPtr<const BindingMapping> GetBindingMapping() const;
  XII_ALWAYS_INLINE const xiiArrayPtr<const VertexInputAttribute> GetVertexInputAttributes() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALShaderVulkan(const xiiGALShaderCreationDescription& description);
  virtual ~xiiGALShaderVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

private:
  DescriptorSetLayoutDesc                 m_descriptorSetLayoutDesc;
  xiiHybridArray<BindingMapping, 16>      m_BindingMapping;
  xiiHybridArray<VertexInputAttribute, 8> m_VertexInputAttributes;
  vk::ShaderModule                        m_Shaders[xiiGALShaderStage::ENUM_COUNT];
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
