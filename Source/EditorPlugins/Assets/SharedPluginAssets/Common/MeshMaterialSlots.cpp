/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <SharedPluginAssets/Common/MeshMaterialSlots.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialResourceSlot, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialResourceSlot>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_MEMBER_PROPERTY("Highlight", m_bHighlight)->AddAttributes(new xiiTemporaryAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiMeshMaterialSlots::Reconcile(xiiArrayPtr<const xiiStringView> labels, xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_slots)
{
  constexpr xiiUInt32                invalid = xiiMath::MaxValue<xiiUInt32>();
  xiiHashTable<xiiString, xiiUInt32> first;
  xiiDynamicArray<xiiUInt32>         next;
  next.SetCount(inout_slots.GetCount(), invalid);
  // Reverse insertion creates a FIFO chain for each label without per-label containers.
  for (xiiUInt32 i = inout_slots.GetCount(); i > 0; --i)
  {
    const auto index = i - 1;
    if (inout_slots[index].m_sLabel.IsEmpty())
      continue;
    first.TryGetValue(inout_slots[index].m_sLabel, next[index]);
    first.Insert(inout_slots[index].m_sLabel, index);
  }
  xiiHybridArray<xiiMaterialResourceSlot, 8> result;
  result.SetCount(labels.GetCount());
  for (xiiUInt32 i = 0; i < labels.GetCount(); ++i)
  {
    xiiUInt32 old = invalid;
    if (!labels[i].IsEmpty())
      first.TryGetValue(labels[i], old);
    if (old != invalid)
    {
      result[i] = inout_slots[old];
      first.Insert(xiiString(labels[i]), next[old]);
    }
    else if (i < inout_slots.GetCount() && inout_slots[i].m_sLabel.IsEmpty())
      result[i] = inout_slots[i];
    result[i].m_sLabel = labels[i];
  }
  inout_slots = std::move(result);
}

bool xiiMeshMaterialSlots::Equal(xiiArrayPtr<const xiiMaterialResourceSlot> a, xiiArrayPtr<const xiiMaterialResourceSlot> b)
{
  if (a.GetCount() != b.GetCount())
    return false;
  for (xiiUInt32 i = 0; i < a.GetCount(); ++i)
    if (a[i].m_sLabel != b[i].m_sLabel || a[i].m_sResource != b[i].m_sResource || a[i].m_bHighlight != b[i].m_bHighlight)
      return false;
  return true;
}
