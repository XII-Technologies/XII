#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <ShaderCompilerDXC/SpirvMetaData.h>

XII_CHECK_AT_COMPILETIME(xiiVulkanDescriptorSetLayoutBinding::ConstantBuffer == xiiGALShaderVulkan::BindingMapping::ConstantBuffer);
XII_CHECK_AT_COMPILETIME(xiiVulkanDescriptorSetLayoutBinding::ResourceView == xiiGALShaderVulkan::BindingMapping::ResourceView);
XII_CHECK_AT_COMPILETIME(xiiVulkanDescriptorSetLayoutBinding::UAV == xiiGALShaderVulkan::BindingMapping::UAV);
XII_CHECK_AT_COMPILETIME(xiiVulkanDescriptorSetLayoutBinding::Sampler == xiiGALShaderVulkan::BindingMapping::Sampler);

void xiiGALShaderVulkan::DescriptorSetLayoutDesc::ComputeHash()
{
  xiiHashStreamWriter32 writer;
  const xiiUInt32       uiSize = m_bindings.GetCount();
  for (xiiUInt32 i = 0; i < uiSize; i++)
  {
    const auto& binding = m_bindings[i];
    writer << binding.binding;
    writer << xiiConversionUtilsVulkan::GetUnderlyingValue(binding.descriptorType);
    writer << binding.descriptorCount;
    writer << xiiConversionUtilsVulkan::GetUnderlyingFlagsValue(binding.stageFlags);
    writer << binding.pImmutableSamplers;
  }
  m_uiHash = writer.GetHashValue();
}

xiiGALShaderVulkan::xiiGALShaderVulkan(const xiiGALShaderCreationDescription& Description) :
  xiiGALShader(Description)
{
}

xiiGALShaderVulkan::~xiiGALShaderVulkan() {}

void xiiGALShaderVulkan::SetDebugName(const char* szName) const
{
  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(xiiGALDevice::GetDefaultDevice());
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; i++)
  {
    pVulkanDevice->SetDebugName(szName, m_Shaders[i]);
  }
}

xiiResult xiiGALShaderVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);

  // Extract meta data and shader code.
  xiiArrayPtr<const xiiUInt8>                      shaderCode[xiiGALShaderStage::ENUM_COUNT];
  xiiDynamicArray<xiiVulkanDescriptorSetLayout>    sets[xiiGALShaderStage::ENUM_COUNT];
  xiiHybridArray<xiiVulkanVertexInputAttribute, 8> vertexInputAttributes;

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((xiiGALShaderStage::Enum)i))
    {
      xiiArrayPtr<const xiiUInt8> metaData(reinterpret_cast<const xiiUInt8*>(m_Description.m_ByteCodes[i]->GetByteCode()), m_Description.m_ByteCodes[i]->GetSize());
      // Only the vertex shader stores vertexInputAttributes, so passing in the array into other shaders is just a no op.
      xiiSpirvMetaData::Read(metaData, shaderCode[i], sets[i], vertexInputAttributes);
    }
  }

  // For now the meta data and what the shader exposes is the exact same data but this might change so different types are used.
  for (xiiVulkanVertexInputAttribute& via : vertexInputAttributes)
  {
    m_VertexInputAttributes.PushBack({via.m_eSemantic, via.m_uiLocation, via.m_eFormat});
  }

  // Compute remapping.
  // Each shader stage is compiled individually and has its own binding indices.
  // In Vulkan we need to map all stages into one descriptor layout which requires us to remap some shader stages so no binding index conflicts appear.
  struct ShaderRemapping
  {
    const xiiVulkanDescriptorSetLayoutBinding* pBinding   = 0;
    xiiUInt16                                  m_uiTarget = 0; ///< The new binding target that pBinding needs to be remapped to.
  };
  struct LayoutBinding
  {
    const xiiVulkanDescriptorSetLayoutBinding* m_binding = nullptr; ///< The first binding under which this resource was encountered.
    vk::ShaderStageFlags                       m_stages  = {};      ///< Bitflags of all stages that share this binding. Matching is done by name.
  };
  xiiHybridArray<ShaderRemapping, 6> remappings[xiiGALShaderStage::ENUM_COUNT]; ///< Remappings for each shader stage.
  xiiHybridArray<LayoutBinding, 6>   sourceBindings;                            ///< Bindings across all stages. Can have gaps. Array index is the binding index.
  xiiMap<xiiStringView, xiiUInt32>   bindingMap;                                ///< Maps binding name to index in sourceBindings.

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; i++)
  {
    const vk::ShaderStageFlags vulkanStage = xiiConversionUtilsVulkan::GetShaderStage((xiiGALShaderStage::Enum)i);
    if (m_Description.HasByteCodeForStage((xiiGALShaderStage::Enum)i))
    {
      XII_ASSERT_DEV(sets[i].GetCount() <= 1, "Only a single descriptor set is currently supported.");

      for (xiiUInt32 j = 0; j < sets[i].GetCount(); j++)
      {
        const xiiVulkanDescriptorSetLayout& set = sets[i][j];
        XII_ASSERT_DEV(set.m_uiSet == 0, "Only a single descriptor set is currently supported.");
        for (xiiUInt32 k = 0; k < set.bindings.GetCount(); k++)
        {
          const xiiVulkanDescriptorSetLayoutBinding& binding = set.bindings[k];
          // Does a binding already exist for the resource with the same name?
          if (xiiUInt32* pBindingIdx = bindingMap.GetValue(binding.m_sName))
          {
            LayoutBinding& layoutBinding = sourceBindings[*pBindingIdx];
            layoutBinding.m_stages |= vulkanStage;
            const xiiVulkanDescriptorSetLayoutBinding* pCurrentBinding = layoutBinding.m_binding;
            XII_ASSERT_DEBUG(pCurrentBinding->m_Type == binding.m_Type, "The descriptor {} was found with different resource type {} and {}", binding.m_sName, pCurrentBinding->m_Type, binding.m_Type);
            XII_ASSERT_DEBUG(pCurrentBinding->m_uiDescriptorType == binding.m_uiDescriptorType, "The descriptor {} was found with different type {} and {}", binding.m_sName, pCurrentBinding->m_uiDescriptorType, binding.m_uiDescriptorType);
            XII_ASSERT_DEBUG(pCurrentBinding->m_uiDescriptorCount == binding.m_uiDescriptorCount, "The descriptor {} was found with different count {} and {}", binding.m_sName, pCurrentBinding->m_uiDescriptorCount, binding.m_uiDescriptorCount);
            // The binding index differs from the one already in the set, remapping is necessary.
            if (binding.m_uiBinding != *pBindingIdx)
            {
              remappings[i].PushBack({&binding, pCurrentBinding->m_uiBinding});
            }
          }
          else
          {
            xiiUInt8 uiTargetBinding = binding.m_uiBinding;
            // Doesn't exist yet, find a good place for it.
            if (binding.m_uiBinding >= sourceBindings.GetCount())
              sourceBindings.SetCount(binding.m_uiBinding + 1);

            // If the original binding index doesn't exist yet, use it (No remapping necessary).
            if (sourceBindings[binding.m_uiBinding].m_binding == nullptr)
            {
              sourceBindings[binding.m_uiBinding] = {&binding, vulkanStage};
              bindingMap[binding.m_sName]         = uiTargetBinding;
            }
            else
            {
              // Binding index already in use, remapping necessary.
              uiTargetBinding = (xiiUInt8)sourceBindings.GetCount();
              sourceBindings.PushBack({&binding, vulkanStage});
              bindingMap[binding.m_sName] = uiTargetBinding;
              remappings[i].PushBack({&binding, uiTargetBinding});
            }

            // The shader reflection used by the high level renderer is per stage and assumes it can map resources to stages.
            // We build this remapping table to map our descriptor binding to the original per-stage resource binding model.
            BindingMapping& bindingMapping  = m_BindingMapping.ExpandAndGetRef();
            bindingMapping.m_descriptorType = (vk::DescriptorType)binding.m_uiDescriptorType;
            bindingMapping.m_xiiType        = binding.m_xiiType;
            bindingMapping.m_type           = (BindingMapping::Type)binding.m_Type;
            bindingMapping.m_stage          = (xiiGALShaderStage::Enum)i;
            bindingMapping.m_uiSource       = binding.m_uiVirtualBinding;
            bindingMapping.m_uiTarget       = uiTargetBinding;
            bindingMapping.m_sName          = binding.m_sName;
          }
        }
      }
    }
  }
  m_BindingMapping.Sort([](const BindingMapping& lhs, const BindingMapping& rhs) { return lhs.m_uiTarget < rhs.m_uiTarget; });
  for (xiiUInt32 i = 0; i < m_BindingMapping.GetCount(); i++)
  {
    m_BindingMapping[i].m_targetStages = xiiConversionUtilsVulkan::GetPipelineStage(sourceBindings[m_BindingMapping[i].m_uiTarget].m_stages);
  }

  // Build Vulkan descriptor set layout
  for (xiiUInt32 i = 0; i < sourceBindings.GetCount(); i++)
  {
    const LayoutBinding& sourceBinding = sourceBindings[i];
    if (sourceBinding.m_binding != nullptr)
    {
      vk::DescriptorSetLayoutBinding& binding = m_descriptorSetLayoutDesc.m_bindings.ExpandAndGetRef();
      binding.binding                         = i;
      binding.descriptorType                  = (vk::DescriptorType)sourceBinding.m_binding->m_uiDescriptorType;
      binding.descriptorCount                 = sourceBinding.m_binding->m_uiDescriptorCount;
      binding.stageFlags                      = sourceBinding.m_stages;
    }
  }
  m_descriptorSetLayoutDesc.m_bindings.Sort([](const vk::DescriptorSetLayoutBinding& lhs, const vk::DescriptorSetLayoutBinding& rhs) { return lhs.binding < rhs.binding; });
  m_descriptorSetLayoutDesc.ComputeHash();

  // Remap and build shaders
  xiiUInt32 uiMaxShaderSize = 0;
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; i++)
  {
    if (!remappings[i].IsEmpty())
    {
      uiMaxShaderSize = xiiMath::Max(uiMaxShaderSize, shaderCode[i].GetCount());
    }
  }

  vk::ShaderModuleCreateInfo createInfo;
  xiiDynamicArray<xiiUInt8>  tempBuffer;
  tempBuffer.Reserve(uiMaxShaderSize);
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((xiiGALShaderStage::Enum)i))
    {
      if (remappings[i].IsEmpty())
      {
        createInfo.codeSize = shaderCode[i].GetCount();
        XII_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
        createInfo.pCode = reinterpret_cast<const xiiUInt32*>(shaderCode[i].GetPtr());
        VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
      }
      else
      {
        tempBuffer       = shaderCode[i];
        xiiUInt32* pData = reinterpret_cast<xiiUInt32*>(tempBuffer.GetData());
        for (const auto& remap : remappings[i])
        {
          XII_ASSERT_DEBUG(pData[remap.pBinding->m_uiWordOffset] == remap.pBinding->m_uiBinding, "Spirv descriptor word offset does not point to descriptor index.");
          pData[remap.pBinding->m_uiWordOffset] = remap.m_uiTarget;
        }
        createInfo.codeSize = tempBuffer.GetCount();
        XII_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
        createInfo.pCode = pData;
        VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALShaderVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  m_descriptorSetLayoutDesc = {};
  m_BindingMapping.Clear();

  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; i++)
  {
    pVulkanDevice->DeleteLater(m_Shaders[i]);
  }
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_ShaderVulkan);
