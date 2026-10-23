/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/LogEntry.h>

class XII_EDITORFRAMEWORK_DLL xiiProcessAssetMsg : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcessAssetMsg, xiiProcessMessage);

public:
  xiiUuid                        m_AssetGuid;
  xiiUInt64                      m_AssetHash   = 0;
  xiiUInt64                      m_ThumbHash   = 0;
  xiiUInt64                      m_PackageHash = 0;
  xiiString                      m_sAssetPath;
  xiiString                      m_sPlatform;
  xiiBitflags<xiiTransformFlags> m_TransformFlags;
  bool                           m_bImport = false; ///< Compute source-document changes without saving the source in the child process.
  xiiDynamicArray<xiiString>     m_DepRefHull;
};

class XII_EDITORFRAMEWORK_DLL xiiProcessAssetResponseMsg : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcessAssetResponseMsg, xiiProcessMessage);

public:
  xiiTransformStatus                   m_Status;
  mutable xiiDynamicArray<xiiLogEntry> m_LogEntries;
  xiiString                            m_sImportBefore;
  xiiString                            m_sImportAfter;
};

class XII_EDITORFRAMEWORK_DLL xiiFreeAllResourcesMsg : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFreeAllResourcesMsg, xiiProcessMessage);
};
