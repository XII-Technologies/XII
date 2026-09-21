/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Types/Bitflags.h>
#  include <Foundation/Types/Delegate.h>

struct xiiDirectoryWatcherImpl;

/// Which action has been performed on a file.
enum class xiiDirectoryWatcherAction
{
  None,           ///< No operation occurred.
  Added,          ///< A file or directory was added
  Removed,        ///< A file or directory was removed
  Modified,       ///< A file was modified. Both Reads and Writes can 'modify' the timestamps of a file.
  RenamedOldName, ///< A file or directory was renamed. First the old name is provided.
  RenamedNewName, ///< A file or directory was renamed. The new name is provided second.
};

enum class xiiDirectoryWatcherType
{
  File,
  Directory
};

/// \brief
///   Watches file actions in a directory. Changes need to be polled.
class XII_FOUNDATION_DLL xiiDirectoryWatcher
{
public:
  /// What to watch out for.
  struct Watch
  {
    using StorageType                 = xiiUInt8;
    constexpr static xiiUInt8 Default = 0;

    /// Enum values
    enum Enum : StorageType
    {
      Writes         = XII_BIT(0), ///< Watch for writes.
      Creates        = XII_BIT(1), ///< Watch for newly created files.
      Deletes        = XII_BIT(2), ///< Watch for deleted files.
      Renames        = XII_BIT(3), ///< Watch for renames.
      Subdirectories = XII_BIT(4), ///< Watch files in subdirectories recursively.
    };

    struct Bits
    {
      StorageType Writes : 1;
      StorageType Creates : 1;
      StorageType Deletes : 1;
      StorageType Renames : 1;
      StorageType Subdirectories : 1;
    };
  };

  xiiDirectoryWatcher();
  xiiDirectoryWatcher(const xiiDirectoryWatcher&)     = delete;
  xiiDirectoryWatcher(xiiDirectoryWatcher&&) noexcept = delete;
  ~xiiDirectoryWatcher();

  xiiDirectoryWatcher& operator=(const xiiDirectoryWatcher&)     = delete;
  xiiDirectoryWatcher& operator=(xiiDirectoryWatcher&&) noexcept = delete;

  /// \brief
  /// Opens the directory at \p absolutePath for watching. \p whatToWatch controls what exactly should be watched.
  ///
  /// \note A instance of xiiDirectoryWatcher can only watch one directory at a time.
  xiiResult OpenDirectory(xiiStringView sAbsolutePath, xiiBitflags<Watch> whatToWatch);

  /// \brief
  /// Closes the currently watched directory if any.
  void CloseDirectory();

  /// \brief
  /// Returns the opened directory, will be empty if no directory was opened.
  xiiStringView GetDirectory() const { return m_sDirectoryPath; }

  using EnumerateChangesFunction = xiiDelegate<void(xiiStringView sFileName, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type), 48>;

  /// \brief
  /// Calls the callback \p func for each change since the last call. For each change the filename
  /// and the action, which was performed on the file, is passed to \p func.
  /// If waitUpToMilliseconds is greater than 0, blocks until either a change was observed or the timelimit is reached.
  ///
  /// \note There might be multiple changes on the same file reported.
  void EnumerateChanges(EnumerateChangesFunction func, xiiTime waitUpTo = xiiTime::MakeZero());

  /// \brief
  /// Same as the other EnumerateChanges function, but enumerates multiple watchers.
  static void EnumerateChanges(xiiArrayPtr<xiiDirectoryWatcher*> watchers, EnumerateChangesFunction func, xiiTime waitUpTo = xiiTime::MakeZero());

private:
  xiiString                m_sDirectoryPath;
  xiiDirectoryWatcherImpl* m_pImpl = nullptr;
};

XII_DECLARE_FLAGS_OPERATORS(xiiDirectoryWatcher::Watch);

#endif
