#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Shader/ShaderByteCode.h>

/// \brief Creates fallback resources in case the high-level renderer did not map a resource to a binding slot.
class XII_GRAPHICSCORE_DLL xiiRendererFallbackResources
{
public:
  static const xiiSharedPtr<xiiGALBufferView>  GetFallbackBuffer(xiiEnum<xiiGALShaderResourceType> resourceType);
  static const xiiSharedPtr<xiiGALTextureView> GetFallbackTexture(xiiEnum<xiiGALShaderResourceType> resourceType, xiiEnum<xiiGALShaderTextureType> textureType, bool bDepth);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, FallbackResources);

  static void Initialize();
  static void DeInitialize();

  static xiiSharedPtr<xiiGALDevice> s_pDevice;

  struct Key
  {
    XII_DECLARE_POD_TYPE();

    xiiEnum<xiiGALShaderResourceType> m_ResourceType;
    xiiEnum<xiiGALShaderTextureType>  m_xiiType;
    bool                              m_bDepth = false;
  };

  struct KeyHash
  {
    static xiiUInt32 Hash(const Key& a);
    static bool      Equal(const Key& a, const Key& b);

    static xiiUInt32 Hash(const xiiEnum<xiiGALShaderResourceType>& a);
    static bool      Equal(const xiiEnum<xiiGALShaderResourceType>& a, const xiiEnum<xiiGALShaderResourceType>& b);
  };

  static xiiHashTable<Key, xiiSharedPtr<xiiGALTextureView>, KeyHash>                              s_TextureResourceViews;
  static xiiHashTable<xiiEnum<xiiGALShaderResourceType>, xiiSharedPtr<xiiGALBufferView>, KeyHash> s_BufferResourceViews;

  static xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>>  s_Buffers;
  static xiiDynamicArray<xiiSharedPtr<xiiGALTexture>> s_Textures;
};
