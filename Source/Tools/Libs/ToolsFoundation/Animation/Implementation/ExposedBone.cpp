/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Animation/ExposedBone.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiExposedBone, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiExposedBone>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Parent", m_sParent),
    XII_MEMBER_PROPERTY("Transform", m_Transform),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiExposedBone);

void operator<<(xiiStreamWriter& inout_stream, const xiiExposedBone& bone)
{
  inout_stream << bone.m_sName << bone.m_sParent << bone.m_Transform;
}

void operator>>(xiiStreamReader& inout_stream, xiiExposedBone& out_bone)
{
  inout_stream >> out_bone.m_sName >> out_bone.m_sParent >> out_bone.m_Transform;
}

bool operator==(const xiiExposedBone& lhs, const xiiExposedBone& rhs)
{
  return lhs.m_sName == rhs.m_sName && lhs.m_sParent == rhs.m_sParent && lhs.m_Transform == rhs.m_Transform;
}

xiiUInt32 xiiHashHelper<xiiExposedBone>::Hash(const xiiExposedBone& value)
{
  xiiUInt32 uiHash = xiiHashingUtils::xxHash32String(value.m_sName);
  uiHash           = xiiHashingUtils::CombineHashValues32(uiHash, xiiHashingUtils::xxHash32String(value.m_sParent));

  // Hash semantic fields rather than string storage, pointers, or transform padding. Canonicalize signed zero
  // because exact transform equality treats positive and negative zero as equal.
  const auto& t            = value.m_Transform;
  const float components[] = {t.m_vPosition.x, t.m_vPosition.y, t.m_vPosition.z,
                              t.m_qRotation.x, t.m_qRotation.y, t.m_qRotation.z, t.m_qRotation.w,
                              t.m_vScale.x, t.m_vScale.y, t.m_vScale.z};
  for (float fComponent : components)
  {
    const float fCanonical = fComponent == 0.0f ? 0.0f : fComponent;
    uiHash                 = xiiHashingUtils::CombineHashValues32(uiHash, xiiHashingUtils::xxHash32(&fCanonical, sizeof(fCanonical)));
  }
  return uiHash;
}

xiiResult xiiExposedBone::ComputeParentTransforms(xiiArrayPtr<const xiiExposedBone> bones, xiiDynamicArray<xiiMat4>& out_parentTransforms)
{
  out_parentTransforms.Clear();
  xiiHashTable<xiiStringView, xiiUInt32> indices;
  indices.Reserve(bones.GetCount());
  for (xiiUInt32 i = 0; i < bones.GetCount(); ++i)
  {
    if (bones[i].m_sName.IsEmpty() || !bones[i].m_Transform.IsValid() || indices.Insert(bones[i].m_sName.GetView(), i))
      return XII_FAILURE;
  }

  xiiDynamicArray<xiiUInt32> parents;
  parents.SetCount(bones.GetCount());
  for (xiiUInt32 i = 0; i < bones.GetCount(); ++i)
  {
    parents[i] = xiiInvalidIndex;
    if (!bones[i].m_sParent.IsEmpty() && !indices.TryGetValue(bones[i].m_sParent.GetView(), parents[i]))
      return XII_FAILURE;
  }

  xiiDynamicArray<xiiUInt8> states;
  states.SetCount(bones.GetCount(), 0);
  xiiDynamicArray<xiiUInt32> path;
  xiiDynamicArray<xiiMat4>   models;
  models.SetCount(bones.GetCount());
  xiiDynamicArray<xiiMat4> parentTransforms;
  parentTransforms.SetCount(bones.GetCount());

  for (xiiUInt32 i = 0; i < bones.GetCount(); ++i)
  {
    xiiUInt32 uiCurrent = i;
    while (uiCurrent != xiiInvalidIndex && states[uiCurrent] == 0)
    {
      states[uiCurrent] = 1;
      path.PushBack(uiCurrent);
      uiCurrent = parents[uiCurrent];
    }
    if (uiCurrent != xiiInvalidIndex && states[uiCurrent] == 1)
      return XII_FAILURE;

    while (!path.IsEmpty())
    {
      const xiiUInt32 uiBone = path.PeekBack();
      path.PopBack();
      const xiiUInt32 uiParent = parents[uiBone];
      parentTransforms[uiBone] = uiParent == xiiInvalidIndex ? xiiMat4::MakeIdentity() : models[uiParent];
      models[uiBone]           = parentTransforms[uiBone] * bones[uiBone].m_Transform.GetAsMat4();
      if (!models[uiBone].IsValid())
        return XII_FAILURE;
      states[uiBone] = 2;
    }
  }

  out_parentTransforms.Swap(parentTransforms);
  return XII_SUCCESS;
}
