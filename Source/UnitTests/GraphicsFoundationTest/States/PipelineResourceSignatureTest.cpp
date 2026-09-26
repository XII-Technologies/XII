/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

namespace
{
  xiiGALPipelineResourceDescription MakeResource(xiiStringView sName, xiiGALShaderResourceType::Enum type, xiiGALShaderType::Enum stages, xiiUInt32 uiSet, xiiUInt32 uiSlot)
  {
    xiiGALPipelineResourceDescription resource;
    resource.m_sName.Assign(sName);
    resource.m_ResourceType = type;
    resource.m_ShaderStages = stages;
    resource.m_uiArraySize  = 1U;
    resource.m_uiBindSet    = uiSet;
    resource.m_uiBindSlot   = uiSlot;
    return resource;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(States, PipelineResourceSignature)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Descriptor equality and hashing")
  {
    xiiGALPipelineResourceSignatureCreationDescription a;
    a.m_uiBindingIndex = 1U;
    a.m_Resources.PushBack(MakeResource("Constants", xiiGALShaderResourceType::ConstantBuffer, xiiGALShaderType::Vertex, 0U, 2U));
    a.m_Resources.PushBack(MakeResource("Diffuse", xiiGALShaderResourceType::TextureSRV, xiiGALShaderType::Pixel, 1U, 0U));
    auto& range          = a.m_PushConstantRanges.ExpandAndGetRef();
    range.m_uiOffset     = 0U;
    range.m_uiSize       = 16U;
    range.m_ShaderStages = xiiGALShaderType::Vertex;

    xiiGALPipelineResourceSignatureCreationDescription b = a;
    XII_TEST_BOOL(xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_INT(xiiGALDescriptorHash::Hash(a), xiiGALDescriptorHash::Hash(b));

    b.m_Resources[0].m_uiBindSlot = 3U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b                            = a;
    b.m_Resources[1].m_uiBindSet = 2U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b                                  = a;
    b.m_PushConstantRanges[0].m_uiSize = 32U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b                                        = a;
    b.m_PushConstantRanges[0].m_ShaderStages = xiiGALShaderType::Pixel;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Device creation, sorting, and compatibility")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALPipelineResourceSignatureCreationDescription description;
      description.m_uiBindingIndex = 0U;
      description.m_Resources.PushBack(MakeResource("LaterSet", xiiGALShaderResourceType::TextureSRV, xiiGALShaderType::Pixel, 2U, 1U));
      description.m_Resources.PushBack(MakeResource("FirstSet", xiiGALShaderResourceType::ConstantBuffer, xiiGALShaderType::Vertex, 0U, 0U));

      auto& range          = description.m_PushConstantRanges.ExpandAndGetRef();
      range.m_uiOffset     = 0U;
      range.m_uiSize       = 16U;
      range.m_ShaderStages = xiiGALShaderType::Vertex;

      xiiSharedPtr<xiiGALPipelineResourceSignature> pSignature = environment.GetDevice()->CreatePipelineResourceSignature(description);
      XII_TEST_BOOL(pSignature != nullptr);
      if (pSignature == nullptr)
        continue;

      XII_TEST_INT(description.m_Resources[0].m_uiBindSet, 0U);
      XII_TEST_INT(description.m_Resources[1].m_uiBindSet, 2U);
      XII_TEST_BOOL(pSignature->GetDescription() == description);
      XII_TEST_BOOL(pSignature->GetDevice().Borrow() == environment.GetDevice());
      pSignature->SetDebugName("Unit Test Resource Signature");
      XII_TEST_STRING(pSignature->GetDebugName(), "Unit Test Resource Signature");
      XII_TEST_BOOL(pSignature->IsCompatibleWith(pSignature.Borrow()));

      xiiGALPipelineResourceSignatureCreationDescription equivalentDescription = description;
      xiiSharedPtr<xiiGALPipelineResourceSignature>      pEquivalent           = environment.GetDevice()->CreatePipelineResourceSignature(equivalentDescription);
      XII_TEST_BOOL(pEquivalent != nullptr);
      XII_TEST_BOOL(pSignature->IsCompatibleWith(pEquivalent.Borrow()));
      XII_TEST_BOOL(pEquivalent->IsCompatibleWith(pSignature.Borrow()));

      xiiGALPipelineResourceSignatureCreationDescription incompatibleDescription = description;
      incompatibleDescription.m_PushConstantRanges[0].m_ShaderStages             = xiiGALShaderType::Pixel;
      xiiSharedPtr<xiiGALPipelineResourceSignature> pIncompatible                = environment.GetDevice()->CreatePipelineResourceSignature(incompatibleDescription);
      XII_TEST_BOOL(pIncompatible != nullptr);
      XII_TEST_BOOL(!pSignature->IsCompatibleWith(pIncompatible.Borrow()));
      XII_TEST_BOOL(!pIncompatible->IsCompatibleWith(pSignature.Borrow()));
    }
  }
}
