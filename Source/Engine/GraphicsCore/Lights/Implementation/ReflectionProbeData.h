#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/TagSet.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiReflectionProbeMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Static,
    Dynamic,

    Default = Static
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiReflectionProbeMode);

/// \brief Describes how a cube map should be generated.
struct XII_GRAPHICSCORE_DLL xiiReflectionProbeDesc
{
  xiiUuid m_uniqueID;

  xiiTagSet m_IncludeTags;
  xiiTagSet m_ExcludeTags;

  xiiEnum<xiiReflectionProbeMode> m_Mode;

  bool m_bShowDebugInfo = false;
  bool m_bShowMipMaps   = false;

  float   m_fIntensity     = 1.0f;
  float   m_fSaturation    = 1.0f;
  float   m_fNearPlane     = 0.0f;
  float   m_fFarPlane      = 100.0f;
  xiiVec3 m_vCaptureOffset = xiiVec3::MakeZero();
};

using xiiReflectionProbeId = xiiGenericId<24, 8>;

template <>
struct xiiHashHelper<xiiReflectionProbeId>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiReflectionProbeId value) { return xiiHashHelper<xiiUInt32>::Hash(value.m_Data); }

  XII_ALWAYS_INLINE static bool Equal(xiiReflectionProbeId a, xiiReflectionProbeId b) { return a == b; }
};

/// \brief Render data for a reflection probe.
class XII_GRAPHICSCORE_DLL xiiReflectionProbeRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReflectionProbeRenderData, xiiRenderData);

public:
  xiiReflectionProbeRenderData()
  {
    m_Id.Invalidate();
    m_vHalfExtents.SetZero();
  }

  xiiReflectionProbeId m_Id;
  xiiUInt32            m_uiIndex = 0;
  xiiVec3              m_vProbePosition; ///< Probe position in world space.
  xiiVec3              m_vHalfExtents;
  xiiVec3              m_vPositiveFalloff;
  xiiVec3              m_vNegativeFalloff;
  xiiVec3              m_vInfluenceScale;
  xiiVec3              m_vInfluenceShift;
};

/// \brief A unique reference to a reflection probe.
struct xiiReflectionProbeRef
{
  bool operator==(const xiiReflectionProbeRef& b) const
  {
    return m_Id == b.m_Id && m_uiWorldIndex == b.m_uiWorldIndex;
  }

  xiiUInt32            m_uiWorldIndex = 0;
  xiiReflectionProbeId m_Id;
};
XII_CHECK_AT_COMPILETIME(sizeof(xiiReflectionProbeRef) == 8);

template <>
struct xiiHashHelper<xiiReflectionProbeRef>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiReflectionProbeRef value) { return xiiHashHelper<xiiUInt64>::Hash(reinterpret_cast<xiiUInt64&>(value)); }

  XII_ALWAYS_INLINE static bool Equal(xiiReflectionProbeRef a, xiiReflectionProbeRef b) { return a.m_Id == b.m_Id && a.m_uiWorldIndex == b.m_uiWorldIndex; }
};

/// \brief Flags that describe a reflection probe.
struct xiiProbeFlags
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    SkyLight         = XII_BIT(0),
    HasCustomCubeMap = XII_BIT(1),
    Sphere           = XII_BIT(2),
    Box              = XII_BIT(3),
    Dynamic          = XII_BIT(4),
    Default          = 0
  };

  struct Bits
  {
    StorageType SkyLight : 1;
    StorageType HasCustomCubeMap : 1;
    StorageType Sphere : 1;
    StorageType Box : 1;
    StorageType Dynamic : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiProbeFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiProbeFlags);
