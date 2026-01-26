#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/RenderContext/RendererFallbackResources.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, FallbackResources)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiRendererFallbackResources::Initialize();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiRendererFallbackResources::DeInitialize();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiSharedPtr<xiiGALDevice> xiiRendererFallbackResources::s_pDevice;

xiiHashTable<xiiRendererFallbackResources::Key, xiiSharedPtr<xiiGALTextureView>, xiiRendererFallbackResources::KeyHash> xiiRendererFallbackResources::s_TextureResourceViews;
xiiHashTable<xiiEnum<xiiGALShaderResourceType>, xiiSharedPtr<xiiGALBufferView>, xiiRendererFallbackResources::KeyHash>  xiiRendererFallbackResources::s_BufferResourceViews;
xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>>                                                                             xiiRendererFallbackResources::s_Buffers;
xiiDynamicArray<xiiSharedPtr<xiiGALTexture>>                                                                            xiiRendererFallbackResources::s_Textures;

void xiiRendererFallbackResources::Initialize()
{
  s_pDevice = xiiGALDevice::GetDefaultDevice();

  auto CreateTexture = [](xiiGALResourceDimension::Enum dimension, xiiGALSampleCount::Enum samples, bool bDepth) -> xiiSharedPtr<xiiGALTexture> {
    xiiGALTextureCreationDescription description;
    description.m_Type               = dimension;
    description.m_Format             = bDepth ? xiiGALResourceFormat::D16UNormalized : xiiGALResourceFormat::BGRA8UNormalizedSRGB;
    description.m_Size               = xiiSizeU32(4, 4);
    description.m_uiArraySizeOrDepth = (dimension == xiiGALResourceDimension::Texture3D) ? 4 : 1;
    description.m_uiMipLevels        = 1;
    description.m_uiSampleCount      = samples;
    description.m_Usage              = xiiGALResourceUsage::Immutable;
    description.m_BindFlags          = xiiGALBindFlags::ShaderResource;

    if (bDepth)
    {
      description.m_BindFlags.Add(xiiGALBindFlags::DepthStencil);
    }
    if (dimension == xiiGALResourceDimension::TextureCube || dimension == xiiGALResourceDimension::TextureCubeArray)
    {
      description.m_uiArraySizeOrDepth *= 6;
    }

    xiiGALTextureData           initialData;
    xiiSharedPtr<xiiGALTexture> pTexture = s_pDevice->CreateTexture(description, &initialData);
    XII_ASSERT_DEV(pTexture != nullptr, "Failed to create fallback texture resource.");
    pTexture->SetDebugName("FallbackTexture");

    s_Textures.PushBack(pTexture);

    return pTexture;
  };

  {
    xiiSharedPtr<xiiGALTexture>     pTexture     = CreateTexture(xiiGALResourceDimension::Texture2D, xiiGALSampleCount::OneSample, false);
    xiiSharedPtr<xiiGALTextureView> pTextureView = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);

    s_TextureResourceViews[{xiiGALShaderResourceType::TextureSRV, xiiGALShaderTextureType::Texture2D, false}]             = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureSRV, xiiGALShaderTextureType::Texture2DArray, false}]        = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureAndSampler, xiiGALShaderTextureType::Texture2D, false}]      = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureAndSampler, xiiGALShaderTextureType::Texture2DArray, false}] = pTextureView;
  }
  {
    xiiSharedPtr<xiiGALTexture>     pTexture     = CreateTexture(xiiGALResourceDimension::Texture2D, xiiGALSampleCount::OneSample, true);
    xiiSharedPtr<xiiGALTextureView> pTextureView = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);

    s_TextureResourceViews[{xiiGALShaderResourceType::TextureSRV, xiiGALShaderTextureType::Texture2D, true}]             = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureSRV, xiiGALShaderTextureType::Texture2DArray, true}]        = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureAndSampler, xiiGALShaderTextureType::Texture2D, true}]      = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureAndSampler, xiiGALShaderTextureType::Texture2DArray, true}] = pTextureView;
  }
  {
    xiiSharedPtr<xiiGALTexture>     pTexture     = CreateTexture(xiiGALResourceDimension::TextureCube, xiiGALSampleCount::OneSample, false);
    xiiSharedPtr<xiiGALTextureView> pTextureView = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);

    s_TextureResourceViews[{xiiGALShaderResourceType::TextureSRV, xiiGALShaderTextureType::TextureCube, false}]             = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureSRV, xiiGALShaderTextureType::TextureCubeArray, false}]        = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureAndSampler, xiiGALShaderTextureType::TextureCube, false}]      = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureAndSampler, xiiGALShaderTextureType::TextureCubeArray, false}] = pTextureView;
  }
  {
    xiiSharedPtr<xiiGALTexture>     pTexture     = CreateTexture(xiiGALResourceDimension::Texture3D, xiiGALSampleCount::OneSample, false);
    xiiSharedPtr<xiiGALTextureView> pTextureView = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);

    s_TextureResourceViews[{xiiGALShaderResourceType::TextureSRV, xiiGALShaderTextureType::Texture3D, false}]        = pTextureView;
    s_TextureResourceViews[{xiiGALShaderResourceType::TextureAndSampler, xiiGALShaderTextureType::Texture3D, false}] = pTextureView;
  }

  {
    xiiGALBufferCreationDescription description;
    description.m_BindFlags           = xiiGALBindFlags::UniformBuffer | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Usage               = xiiGALResourceUsage::Default;
    description.m_Mode                = xiiGALBufferMode::Raw;
    description.m_uiElementByteStride = 0U;
    description.m_uiSize              = 128U;

    xiiSharedPtr<xiiGALBuffer> pBuffer = s_pDevice->CreateBuffer(description);
    XII_ASSERT_DEV(pBuffer != nullptr, "Failed to create fallback buffer resource.");
    pBuffer->SetDebugName("FallbackBuffer");

    s_Buffers.PushBack(pBuffer);

    s_BufferResourceViews[xiiGALShaderResourceType::ConstantBuffer] = pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource);
    s_BufferResourceViews[xiiGALShaderResourceType::BufferSRV]      = pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource);
    s_BufferResourceViews[xiiGALShaderResourceType::BufferUAV]      = pBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess);
  }
}

void xiiRendererFallbackResources::DeInitialize()
{
  s_TextureResourceViews.Clear();
  s_TextureResourceViews.Compact();
  s_BufferResourceViews.Clear();
  s_BufferResourceViews.Compact();

  s_Buffers.Clear();
  s_Buffers.Compact();

  s_Textures.Clear();
  s_Textures.Compact();

  s_pDevice = nullptr;
}

const xiiSharedPtr<xiiGALBufferView> xiiRendererFallbackResources::GetFallbackBuffer(xiiEnum<xiiGALShaderResourceType> resourceType)
{
  if (auto* pView = s_BufferResourceViews.GetValue(resourceType))
  {
    return *pView;
  }
  XII_REPORT_FAILURE("No fallback resource set, update xiiRendererFallbackResources.");
  return {};
}

const xiiSharedPtr<xiiGALTextureView> xiiRendererFallbackResources::GetFallbackTexture(xiiEnum<xiiGALShaderResourceType> resourceType, xiiEnum<xiiGALShaderTextureType> textureType, bool bDepth)
{
  if (auto* pView = s_TextureResourceViews.GetValue(Key{resourceType, textureType, bDepth}))
  {
    return *pView;
  }
  XII_REPORT_FAILURE("No fallback resource set, update xiiRendererFallbackResources.");
  return {};
}

/////////////////////////////////////////////////////////////////////////////////////////

xiiUInt32 xiiRendererFallbackResources::KeyHash::Hash(const Key& a)
{
  xiiHashStreamWriter32 writer;
  writer << a.m_ResourceType.GetValue();
  writer << a.m_xiiType.GetValue();
  writer << a.m_bDepth;
  return writer.GetHashValue();
}

bool xiiRendererFallbackResources::KeyHash::Equal(const Key& a, const Key& b)
{
  return a.m_ResourceType == b.m_ResourceType && a.m_xiiType == b.m_xiiType && a.m_bDepth == b.m_bDepth;
}

xiiUInt32 xiiRendererFallbackResources::KeyHash::Hash(const xiiEnum<xiiGALShaderResourceType>& a)
{
  xiiHashStreamWriter32 writer;
  writer << a.GetValue();
  return writer.GetHashValue();
}

bool xiiRendererFallbackResources::KeyHash::Equal(const xiiEnum<xiiGALShaderResourceType>& a, const xiiEnum<xiiGALShaderResourceType>& b)
{
  return a == b;
}
