/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

struct XII_SHAREDPLUGINASSETS_DLL xiiMaterialResourceSlot
{
  xiiString m_sLabel;
  xiiString m_sResource;
  bool      m_bHighlight = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_SHAREDPLUGINASSETS_DLL, xiiMaterialResourceSlot);

namespace xiiMeshMaterialSlots
{
  /// Reconcile source material slots, retaining assignments by label and duplicate occurrence.
  /// Unnamed legacy slots retain their assignment at the same index. A section count is not
  /// a material count: several native sections may reference one shared material slot.
  XII_SHAREDPLUGINASSETS_DLL void Reconcile(xiiArrayPtr<const xiiStringView> labels, xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_slots);

  XII_SHAREDPLUGINASSETS_DLL bool Equal(xiiArrayPtr<const xiiMaterialResourceSlot> a, xiiArrayPtr<const xiiMaterialResourceSlot> b);
} // namespace xiiMeshMaterialSlots
