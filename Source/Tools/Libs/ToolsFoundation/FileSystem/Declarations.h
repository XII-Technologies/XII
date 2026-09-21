/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Uuid.h>

#if 0 // Define to enable extensive file system profile scopes
#  define FILESYSTEM_PROFILE(szName) XII_PROFILE_SCOPE(szName)

#else
#  define FILESYSTEM_PROFILE(Name)

#endif

/// Information about a single file on disk. The file might be a document or any other file found in the data directories.
struct XII_TOOLSFOUNDATION_DLL xiiFileStatus
{
  enum class Status : xiiUInt8
  {
    Unknown,    ///< Since the file has been tagged as 'Unknown' it has not been encountered again on disk (yet). Use internally to find stale entries in the model.
    FileLocked, ///< The file is locked, i.e. reading is currently not possible. Try again at a later date.
    Valid       ///< The file exists on disk.
  };

  xiiFileStatus() = default;

  xiiTimestamp m_LastModified;
  xiiUInt64    m_uiHash = 0;
  xiiUuid      m_DocumentID; ///< If the file is linked to a document, the GUID is valid, otherwise not.
  Status       m_Status = Status::Unknown;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_TOOLSFOUNDATION_DLL, xiiFileStatus);

XII_ALWAYS_INLINE xiiStreamWriter& operator<<(xiiStreamWriter& inout_stream, const xiiFileStatus& value)
{
  inout_stream.WriteBytes(&value, sizeof(xiiFileStatus)).IgnoreResult();
  return inout_stream;
}

XII_ALWAYS_INLINE xiiStreamReader& operator>>(xiiStreamReader& inout_stream, xiiFileStatus& ref_value)
{
  inout_stream.ReadBytes(&ref_value, sizeof(xiiFileStatus));
  return inout_stream;
}
