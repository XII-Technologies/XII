/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <Foundation/Network/Reflection/NetworkAttributes.h>

#include <EditorFramework/Assets/AssetProcessorMessages.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcessAssetMsg, 1, xiiRTTIDefaultAllocator<xiiProcessAssetMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("AssetGuid", m_AssetGuid)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("AssetHash", m_AssetHash)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("ThumbHash", m_ThumbHash)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("PackageHash", m_PackageHash)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("AssetPath", m_sAssetPath)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_MEMBER_PROPERTY("Platform", m_sPlatform)->AddAttributes(new xiiNetworkAttribute(6)),
    XII_BITFLAGS_MEMBER_PROPERTY("TransformFlags", xiiTransformFlags, m_TransformFlags)->AddAttributes(new xiiNetworkAttribute(7)),
    XII_MEMBER_PROPERTY("Import", m_bImport)->AddAttributes(new xiiNetworkAttribute(8)),
    XII_ARRAY_MEMBER_PROPERTY("DepRefHull", m_DepRefHull)->AddAttributes(new xiiNetworkAttribute(9)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494100000001ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcessAssetResponseMsg, 1, xiiRTTIDefaultAllocator<xiiProcessAssetResponseMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Status", m_Status)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_ARRAY_MEMBER_PROPERTY("LogEntries", m_LogEntries)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("ImportBefore", m_sImportBefore)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("ImportAfter", m_sImportAfter)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_ARRAY_MEMBER_PROPERTY("ImportFiles", m_ImportFiles)->AddAttributes(new xiiNetworkAttribute(5)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494100000002ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFreeAllResourcesMsg, 1, xiiRTTIDefaultAllocator<xiiFreeAllResourcesMsg>)
{
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494100000003ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
