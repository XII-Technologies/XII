#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRmlUiAssetProperties, 1, xiiRTTIDefaultAllocator<xiiRmlUiAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RmlFile", m_sRmlFile)->AddAttributes(new xiiFileBrowserAttribute("Select Rml file", "*.rml")),
    XII_ENUM_MEMBER_PROPERTY("ScaleMode", xiiRmlUiScaleMode, m_ScaleMode),
    XII_MEMBER_PROPERTY("ReferenceResolution", m_ReferenceResolution)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2U32(1920, 1080))),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRmlUiAssetProperties::xiiRmlUiAssetProperties()  = default;
xiiRmlUiAssetProperties::~xiiRmlUiAssetProperties() = default;
