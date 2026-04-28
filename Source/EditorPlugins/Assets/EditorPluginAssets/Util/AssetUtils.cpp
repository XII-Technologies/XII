/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Util/AssetUtils.h>

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
