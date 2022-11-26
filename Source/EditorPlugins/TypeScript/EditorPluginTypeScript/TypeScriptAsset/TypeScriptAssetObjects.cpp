#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptParameter, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptParameterNumber, 1, xiiRTTIDefaultAllocator<xiiTypeScriptParameterNumber>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptParameterBool, 1, xiiRTTIDefaultAllocator<xiiTypeScriptParameterBool>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptParameterString, 1, xiiRTTIDefaultAllocator<xiiTypeScriptParameterString>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptParameterVec3, 1, xiiRTTIDefaultAllocator<xiiTypeScriptParameterVec3>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptParameterColor, 1, xiiRTTIDefaultAllocator<xiiTypeScriptParameterColor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptAssetProperties, 1, xiiRTTIDefaultAllocator<xiiTypeScriptAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ScriptFile", m_sScriptFile)->AddAttributes(new xiiFileBrowserAttribute("Select Script", "*.ts")),
    XII_ARRAY_MEMBER_PROPERTY("NumberParameters", m_NumberParameters),
    XII_ARRAY_MEMBER_PROPERTY("BoolParameters", m_BoolParameters),
    XII_ARRAY_MEMBER_PROPERTY("StringParameters", m_StringParameters),
    XII_ARRAY_MEMBER_PROPERTY("Vec3Parameters", m_Vec3Parameters),
    XII_ARRAY_MEMBER_PROPERTY("ColorParameters", m_ColorParameters),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTypeScriptAssetProperties::xiiTypeScriptAssetProperties()  = default;
xiiTypeScriptAssetProperties::~xiiTypeScriptAssetProperties() = default;
