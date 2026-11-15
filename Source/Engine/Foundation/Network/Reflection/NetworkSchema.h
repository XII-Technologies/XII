/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>

/// Version-one wire kinds; numeric values are independent of xiiVariantType and compiler RTTI.
enum class xiiNetworkValueKind : xiiUInt8
{
  Invalid           = 0,
  Bool              = 1,
  Int8              = 2,
  Int16             = 3,
  Int32             = 4,
  Int64             = 5,
  UInt8             = 6,
  UInt16            = 7,
  UInt32            = 8,
  UInt64            = 9,
  Float             = 10,
  Double            = 11,
  String            = 12,
  Bytes             = 13,
  Vector2           = 14,
  Vector3           = 15,
  Vector4           = 16,
  Vector2d          = 17,
  Vector3d          = 18,
  Vector4d          = 19,
  Vector2I          = 20,
  Vector3I          = 21,
  Vector4I          = 22,
  Vector2U          = 23,
  Vector3U          = 24,
  Vector4U          = 25,
  Quaternion        = 26,
  Quaterniond       = 27,
  Matrix3           = 28,
  Matrix4           = 29,
  Matrix3d          = 30,
  Matrix4d          = 31,
  Transform         = 32,
  Transformd        = 33,
  Color             = 34,
  ColorGamma        = 35,
  Angle             = 36,
  Angled            = 37,
  Time              = 38,
  Timestamp         = 39,
  Uuid              = 40,
  Variant           = 41,
  VariantArray      = 42,
  VariantDictionary = 43,
  Structure         = 44,
  Custom            = 45,
  Vector2I64        = 46,
  Vector3I64        = 47,
  Vector4I64        = 48,
  Vector2U64        = 49,
  Vector3U64        = 50,
  Vector4U64        = 51,
  HashedString      = 52
};

enum class xiiNetworkFieldCategory : xiiUInt8
{
  Member = 0,
  Array  = 1,
  Map    = 2
};

struct XII_FOUNDATION_DLL xiiNetworkFieldSchema
{
  xiiString               m_sName;
  xiiUInt64               m_uiId                 = 0;
  xiiUInt64               m_uiWireSignature      = 0;
  xiiUInt64               m_uiNestedTypeId       = 0;
  xiiUInt64               m_uiCodecId            = 0;
  xiiUInt32               m_uiCodecVersion       = 0;
  xiiNetworkValueKind     m_Kind                 = xiiNetworkValueKind::Invalid;
  xiiNetworkFieldCategory m_Category             = xiiNetworkFieldCategory::Member;
  bool                    m_bOptional            = false;
  xiiUInt32               m_uiMaximumElements    = 4096;
  xiiUInt32               m_uiMaximumStringBytes = 65536;
  xiiUInt8                m_uiQuantizationBits   = 0;
  double                  m_fMinimum             = 0;
  double                  m_fMaximum             = 1;
};

/// Immutable portable snapshot returned by the registry. Contains no borrowed RTTI or code pointers;
/// snapshots may outlive plugin unload. A snapshot alone does not authorize remote construction.
class XII_FOUNDATION_DLL xiiNetworkSchema final : public xiiRefCountingImpl
{
public:
  xiiString                              m_sTypeName;
  xiiUInt64                              m_uiTypeId      = 0;
  xiiUInt32                              m_uiTypeVersion = 0;
  xiiUInt64                              m_uiFingerprint = 0;
  xiiDynamicArray<xiiNetworkFieldSchema> m_Fields;
  /// Exact recursive descriptor used for compact agreement, including nested fingerprints.
  xiiDynamicArray<xiiUInt8> m_Descriptor;
};
