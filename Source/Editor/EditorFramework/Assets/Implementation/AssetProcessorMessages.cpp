#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetProcessorMessages.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcessAssetMsg, 1, xiiRTTIDefaultAllocator<xiiProcessAssetMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("AssetGuid", m_AssetGuid),
    XII_MEMBER_PROPERTY("AssetHash", m_AssetHash),
    XII_MEMBER_PROPERTY("ThumbHash", m_ThumbHash),
    XII_MEMBER_PROPERTY("PackageHash", m_PackageHash),
    XII_MEMBER_PROPERTY("AssetPath", m_sAssetPath),
    XII_MEMBER_PROPERTY("Platform", m_sPlatform),
    XII_ARRAY_MEMBER_PROPERTY("DepRefHull", m_DepRefHull),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcessAssetResponseMsg, 1, xiiRTTIDefaultAllocator<xiiProcessAssetResponseMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Status", m_Status),
    XII_ARRAY_MEMBER_PROPERTY("LogEntries", m_LogEntries),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
