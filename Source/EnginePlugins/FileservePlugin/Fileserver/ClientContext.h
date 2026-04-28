/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <FileservePlugin/FileservePluginDLL.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>

enum class xiiFileserveFileState
{
  None              = 0,
  NonExistant       = 1,
  NonExistantEither = 2,
  SameTimestamp     = 3,
  SameHash          = 4,
  Different         = 5,
};

class XII_FILESERVEPLUGIN_DLL xiiFileserveClientContext
{
public:
  struct DataDir
  {
    xiiString m_sRootName;
    xiiString m_sPathOnClient;
    xiiString m_sPathOnServer;
    xiiString m_sMountPoint;
    bool      m_bMounted = false;
  };

  struct FileStatus
  {
    xiiInt64  m_iTimestamp = -1;
    xiiUInt64 m_uiHash     = 0;
    xiiUInt64 m_uiFileSize = 0;
  };

  xiiFileserveFileState GetFileStatus(xiiUInt16& inout_uiDataDirID, const char* szRequestedFile, FileStatus& inout_status, xiiDynamicArray<xiiUInt8>& out_fileContent, bool bForceThisDataDir) const;

  bool                       m_bLostConnection = false;
  xiiUInt32                  m_uiApplicationID = 0;
  xiiHybridArray<DataDir, 8> m_MountedDataDirs;
};
