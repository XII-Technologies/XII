#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <ProcGenPlugin/ProcGenPluginDLL.h>

class xiiExpressionByteCode;
using xiiColorGradientResourceHandle = xiiTypedResourceHandle<class xiiColorGradientResource>;
using xiiPrefabResourceHandle        = xiiTypedResourceHandle<class xiiPrefabResource>;
using xiiSurfaceResourceHandle       = xiiTypedResourceHandle<class xiiSurfaceResource>;

struct xiiProcGenBinaryOperator
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    Min,

    Default = Multiply
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PROCGENPLUGIN_DLL, xiiProcGenBinaryOperator);

struct xiiProcGenBlendMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    Min,
    Set,

    Default = Multiply
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PROCGENPLUGIN_DLL, xiiProcGenBlendMode);

struct xiiProcVertexColorChannelMapping
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    R,
    G,
    B,
    A,
    Black,
    White,

    Default = R
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PROCGENPLUGIN_DLL, xiiProcVertexColorChannelMapping);

struct xiiProcVertexColorMapping
{
  xiiEnum<xiiProcVertexColorChannelMapping> m_R = xiiProcVertexColorChannelMapping::R;
  xiiEnum<xiiProcVertexColorChannelMapping> m_G = xiiProcVertexColorChannelMapping::G;
  xiiEnum<xiiProcVertexColorChannelMapping> m_B = xiiProcVertexColorChannelMapping::B;
  xiiEnum<xiiProcVertexColorChannelMapping> m_A = xiiProcVertexColorChannelMapping::A;

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PROCGENPLUGIN_DLL, xiiProcVertexColorMapping);

struct xiiProcPlacementMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Raycast,
    Fixed,

    Default = Raycast
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PROCGENPLUGIN_DLL, xiiProcPlacementMode);

struct xiiProcVolumeImageMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    ReferenceColor,
    ChannelR,
    ChannelG,
    ChannelB,
    ChannelA,

    Default = ReferenceColor
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PROCGENPLUGIN_DLL, xiiProcVolumeImageMode);

//////////////////////////////////////////////////////////////////////////

namespace xiiProcGenInternal
{
  class PlacementTile;
  class FindPlacementTilesTask;
  class PreparePlacementTask;
  class PlacementTask;
  class VertexColorTask;
  struct PlacementData;

  struct InvalidatedArea
  {
    xiiBoundingBox m_Box;
    xiiWorld*      m_pWorld = nullptr;
  };

  struct Pattern
  {
    struct Point
    {
      xiiVec2 m_Coordinates;
      float   m_fThreshold;
    };

    xiiArrayPtr<Point> m_Points;
    float              m_fSize;
  };

  struct XII_PROCGENPLUGIN_DLL GraphSharedDataBase : public xiiRefCounted
  {
    virtual ~GraphSharedDataBase();
  };

  struct Output : public xiiRefCounted
  {
    virtual ~Output();

    xiiHashedString m_sName;

    xiiHybridArray<xiiUInt8, 4>             m_VolumeTagSetIndices;
    xiiSharedPtr<const GraphSharedDataBase> m_pGraphSharedData;

    xiiUniquePtr<xiiExpressionByteCode> m_pByteCode;
  };

  struct PlacementOutput : public Output
  {
    float GetTileSize() const { return m_pPattern->m_fSize * m_fFootprint; }

    bool IsValid() const
    {
      return !m_ObjectsToPlace.IsEmpty() && m_pPattern != nullptr && m_fFootprint > 0.0f && m_fCullDistance > 0.0f && m_pByteCode != nullptr;
    }

    xiiHybridArray<xiiPrefabResourceHandle, 4> m_ObjectsToPlace;

    const Pattern* m_pPattern   = nullptr;
    float          m_fFootprint = 1.0f;

    xiiVec3 m_vMinOffset = xiiVec3::ZeroVector();
    xiiVec3 m_vMaxOffset = xiiVec3::ZeroVector();

    xiiAngle m_YawRotationSnap = xiiAngle::Radian(0.0f);
    float    m_fAlignToNormal  = 1.0f;

    xiiVec3 m_vMinScale = xiiVec3(1.0f);
    xiiVec3 m_vMaxScale = xiiVec3(1.0f);

    float m_fCullDistance = 30.0f;

    xiiUInt32 m_uiCollisionLayer = 0;

    xiiColorGradientResourceHandle m_hColorGradient;

    xiiSurfaceResourceHandle m_hSurface;

    xiiEnum<xiiProcPlacementMode> m_Mode;
  };

  struct VertexColorOutput : public Output
  {
  };

  struct XII_PROCGENPLUGIN_DLL ExpressionInputs
  {
    static xiiHashedString s_sPosition;
    static xiiHashedString s_sPositionX;
    static xiiHashedString s_sPositionY;
    static xiiHashedString s_sPositionZ;
    static xiiHashedString s_sNormal;
    static xiiHashedString s_sNormalX;
    static xiiHashedString s_sNormalY;
    static xiiHashedString s_sNormalZ;
    static xiiHashedString s_sColor;
    static xiiHashedString s_sColorR;
    static xiiHashedString s_sColorG;
    static xiiHashedString s_sColorB;
    static xiiHashedString s_sColorA;
    static xiiHashedString s_sPointIndex;
  };

  struct XII_PROCGENPLUGIN_DLL ExpressionOutputs
  {
    static xiiHashedString s_sOutDensity;
    static xiiHashedString s_sOutScale;
    static xiiHashedString s_sOutColorIndex;
    static xiiHashedString s_sOutObjectIndex;

    static xiiHashedString s_sOutColor;
    static xiiHashedString s_sOutColorR;
    static xiiHashedString s_sOutColorG;
    static xiiHashedString s_sOutColorB;
    static xiiHashedString s_sOutColorA;
  };

  struct PlacementPoint
  {
    XII_DECLARE_POD_TYPE();

    xiiVec3   m_vPosition;
    float     m_fScale;
    xiiVec3   m_vNormal;
    xiiUInt8  m_uiColorIndex;
    xiiUInt8  m_uiObjectIndex;
    xiiUInt16 m_uiPointIndex;
  };

  struct PlacementTransform
  {
    XII_DECLARE_POD_TYPE();

    xiiSimdTransform  m_Transform;
    xiiColorLinear16f m_ObjectColor;
    xiiUInt16         m_uiPointIndex;
    xiiUInt8          m_uiObjectIndex;
    bool              m_bHasValidColor;
    xiiUInt32         m_uiPadding;
  };

  struct PlacementTileDesc
  {
    xiiComponentHandle m_hComponent;
    xiiUInt32          m_uiOutputIndex;
    xiiInt32           m_iPosX;
    xiiInt32           m_iPosY;
    float              m_fMinZ;
    float              m_fMaxZ;
    float              m_fTileSize;
    float              m_fDistanceToCamera;

    bool operator==(const PlacementTileDesc& other) const
    {
      return m_hComponent == other.m_hComponent && m_uiOutputIndex == other.m_uiOutputIndex && m_iPosX == other.m_iPosX && m_iPosY == other.m_iPosY;
    }

    xiiBoundingBox GetBoundingBox() const
    {
      xiiVec2 vCenter = xiiVec2(m_iPosX * m_fTileSize, m_iPosY * m_fTileSize);
      xiiVec3 vMin    = (vCenter - xiiVec2(m_fTileSize * 0.5f)).GetAsVec3(m_fMinZ);
      xiiVec3 vMax    = (vCenter + xiiVec2(m_fTileSize * 0.5f)).GetAsVec3(m_fMaxZ);

      return xiiBoundingBox(vMin, vMax);
    }

    xiiHybridArray<xiiSimdMat4f, 8, xiiAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;
  };
} // namespace xiiProcGenInternal
