/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Identifies the renderer or simulation domain in which a material participates.
struct XII_GRAPHICSCORE_DLL xiiMaterialDomain
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Surface,
    Decal,
    Volume,
    PostProcess,
    Sensor,
    Compute,

    ENUM_COUNT,

    Default = Surface
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialDomain);

/// High-level shading contract used to select shader families and render paths.
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
    ParticipatingMedia,
    XRayAttenuation,
    SensorResponse,
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

/// Features which alter shader specialization or render scheduling.
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
    ReceivesLighting         = XII_BIT(12),
    CastsShadows             = XII_BIT(13),
    WritesVelocity           = XII_BIT(14),
    UsesBindlessResources    = XII_BIT(15),

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
    StorageType ReceivesLighting : 1;
    StorageType CastsShadows : 1;
    StorageType WritesVelocity : 1;
    StorageType UsesBindlessResources : 1;
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

/// CPU authoring type and canonical GPU packing type for a material parameter.
struct XII_GRAPHICSCORE_DLL xiiMaterialParameterType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Bool,
    Int,
    UInt,
    Float,
    Float2,
    Float3,
    Float4,
    Color,
    Matrix3,
    Matrix4,

    ENUM_COUNT,

    Default = Float
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialParameterType);

struct XII_GRAPHICSCORE_DLL xiiMaterialUpdateFrequency
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Static,
    PerFrame,
    PerView,
    PerInstance,

    ENUM_COUNT,

    Default = Static
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialUpdateFrequency);

struct XII_GRAPHICSCORE_DLL xiiMaterialParameterFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None            = 0U,
    EditorVisible   = XII_BIT(0),
    Animatable      = XII_BIT(1),
    RuntimeWritable = XII_BIT(2),
    Specialization  = XII_BIT(3),
    Normalized      = XII_BIT(4),
    HighPrecision   = XII_BIT(5),
    NoSerialize     = XII_BIT(6),

    Default = EditorVisible | RuntimeWritable
  };

  struct Bits
  {
    StorageType EditorVisible : 1;
    StorageType Animatable : 1;
    StorageType RuntimeWritable : 1;
    StorageType Specialization : 1;
    StorageType Normalized : 1;
    StorageType HighPrecision : 1;
    StorageType NoSerialize : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiMaterialParameterFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialParameterFlags);

struct XII_GRAPHICSCORE_DLL xiiMaterialDirtyFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None           = 0U,
    Parameters     = XII_BIT(0),
    Resources      = XII_BIT(1),
    Specialization = XII_BIT(2),
    Pipeline       = XII_BIT(3),
    All            = Parameters | Resources | Specialization | Pipeline,

    Default = None
  };

  struct Bits
  {
    StorageType Parameters : 1;
    StorageType Resources : 1;
    StorageType Specialization : 1;
    StorageType Pipeline : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiMaterialDirtyFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialDirtyFlags);

/// Stable name-derived identifier used instead of string lookup in hot paths.
struct XII_GRAPHICSCORE_DLL xiiMaterialParameterId : public xiiHashableStruct<xiiMaterialParameterId>
{
  XII_DECLARE_POD_TYPE();

  static xiiMaterialParameterId Make(xiiStringView sName);

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiValue != 0U; }

  xiiUInt64 m_uiValue = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialParameterId);

/// Generation-checked handle into GPU-visible material storage.
struct XII_GRAPHICSCORE_DLL xiiMaterialGpuHandle
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiSlot != xiiInvalidIndex; }
  XII_ALWAYS_INLINE void Invalidate()
  {
    m_uiSlot       = xiiInvalidIndex;
    m_uiGeneration = 0U;
  }

  xiiUInt32 m_uiSlot       = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialGpuHandle);

/// Immutable classification used by sorting, shader selection and render-graph routing.
struct XII_GRAPHICSCORE_DLL xiiMaterialRuntimeState
{
  xiiEnum<xiiMaterialDomain>          m_Domain       = xiiMaterialDomain::Surface;
  xiiEnum<xiiMaterialShadingModel>    m_ShadingModel = xiiMaterialShadingModel::Lit;
  xiiEnum<xiiMaterialBlendMode>       m_BlendMode    = xiiMaterialBlendMode::Opaque;
  xiiEnum<xiiMaterialAlphaMode>       m_AlphaMode    = xiiMaterialAlphaMode::Opaque;
  xiiBitflags<xiiMaterialFeatureFlags> m_FeatureFlags = xiiMaterialFeatureFlags::Default;

  xiiUInt64 m_uiPipelineKey = 0U;
  xiiUInt64 m_uiLayoutHash  = 0U;
  xiiUInt32 m_uiRuntimeHash = 0U;
  xiiUInt32 m_uiTextureMask = 0U;
  xiiUInt32 m_uiRevision    = 0U;
  xiiInt16  m_iSortPriority = 0;

  XII_ALWAYS_INLINE bool IsMasked() const { return m_AlphaMode == xiiMaterialAlphaMode::Mask || m_BlendMode == xiiMaterialBlendMode::Masked; }
  XII_ALWAYS_INLINE bool IsTranslucent() const { return m_AlphaMode == xiiMaterialAlphaMode::Blend || m_BlendMode == xiiMaterialBlendMode::Translucent || m_BlendMode == xiiMaterialBlendMode::Additive || m_BlendMode == xiiMaterialBlendMode::Modulate; }
  XII_ALWAYS_INLINE bool IsTwoSided() const { return m_FeatureFlags.IsSet(xiiMaterialFeatureFlags::TwoSided); }
  XII_ALWAYS_INLINE bool UsesTexture(xiiMaterialTextureSlot::Enum slot) const { return (m_uiTextureMask & (1U << static_cast<xiiUInt8>(slot))) != 0U; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialRuntimeState);

