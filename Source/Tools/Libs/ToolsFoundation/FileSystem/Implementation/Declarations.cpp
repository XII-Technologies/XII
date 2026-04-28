/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <ToolsFoundation/FileSystem/Declarations.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiFileStatus, xiiNoBase, 3, xiiRTTIDefaultAllocator<xiiFileStatus>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LastModified", m_LastModified),
    XII_MEMBER_PROPERTY("Hash", m_uiHash),
    XII_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on
