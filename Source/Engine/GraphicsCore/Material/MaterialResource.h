#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Shader/ShaderResource.h>

using xiiMaterialResourceHandle    = xiiTypedResourceHandle<class xiiMaterialResource>;
using xiiTexture2DResourceHandle   = xiiTypedResourceHandle<class xiiTexture2DResource>;
using xiiTextureCubeResourceHandle = xiiTypedResourceHandle<class xiiTextureCubeResource>;

struct xiiMaterialResourceDescriptor
{
  struct Parameter
  {
    xiiHashedString m_Name;
    xiiVariant      m_Value;

    XII_FORCE_INLINE bool operator==(const Parameter& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct Texture2DBinding
  {
    xiiHashedString            m_Name;
    xiiTexture2DResourceHandle m_Value;

    XII_FORCE_INLINE bool operator==(const Texture2DBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct TextureCubeBinding
  {
    xiiHashedString              m_Name;
    xiiTextureCubeResourceHandle m_Value;

    XII_FORCE_INLINE bool operator==(const TextureCubeBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  void Clear();

  bool operator==(const xiiMaterialResourceDescriptor& other) const;

  xiiMaterialResourceHandle m_hBaseMaterial;
  // xiiSurfaceResource is not linked into this project (not true anymore -> could be changed)
  // this is not used for game purposes but rather for automatic collision mesh generation, so we only store the asset ID here
  xiiHashedString                     m_sSurface;
  xiiShaderResourceHandle             m_hShader;
  xiiDynamicArray<xiiPermutationVar>  m_PermutationVars;
  xiiDynamicArray<Parameter>          m_Parameters;
  xiiDynamicArray<Texture2DBinding>   m_Texture2DBindings;
  xiiDynamicArray<TextureCubeBinding> m_TextureCubeBindings;
  xiiRenderData::Category             m_RenderDataCategory;
};

class XII_GRAPHICSCORE_DLL xiiMaterialResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMaterialResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiMaterialResource, xiiMaterialResourceDescriptor);

public:
  xiiMaterialResource();
  ~xiiMaterialResource();

  xiiHashedString GetPermutationValue(const xiiTempHashedString& sName);
  xiiHashedString GetSurface() const;

  void       SetParameter(const xiiHashedString& sName, const xiiVariant& value);
  void       SetParameter(const char* szName, const xiiVariant& value);
  xiiVariant GetParameter(const xiiTempHashedString& sName);

  void                       SetTexture2DBinding(const xiiHashedString& sName, const xiiTexture2DResourceHandle& value);
  void                       SetTexture2DBinding(const char* szName, const xiiTexture2DResourceHandle& value);
  xiiTexture2DResourceHandle GetTexture2DBinding(const xiiTempHashedString& sName);

  void                         SetTextureCubeBinding(const xiiHashedString& sName, const xiiTextureCubeResourceHandle& value);
  void                         SetTextureCubeBinding(const char* szName, const xiiTextureCubeResourceHandle& value);
  xiiTextureCubeResourceHandle GetTextureCubeBinding(const xiiTempHashedString& sName);

  xiiRenderData::Category GetRenderDataCategory();

  /// \brief Copies current desc to original desc so the material is not modified on reset
  void         PreserveCurrentDesc();
  virtual void ResetResource() override;

  const xiiMaterialResourceDescriptor& GetCurrentDesc() const;

  /// \brief Use these enum values together with GetDefaultMaterialFileName() to get the default file names for these material types.
  enum class DefaultMaterialType
  {
    Fullbright,
    FullbrightAlphaTest,
    Lit,
    LitAlphaTest,
    Sky,
    MissingMaterial
  };

  /// \brief Returns the default material file name for the given type (materials in Data/Base/Materials/BaseMaterials).
  static const char* GetDefaultMaterialFileName(DefaultMaterialType materialType);

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiMaterialResourceDescriptor m_mOriginalDesc; // stores the state at loading, such that SetParameter etc. calls can be reset later
  xiiMaterialResourceDescriptor m_mDesc;

  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, MaterialResource);

  xiiEvent<const xiiMaterialResource*, xiiMutex> m_ModifiedEvent;
  void                                           OnBaseMaterialModified(const xiiMaterialResource* pModifiedMaterial);
  void                                           OnResourceEvent(const xiiResourceEvent& resourceEvent);

  void AddPermutationVar(xiiStringView sName, xiiStringView sValue);

  xiiAtomicInteger32 m_iLastModified;
  xiiAtomicInteger32 m_iLastConstantsModified;
  xiiInt32           m_iLastUpdated;
  xiiInt32           m_iLastConstantsUpdated;

  bool IsModified();
  bool AreConstantsModified();

  void UpdateConstantBuffer(xiiShaderPermutationResource* pShaderPermutation);

  xiiConstantBufferStorageHandle m_hConstantBufferStorage;

  struct CachedValues
  {
    xiiShaderResourceHandle                                     m_hShader;
    xiiHashTable<xiiHashedString, xiiHashedString>              m_PermutationVars;
    xiiHashTable<xiiHashedString, xiiVariant>                   m_Parameters;
    xiiHashTable<xiiHashedString, xiiTexture2DResourceHandle>   m_Texture2DBindings;
    xiiHashTable<xiiHashedString, xiiTextureCubeResourceHandle> m_TextureCubeBindings;
    xiiRenderData::Category                                     m_RenderDataCategory;

    void Reset();
  };

  xiiUInt32     m_uiCacheIndex;
  CachedValues* m_pCachedValues;

  CachedValues*        GetOrUpdateCachedValues();
  static CachedValues* AllocateCache(xiiUInt32& inout_uiCacheIndex);
  static void          DeallocateCache(xiiUInt32 uiCacheIndex);

  xiiMutex                                           m_UpdateCacheMutex;
  static xiiDeque<xiiMaterialResource::CachedValues> s_CachedValues;

  static void ClearCache();
};
