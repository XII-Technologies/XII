/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <FileservePlugin/FileservePluginDLL.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Network/NetworkChannel.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

class xiiOSFile;

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
    bool      m_bMounted  = false;
    bool      m_bWritable = false;
  };

  struct FileStatus
  {
    xiiInt64  m_iTimestamp = -1;
    xiiUInt64 m_uiHash     = 0;
    xiiUInt64 m_uiFileSize = 0;
  };

  static bool           IsValidRelativePath(xiiStringView sPath);
  static xiiResult      ResolveFilePath(xiiStringView sRoot, xiiStringView sRelativePath, xiiStringBuilder& out_sFile, bool bDirectory = false);
  static xiiResult      HashFile(xiiOSFile& ref_file, xiiUInt64 uiSize, xiiUInt64& out_uiHash);
  xiiFileserveFileState GetFileStatus(xiiUInt16& inout_uiDataDirID, xiiStringView sRequestedFile, FileStatus& inout_status, xiiOSFile& out_file, bool bForceThisDataDir) const;

  xiiNetworkChannel          m_Channel;
  xiiUInt32                  m_uiApplicationID = 0;
  xiiHybridArray<DataDir, 8> m_MountedDataDirs;
};
