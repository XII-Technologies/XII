#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Utils/CommandListUtilities.h>

void xiiGALCommandListUtilities::BindConstantBuffer(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pConstantBuffer)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pConstantBuffer != nullptr, "pConstantBuffer must not be null.");

  pCommandList->ResolveAndSetConstantBuffer(sSlotName, pConstantBuffer);
}

void xiiGALCommandListUtilities::BindBuffer(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pBuffer)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be null.");

  pCommandList->ResolveAndSetShaderResourceBufferView(sSlotName, pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
}

void xiiGALCommandListUtilities::BindBufferView(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pBufferView != nullptr, "pBufferView must not be null.");

  pCommandList->ResolveAndSetShaderResourceBufferView(sSlotName, pBufferView);
}

void xiiGALCommandListUtilities::BindTexture(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTexture> pTexture)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pTexture != nullptr, "pTexture must not be null.");

  pCommandList->ResolveAndSetShaderResourceTextureView(sSlotName, pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));
}

void xiiGALCommandListUtilities::BindTextureView(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pTextureView != nullptr, "pTextureView must not be null.");

  pCommandList->ResolveAndSetShaderResourceTextureView(sSlotName, pTextureView);
}

void xiiGALCommandListUtilities::BindSampler(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALSampler> pSamplerSate)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pSamplerSate != nullptr, "pSamplerSate must not be null.");

  pCommandList->ResolveAndSetSampler(sSlotName, pSamplerSate);
}

void xiiGALCommandListUtilities::BindBufferUAV(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pBuffer)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be null.");

  pCommandList->ResolveAndSetUnorderedAccessBufferView(sSlotName, pBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess));
}

void xiiGALCommandListUtilities::BindBufferViewUAV(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pBufferView != nullptr, "pBufferView must not be null.");

  pCommandList->ResolveAndSetUnorderedAccessBufferView(sSlotName, pBufferView);
}

void xiiGALCommandListUtilities::BindTextureUAV(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTexture> pTexture)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pTexture != nullptr, "pTexture must not be null.");

  pCommandList->ResolveAndSetUnorderedAccessTextureView(sSlotName, pTexture->GetDefaultView(xiiGALTextureViewType::UnorderedAccess));
}

void xiiGALCommandListUtilities::BindTextureViewUAV(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(pTextureView != nullptr, "pTextureView must not be null.");

  pCommandList->ResolveAndSetUnorderedAccessTextureView(sSlotName, pTextureView);
}

void xiiGALCommandListUtilities::BindTexture2D(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, const xiiTexture2DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(hTexture.IsValid(), "hTexture must be valid.");

  xiiResourceLock<xiiTexture2DResource> pTexture(hTexture, acquireMode);

  pCommandList->ResolveAndSetShaderResourceTextureView(sSlotName, pTexture->GetGALTexture());
  pCommandList->ResolveAndSetSampler(sSlotName, pTexture->GetGALSampler());
}

void xiiGALCommandListUtilities::BindTexture3D(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, const xiiTexture3DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(hTexture.IsValid(), "hTexture must be valid.");

  xiiResourceLock<xiiTexture3DResource> pTexture(hTexture, acquireMode);

  pCommandList->ResolveAndSetShaderResourceTextureView(sSlotName, pTexture->GetGALTexture());
  pCommandList->ResolveAndSetSampler(sSlotName, pTexture->GetGALSampler());
}

void xiiGALCommandListUtilities::BindTextureCube(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, const xiiTextureCubeResourceHandle& hTexture, xiiResourceAcquireMode acquireMode)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(hTexture.IsValid(), "hTexture must be valid.");

  xiiResourceLock<xiiTextureCubeResource> pTexture(hTexture, acquireMode);

  pCommandList->ResolveAndSetShaderResourceTextureView(sSlotName, pTexture->GetGALTexture());
  pCommandList->ResolveAndSetSampler(sSlotName, pTexture->GetGALSampler());
}

void xiiGALCommandListUtilities::BindMaterial(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiMaterialResourceHandle& hMaterial)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "pCommandList must not be null.");
  XII_ASSERT_DEV(hMaterial.IsValid(), "hMaterial must be valid.");

  xiiResourceLock<xiiMaterialResource> pMaterial(hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);

  XII_IGNORE_UNUSED(pMaterial);

  /// \todo Create or update pipeline state.
}
