#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetObjects.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImageDataAssetProperties, 1, xiiRTTIDefaultAllocator<xiiImageDataAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_sInputFile)->AddAttributes(new xiiFileBrowserAttribute("Select Image", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr"))
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
