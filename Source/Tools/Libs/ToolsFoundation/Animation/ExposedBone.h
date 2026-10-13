/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// Tooling value used by asset metadata and bone property editors. Does not own a runtime skeleton or pose.
/// The reflected name, version, properties, and binary field order preserve existing asset metadata.
struct XII_TOOLSFOUNDATION_DLL xiiExposedBone
{
  xiiString    m_sName;
  xiiString    m_sParent;
  xiiTransform m_Transform = xiiTransform::MakeIdentity();

  /// Computes each bone's parent-to-model matrix in input order, allowing unsorted and disconnected hierarchies.
  /// Empty parent names denote roots. Empty/duplicate names, missing parents, cycles, and non-finite transforms
  /// fail and clear out_parentTransforms. Uses O(n) expected time and O(n) scratch memory without recursion.
  /// Any asset import/root transform is applied by the consumer, outside these local bone transforms.
  static xiiResult ComputeParentTransforms(xiiArrayPtr<const xiiExposedBone> bones, xiiDynamicArray<xiiMat4>& out_parentTransforms);
};

XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiExposedBone);
XII_DECLARE_REFLECTABLE_TYPE(XII_TOOLSFOUNDATION_DLL, xiiExposedBone);

XII_TOOLSFOUNDATION_DLL void operator<<(xiiStreamWriter& inout_stream, const xiiExposedBone& bone);
XII_TOOLSFOUNDATION_DLL void operator>>(xiiStreamReader& inout_stream, xiiExposedBone& out_bone);
XII_TOOLSFOUNDATION_DLL bool operator==(const xiiExposedBone& lhs, const xiiExposedBone& rhs);

template <>
struct xiiHashHelper<xiiExposedBone>
{
  XII_TOOLSFOUNDATION_DLL static xiiUInt32 Hash(const xiiExposedBone& value);
  XII_ALWAYS_INLINE static bool            Equal(const xiiExposedBone& a, const xiiExposedBone& b) { return a == b; }
};
