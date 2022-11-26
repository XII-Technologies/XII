#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLUTAssetProperties, 1, xiiRTTIDefaultAllocator<xiiLUTAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Input", GetInputFile, SetInputFile)->AddAttributes(new xiiFileBrowserAttribute("Select CUBE file", "*.cube")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiLUTAssetProperties::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiLUTAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool isRenderTarget = e.m_pObject->GetTypeAccessor().GetValue("IsRenderTarget").ConvertTo<bool>();

    props["Input"].m_Visibility    = xiiPropertyUiState::Default;
    props["Input"].m_sNewLabelText = "CUBE file";
  }
}

xiiString xiiLUTAssetProperties::GetAbsoluteInputFilePath() const
{
  xiiStringBuilder sPath = m_sInput;
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}
