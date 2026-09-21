/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>

using xiiMaterialResourceHandle    = xiiTypedResourceHandle<class xiiMaterialResource>;
using xiiTexture2DResourceHandle   = xiiTypedResourceHandle<class xiiTexture2DResource>;
using xiiTextureCubeResourceHandle = xiiTypedResourceHandle<class xiiTextureCubeResource>;

struct XII_GRAPHICSCORE_DLL xiiMaterialShadingModel
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Lit,
    Subsurface,
    ClearCoat,
    Cloth,
    Hair,
    Eye,
    Unlit,
    Custom,

    ENUM_COUNT,

    Default = Lit
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialShadingModel);

struct XII_GRAPHICSCORE_DLL xiiMaterialBlendMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Opaque,
    Masked,
    Translucent,
    Additive,
    Modulate,

    ENUM_COUNT,

    Default = Opaque
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialBlendMode);

struct XII_GRAPHICSCORE_DLL xiiMaterialAlphaMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Opaque,
    Mask,
    Blend,

    ENUM_COUNT,

    Default = Opaque
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialAlphaMode);

struct XII_GRAPHICSCORE_DLL xiiMaterialFeatureFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None                     = 0U,
    NormalTexture            = XII_BIT(0),
    MetallicRoughnessTexture = XII_BIT(1),
    OcclusionTexture         = XII_BIT(2),
    EmissiveTexture          = XII_BIT(3),
    HeightTexture            = XII_BIT(4),
    ClearCoat                = XII_BIT(5),
    Transmission             = XII_BIT(6),
    Sheen                    = XII_BIT(7),
    Anisotropy               = XII_BIT(8),
    VertexColor              = XII_BIT(9),
    TwoSided                 = XII_BIT(10),
    RuntimeGenerated         = XII_BIT(11),

    Default = None
  };

  struct Bits
  {
    StorageType NormalTexture : 1;
    StorageType MetallicRoughnessTexture : 1;
    StorageType OcclusionTexture : 1;
    StorageType EmissiveTexture : 1;
    StorageType HeightTexture : 1;
    StorageType ClearCoat : 1;
    StorageType Transmission : 1;
    StorageType Sheen : 1;
    StorageType Anisotropy : 1;
    StorageType VertexColor : 1;
    StorageType TwoSided : 1;
    StorageType RuntimeGenerated : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiMaterialFeatureFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialFeatureFlags);

struct XII_GRAPHICSCORE_DLL xiiMaterialTextureSlot
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    BaseColor,
    Normal,
    MetallicRoughness,
    Occlusion,
    Emissive,
    Height,
    ClearCoat,
    Transmission,

    ENUM_COUNT,

    Default = BaseColor
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialTextureSlot);

struct XII_GRAPHICSCORE_DLL xiiMaterialRuntimeState
{
  xiiEnum<xiiMaterialShadingModel>     m_ShadingModel = xiiMaterialShadingModel::Lit;
  xiiEnum<xiiMaterialBlendMode>        m_BlendMode    = xiiMaterialBlendMode::Opaque;
  xiiEnum<xiiMaterialAlphaMode>        m_AlphaMode    = xiiMaterialAlphaMode::Opaque;
  xiiBitflags<xiiMaterialFeatureFlags> m_FeatureFlags = xiiMaterialFeatureFlags::Default;

  xiiUInt32 m_uiRuntimeHash = 0U;
  xiiUInt32 m_uiPipelineKey = 0U;
  xiiUInt32 m_uiTextureMask = 0U;
  xiiInt16  m_iSortPriority = 0;

  XII_ALWAYS_INLINE bool IsMasked() const { return m_AlphaMode == xiiMaterialAlphaMode::Mask || m_BlendMode == xiiMaterialBlendMode::Masked; }
  XII_ALWAYS_INLINE bool IsTranslucent() const { return m_AlphaMode == xiiMaterialAlphaMode::Blend || m_BlendMode == xiiMaterialBlendMode::Translucent || m_BlendMode == xiiMaterialBlendMode::Additive || m_BlendMode == xiiMaterialBlendMode::Modulate; }
  XII_ALWAYS_INLINE bool IsTwoSided() const { return m_FeatureFlags.IsSet(xiiMaterialFeatureFlags::TwoSided); }
  XII_ALWAYS_INLINE bool UsesTexture(xiiMaterialTextureSlot::Enum slot) const { return (m_uiTextureMask & (1U << static_cast<xiiUInt8>(slot))) != 0U; }
};

struct xiiMaterialResourceDescriptor
{
  struct Parameter
  {
    xiiHashedString m_Name;
    xiiVariant      m_Value;

    XII_ALWAYS_INLINE bool operator==(const Parameter& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct Texture2DBinding
  {
    xiiHashedString            m_Name;
    xiiTexture2DResourceHandle m_Value;

    XII_ALWAYS_INLINE bool operator==(const Texture2DBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct TextureCubeBinding
  {
    xiiHashedString              m_Name;
    xiiTextureCubeResourceHandle m_Value;

    XII_ALWAYS_INLINE bool operator==(const TextureCubeBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  void Clear();

  void                    ApplyPbrParameterDefaults(bool bOnlyIfMissing = true);
  xiiMaterialRuntimeState BuildRuntimeState() const;
  xiiUInt32               ComputeRuntimeHash() const;
  void                    RecomputeRuntimeHash();

  XII_ALWAYS_INLINE bool operator==(const xiiMaterialResourceDescriptor& other) const
  {
    return m_hBaseMaterial == other.m_hBaseMaterial && m_sSurface == other.m_sSurface && m_hShader == other.m_hShader && m_ShadingModel == other.m_ShadingModel && m_BlendMode == other.m_BlendMode && m_AlphaMode == other.m_AlphaMode && m_FeatureFlags == other.m_FeatureFlags && m_BaseColor == other.m_BaseColor && m_EmissiveColor == other.m_EmissiveColor && m_fMetallic == other.m_fMetallic && m_fRoughness == other.m_fRoughness && m_fOcclusionStrength == other.m_fOcclusionStrength && m_fAlphaCutoff == other.m_fAlphaCutoff && m_fNormalScale == other.m_fNormalScale && m_fDisplacementScale == other.m_fDisplacementScale && m_fClearCoat == other.m_fClearCoat && m_fClearCoatRoughness == other.m_fClearCoatRoughness && m_fTransmission == other.m_fTransmission && m_fThickness == other.m_fThickness && m_fIndexOfRefraction == other.m_fIndexOfRefraction && m_fAnisotropy == other.m_fAnisotropy && m_fSheenRoughness == other.m_fSheenRoughness && m_iSortPriority == other.m_iSortPriority && m_hBaseColorTexture == other.m_hBaseColorTexture && m_hNormalTexture == other.m_hNormalTexture && m_hMetallicRoughnessTexture == other.m_hMetallicRoughnessTexture && m_hOcclusionTexture == other.m_hOcclusionTexture && m_hEmissiveTexture == other.m_hEmissiveTexture && m_hHeightTexture == other.m_hHeightTexture && m_hClearCoatTexture == other.m_hClearCoatTexture && m_hTransmissionTexture == other.m_hTransmissionTexture && m_PermutationVariables == other.m_PermutationVariables && m_Parameters == other.m_Parameters && m_Texture2DBindings == other.m_Texture2DBindings && m_TextureCubeBindings == other.m_TextureCubeBindings;
  }

  xiiMaterialResourceHandle m_hBaseMaterial;
  // xiiSurfaceResource is not linked into this project (not true anymore -> could be changed)
  // this is not used for game purposes but rather for automatic collision mesh generation, so we only store the asset ID here
  xiiHashedString         m_sSurface;
  xiiShaderResourceHandle m_hShader;

  xiiEnum<xiiMaterialShadingModel>     m_ShadingModel = xiiMaterialShadingModel::Lit;
  xiiEnum<xiiMaterialBlendMode>        m_BlendMode    = xiiMaterialBlendMode::Opaque;
  xiiEnum<xiiMaterialAlphaMode>        m_AlphaMode    = xiiMaterialAlphaMode::Opaque;
  xiiBitflags<xiiMaterialFeatureFlags> m_FeatureFlags = xiiMaterialFeatureFlags::Default;

  xiiColor m_BaseColor           = xiiColor::White;
  xiiColor m_EmissiveColor       = xiiColor::Black;
  float    m_fMetallic           = 0.0f;
  float    m_fRoughness          = 0.5f;
  float    m_fOcclusionStrength  = 1.0f;
  float    m_fAlphaCutoff        = 0.5f;
  float    m_fNormalScale        = 1.0f;
  float    m_fDisplacementScale  = 0.0f;
  float    m_fClearCoat          = 0.0f;
  float    m_fClearCoatRoughness = 0.0f;
  float    m_fTransmission       = 0.0f;
  float    m_fThickness          = 0.0f;
  float    m_fIndexOfRefraction  = 1.5f;
  float    m_fAnisotropy         = 0.0f;
  float    m_fSheenRoughness     = 0.5f;
  xiiInt16 m_iSortPriority       = 0;

  xiiTexture2DResourceHandle m_hBaseColorTexture;
  xiiTexture2DResourceHandle m_hNormalTexture;
  xiiTexture2DResourceHandle m_hMetallicRoughnessTexture;
  xiiTexture2DResourceHandle m_hOcclusionTexture;
  xiiTexture2DResourceHandle m_hEmissiveTexture;
  xiiTexture2DResourceHandle m_hHeightTexture;
  xiiTexture2DResourceHandle m_hClearCoatTexture;
  xiiTexture2DResourceHandle m_hTransmissionTexture;

  xiiUInt32 m_uiRuntimeHash = 0U;

  xiiDynamicArray<xiiGALPermutationVariable> m_PermutationVariables;
  xiiDynamicArray<Parameter>                 m_Parameters;
  xiiDynamicArray<Texture2DBinding>          m_Texture2DBindings;
  xiiDynamicArray<TextureCubeBinding>        m_TextureCubeBindings;
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

  xiiEnum<xiiMaterialShadingModel>     GetShadingModel() const;
  xiiEnum<xiiMaterialBlendMode>        GetBlendMode() const;
  xiiEnum<xiiMaterialAlphaMode>        GetAlphaMode() const;
  xiiBitflags<xiiMaterialFeatureFlags> GetFeatureFlags() const;
  const xiiMaterialRuntimeState&       GetRuntimeState() const;
  xiiUInt32                            GetRuntimeHash() const;
  xiiUInt32                            GetTextureMask() const;
  bool                                 IsTranslucent() const;

  void       SetParameter(const xiiHashedString& sName, const xiiVariant& value);
  void       SetParameter(xiiStringView sName, const xiiVariant& value);
  xiiVariant GetParameter(const xiiTempHashedString& sName);

  void                       SetTexture2DBinding(const xiiHashedString& sName, const xiiTexture2DResourceHandle& value);
  void                       SetTexture2DBinding(xiiStringView sName, const xiiTexture2DResourceHandle& value);
  xiiTexture2DResourceHandle GetTexture2DBinding(const xiiTempHashedString& sName);

  void                         SetTextureCubeBinding(const xiiHashedString& sName, const xiiTextureCubeResourceHandle& value);
  void                         SetTextureCubeBinding(xiiStringView sName, const xiiTextureCubeResourceHandle& value);
  xiiTextureCubeResourceHandle GetTextureCubeBinding(const xiiTempHashedString& sName);

  /// Copies current description to the loading description so the material is not modified on reset.
  void         PreserveCurrentDescription();
  virtual void ResetResource() override;

  const xiiMaterialResourceDescriptor& GetCurrentDescription() const;

  /// Use these enum values together with GetDefaultMaterialFileName() to get the default file names for these material types.
  enum class DefaultMaterialType
  {
    Fullbright,
    FullbrightAlphaTest,
    Lit,
    LitAlphaTest,
    Sky,
    MissingMaterial
  };

  /// Returns the default material file name for the given type (materials in Data/Base/Materials/BaseMaterials).
  static xiiStringView GetDefaultMaterialFileName(DefaultMaterialType materialType);

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  friend class xiiRenderContext;

  xiiMaterialResourceDescriptor m_LoadingDescription; // stores the state at loading, such that SetParameter etc. calls can be reset later
  xiiMaterialResourceDescriptor m_Description;
  xiiMaterialRuntimeState       m_RuntimeState;

  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, MaterialResource);

  xiiEvent<const xiiMaterialResource*, xiiMutex> m_ModifiedEvent;
  void                                           OnBaseMaterialModified(const xiiMaterialResource* pModifiedMaterial);
  void                                           OnResourceEvent(const xiiResourceEvent& resourceEvent);

  void AddPermutationVariable(xiiStringView sName, xiiStringView sValue);
  void UpdateRuntimeState();

  xiiAtomicInteger32 m_iLastModified;
  xiiAtomicInteger32 m_iLastConstantsModified;
  xiiInt32           m_iLastUpdated;
  xiiInt32           m_iLastConstantsUpdated;

  bool IsModified();
  bool AreConstantsModified();

  void UpdateConstantBuffer(xiiShaderPermutationResource* pShaderPermutation);

  xiiSharedPtr<xiiGALBuffer> m_pMaterialConstantsBuffer;
  xiiArrayPtr<xiiUInt8>      m_pMaterialData;

  struct CachedValues
  {
    xiiShaderResourceHandle                                     m_hShader;
    xiiHashTable<xiiHashedString, xiiHashedString>              m_PermutationVariables;
    xiiHashTable<xiiHashedString, xiiVariant>                   m_Parameters;
    xiiHashTable<xiiHashedString, xiiTexture2DResourceHandle>   m_Texture2DBindings;
    xiiHashTable<xiiHashedString, xiiTextureCubeResourceHandle> m_TextureCubeBindings;

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
