/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

XII_CREATE_SIMPLE_TEST_GROUP(Utilities);

XII_CREATE_SIMPLE_TEST(Utilities, GraphicsUtilities)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap-chain usage conversion")
  {
    XII_TEST_BOOL(xiiGALGraphicsUtilities::SwapChainUsageFlagsToBindFlags(xiiGALSwapChainUsageFlags::None).IsNoFlagSet());

    const xiiBitflags<xiiGALSwapChainUsageFlags> usage = xiiGALSwapChainUsageFlags::RenderTarget | xiiGALSwapChainUsageFlags::ShaderResource | xiiGALSwapChainUsageFlags::InputAttachment | xiiGALSwapChainUsageFlags::CopySource;
    const xiiBitflags<xiiGALBindFlags>           flags = xiiGALGraphicsUtilities::SwapChainUsageFlagsToBindFlags(usage);
    XII_TEST_BOOL(flags.IsSet(xiiGALBindFlags::RenderTarget));
    XII_TEST_BOOL(flags.IsSet(xiiGALBindFlags::ShaderResource));
    XII_TEST_BOOL(flags.IsSet(xiiGALBindFlags::InputAttachment));
    XII_TEST_INT(flags.GetValue(), (xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::InputAttachment).GetValue());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pipeline resource flag domains")
  {
    const auto constantBuffer = xiiGALGraphicsUtilities::GetValidPipelineResourceFlags(xiiGALShaderResourceType::ConstantBuffer);
    XII_TEST_BOOL(constantBuffer.IsSet(xiiGALPipelineResourceFlags::NoDynamicBuffers));
    XII_TEST_BOOL(constantBuffer.IsSet(xiiGALPipelineResourceFlags::RuntimeArray));
    XII_TEST_BOOL(!constantBuffer.IsSet(xiiGALPipelineResourceFlags::CombinedSampler));

    const auto texture = xiiGALGraphicsUtilities::GetValidPipelineResourceFlags(xiiGALShaderResourceType::TextureSRV);
    XII_TEST_BOOL(texture.IsSet(xiiGALPipelineResourceFlags::CombinedSampler));
    XII_TEST_BOOL(texture.IsSet(xiiGALPipelineResourceFlags::RuntimeArray));

    const auto buffer = xiiGALGraphicsUtilities::GetValidPipelineResourceFlags(xiiGALShaderResourceType::BufferUAV);
    XII_TEST_BOOL(buffer.IsSet(xiiGALPipelineResourceFlags::NoDynamicBuffers));
    XII_TEST_BOOL(buffer.IsSet(xiiGALPipelineResourceFlags::Formattedbuffer));
    XII_TEST_BOOL(buffer.IsSet(xiiGALPipelineResourceFlags::RuntimeArray));

    const auto inputAttachment = xiiGALGraphicsUtilities::GetValidPipelineResourceFlags(xiiGALShaderResourceType::InputAttachment);
    XII_TEST_INT(inputAttachment.GetValue(), xiiGALPipelineResourceFlags::GeneralInputAttachment);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default sampler")
  {
    const xiiGALSamplerCreationDescription description = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();
    XII_TEST_BOOL(description.m_MinFilter == xiiGALFilterType::Linear);
    XII_TEST_BOOL(description.m_MagFilter == xiiGALFilterType::Linear);
    XII_TEST_BOOL(description.m_MipFilter == xiiGALFilterType::Linear);
    XII_TEST_BOOL(description.m_AddressU == xiiGALTextureAddressMode::Wrap);
    XII_TEST_BOOL(description.m_AddressV == xiiGALTextureAddressMode::Wrap);
    XII_TEST_BOOL(description.m_AddressW == xiiGALTextureAddressMode::Wrap);
    XII_TEST_INT(description.m_uiMaxAnisotropy, 4U);
    XII_TEST_BOOL(description.m_ComparisonFunction == xiiGALComparisonFunction::Never);
    XII_TEST_BOOL(description.m_BorderColor == xiiColor::Black);
    XII_TEST_FLOAT(description.m_fMinLOD, -1.0f, 0.0f);
    XII_TEST_FLOAT(description.m_fMaxLOD, 4200.0f, 0.0f);
  }
}
