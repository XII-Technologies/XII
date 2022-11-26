#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/IO/FileEnums.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Timestamp.h>

struct xiiOSFileData;

#if XII_ENABLED(XII_USE_POSIX_FILE_API)
#  include <Foundation/IO/Implementation/Posix/OSFileDeclarations_posix.h>
#elif XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/IO/Implementation/Win/OSFileDeclarations_win.h>
#endif

/// \brief Defines in which mode to open a file.
struct xiiFileOpenMode
{
  enum Enum
  {
    None,   ///< None, only used internally.
    Read,   ///< Open file for reading.
    Write,  ///< Open file for writing (already existing data is discarded).
    Append, ///< Open file for appending (writing, but always only at the end, already existing data is preserved).
  };
};

/// \brief Holds the stats for a file.
struct XII_FOUNDATION_DLL xiiFileStats
{
  xiiFileStats();

  /// \brief Stores the concatenated m_sParentPath and m_sName in \a path.
  void GetFullPath(xiiStringBuilder& path) const;

  /// \brief Path to the parent folder.
  /// Append m_sName to m_sParentPath to obtain the full path.
  xiiStringBuilder m_sParentPath;

  /// \brief The name of the file or folder that the stats are for. Does not include the parent path to it.
  /// Append m_sName to m_sParentPath to obtain the full path.
  xiiString m_sName;

  /// \brief The last modification time as an UTC timestamp since Unix epoch.
  xiiTimestamp m_LastModificationTime;

  /// \brief The size of the file in bytes.
  xiiUInt64 m_uiFileSize = 0;

  /// \brief Whether the file object is a file or folder.
  bool m_bIsDirectory = false;
};

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) || defined(XII_DOCS)

struct xiiFileIterationData;

struct xiiFileSystemIteratorFlags
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Recursive     = XII_BIT(0),
    ReportFiles   = XII_BIT(1),
    ReportFolders = XII_BIT(2),

    ReportFilesRecursive           = Recursive | ReportFiles,
    ReportFoldersRecursive         = Recursive | ReportFolders,
    ReportFilesAndFoldersRecursive = Recursive | ReportFiles | ReportFolders,

    Default = ReportFilesAndFoldersRecursive,
  };

  struct Bits
  {
    StorageType Recursive : 1;
    StorageType ReportFiles : 1;
    StorageType ReportFolders : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiFileSystemIteratorFlags);

/// \brief An xiiFileSystemIterator allows to iterate over all files in a certain directory.
///
/// The search can be recursive, and it can contain wildcards (* and ?) to limit the search to specific file types.
class XII_FOUNDATION_DLL xiiFileSystemIterator
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiFileSystemIterator);

public:
  xiiFileSystemIterator();
  ~xiiFileSystemIterator();

  /// \brief Starts a search at the given folder. Use * and ? as wildcards.
  ///
  /// To iterate all files from one folder, use '/Some/Folder'
  /// To iterate over all files of a certain type (in one folder) use '/Some/Folder/*.ext'
  /// Only the final path segment can use placeholders, folders in between must be fully named.
  /// If bRecursive is false, the iterator will only iterate over the files in the start folder, and will not recurse into subdirectories.
  /// If bReportFolders is false, only files will be reported, folders will be skipped (though they will be recursed into, if bRecursive is true).
  ///
  /// If XII_SUCCESS is returned, the iterator points to a valid file, and the functions GetCurrentPath() and GetStats() will return
  /// the information about that file. To advance to the next file, use Next() or SkipFolder().
  /// When no iteration is possible (the directory does not exist or the wild-cards are used incorrectly), XII_FAILURE is returned.
  void StartSearch(const char* szSearchTerm, xiiBitflags<xiiFileSystemIteratorFlags> flags = xiiFileSystemIteratorFlags::Default); // [tested]

  /// \brief The same as StartSearch() but executes the same search on multiple folders.
  ///
  /// The search term is appended to each start folder and they are searched one after the other.
  void StartMultiFolderSearch(xiiArrayPtr<xiiString> startFolders, const char* szSearchTerm, xiiBitflags<xiiFileSystemIteratorFlags> flags = xiiFileSystemIteratorFlags::Default);

  /// \brief Returns the search string with which StartSearch() was called.
  ///
  /// If StartMultiFolderSearch() is used, every time a new top-level folder is entered, StartSearch() is executed. In this case GetCurrentSearchTerm() can be used to know in which top-level folder the search is currently running.
  const xiiStringView GetCurrentSearchTerm() const { return m_sSearchTerm; }

  /// \brief Returns the current path in which files are searched. Changes when 'Next' moves in or out of a sub-folder.
  ///
  /// You can use this to get the full path of the current file, by appending this value and the filename from 'GetStats'
  const xiiStringBuilder& GetCurrentPath() const { return m_sCurPath; } // [tested]

  /// \brief Returns the file stats of the current object that the iterator points to.
  const xiiFileStats& GetStats() const { return m_CurFile; } // [tested]

  /// \brief Advances the iterator to the next file object. Might recurse into sub-folders.
  void Next(); // [tested]

  /// \brief The same as 'Next' only that the current folder will not be recursed into.
  void SkipFolder(); // [tested]

  /// \brief Returns true if the iterator currently points to a valid file entry.
  bool IsValid() const;

private:
  xiiInt32 InternalNext();

  /// \brief The current path of the folder, in which the iterator currently is.
  xiiStringBuilder m_sCurPath;

  xiiBitflags<xiiFileSystemIteratorFlags> m_Flags;

  /// \brief The stats about the file that the iterator currently points to.
  xiiFileStats m_CurFile;

  /// \brief Platform specific data, required by the implementation.
  xiiFileIterationData m_Data;

  xiiString                    m_sSearchTerm;
  xiiString                    m_sMultiSearchTerm;
  xiiUInt32                    m_uiCurrentStartFolder = 0;
  xiiHybridArray<xiiString, 8> m_StartFolders;
};

#endif

/// \brief This is an abstraction for the most important file operations.
///
/// Instances of xiiOSFile can be used for reading and writing files.
/// All paths must be absolute paths, relative paths and current working directories are not supported,
/// since that cannot be guaranteed to work equally on all platforms under all circumstances.
/// A few static functions allow to query the most important data about files, to delete files and create directories.
class XII_FOUNDATION_DLL xiiOSFile
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiOSFile);

public:
  xiiOSFile();
  ~xiiOSFile();

  /// \brief Opens a file for reading or writing. Returns XII_SUCCESS if the file could be opened successfully.
  xiiResult Open(const char* szFile, xiiFileOpenMode::Enum OpenMode, xiiFileShareMode::Enum FileShareMode = xiiFileShareMode::Default); // [tested]

  /// \brief Returns true if a file is currently open.
  bool IsOpen() const; // [tested]

  /// \brief Closes the file, if it is currently opened.
  void Close(); // [tested]

  /// \brief Writes the given number of bytes from the buffer into the file. Returns true if all data was successfully written.
  xiiResult Write(const void* pBuffer, xiiUInt64 uiBytes); // [tested]

  /// \brief Reads up to the given number of bytes from the file. Returns the actual number of bytes that was read.
  xiiUInt64 Read(void* pBuffer, xiiUInt64 uiBytes); // [tested]

  /// \brief Reads the entire file content into the given array
  xiiUInt64 ReadAll(xiiDynamicArray<xiiUInt8>& out_FileContent); // [tested]

  /// \brief Returns the name of the file that is currently opened. Returns an empty string, if no file is open.
  const char* GetOpenFileName() const { return m_sFileName.GetData(); } // [tested]

  /// \brief Returns the position in the file at which read/write operations will occur.
  xiiUInt64 GetFilePosition() const; // [tested]

  /// \brief Sets the position where in the file to read/write next.
  void SetFilePosition(xiiInt64 iDistance, xiiFileSeekMode::Enum Pos) const; // [tested]

  /// \brief Returns the current total size of the file.
  xiiUInt64 GetFileSize() const; // [tested]

  /// \brief This will return the platform specific file data (handles etc.), if you really want to be able to wreak havoc.
  const xiiOSFileData& GetFileData() const { return m_FileData; }

  /// \brief Returns the processes current working directory (CWD).
  ///
  /// The value typically depends on the directory from which the application was launched.
  /// Since this is a process wide global variable, other code can modify it at any time.
  ///
  /// \note xii does not use the CWD for any file resolution. This function is provided to enable
  /// tools to work with relative paths from the command-line, but every application has to implement
  /// such behavior individually.
  static const xiiString GetCurrentWorkingDirectory(); // [tested]

  /// \brief If szPath is a relative path, this function prepends GetCurrentWorkingDirectory().
  ///
  /// In either case, MakeCleanPath() is used before the string is returned.
  static const xiiString MakePathAbsoluteWithCWD(const char* szPath); // [tested]

  /// \brief Checks whether the given file exists.
  static bool ExistsFile(const char* szFile); // [tested]

  /// \brief Checks whether the given file exists.
  static bool ExistsDirectory(const char* szDirectory); // [tested]

  /// \brief Deletes the given file. Returns XII_SUCCESS, if the file was deleted or did not exist in the first place. Returns XII_FAILURE
  static xiiResult DeleteFile(const char* szFile); // [tested]

  /// \brief Creates the given directory structure (meaning all directories in the path, that do not exist). Returns false, if any directory could not
  /// be created.
  static xiiResult CreateDirectoryStructure(const char* szDirectory); // [tested]

  /// \brief Renames / Moves an existing directory. The file / directory at szFrom must exist. The parent directory of szTo must exist.
  /// Returns XII_FAILURE if the move failed.
  static xiiResult MoveFileOrDirectory(const char* szFrom, const char* szTo);

  /// \brief Copies the source file into the destination file.
  static xiiResult CopyFile(const char* szSource, const char* szDestination); // [tested]

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS) || defined(XII_DOCS)
  /// \brief Gets the stats about the given file or folder. Returns false, if the stats could not be determined.
  static xiiResult GetFileStats(const char* szFileOrFolder, xiiFileStats& out_Stats); // [tested]

#  if (XII_ENABLED(XII_SUPPORTS_CASE_INSENSITIVE_PATHS) && XII_ENABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)) || defined(XII_DOCS)
  /// \brief Useful on systems that are not strict about the casing of file names. Determines the correct name of a file.
  static xiiResult GetFileCasing(const char* szFileOrFolder, xiiStringBuilder& out_sCorrectSpelling); // [tested]
#  endif

#endif

#if (XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) && XII_ENABLED(XII_SUPPORTS_FILE_STATS)) || defined(XII_DOCS)

  /// \brief Returns the xiiFileStats for all files and folders in the given folder
  static void GatherAllItemsInFolder(xiiDynamicArray<xiiFileStats>& out_ItemList, const char* szFolder, xiiBitflags<xiiFileSystemIteratorFlags> flags = xiiFileSystemIteratorFlags::Default);

  /// \brief Copies \a szSourceFolder to \a szDestinationFolder. Overwrites existing files.
  ///
  /// If \a out_FilesCopied is provided, the destination path of every successfully copied file is appended to it.
  static xiiResult CopyFolder(const char* szSourceFolder, const char* szDestinationFolder, xiiDynamicArray<xiiString>* out_FilesCopied = nullptr);

  /// \brief Deletes all files recursively in \a szFolder.
  ///
  /// \note The current implementation does not remove the (empty) folders themselves.
  static xiiResult DeleteFolder(const char* szFolder);

#endif

  /// \brief Returns the path in which the applications binary file is located.
  static const char* GetApplicationDirectory();

  /// \brief Returns the folder into which user data may be safely written.
  /// Append a sub-folder for your application.
  ///
  /// On Windows this is the '%appdata%' directory.
  /// On Posix systems this is the '~' (home) directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static xiiString GetUserDataFolder(const char* szSubFolder = nullptr);

  /// \brief Returns the folder into which temp data may be written.
  ///
  /// On Windows this is the '%localappdata%/Temp' directory.
  /// On Posix systems this is the '~/.cache' directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static xiiString GetTempDataFolder(const char* szSubFolder = nullptr);

public:
  /// \brief Describes the types of events that xiiOSFile sends.
  struct EventType
  {
    enum Enum
    {
      None,
      FileOpen,        ///< A file has been (attempted) to open.
      FileClose,       ///< An open file has been closed.
      FileExists,      ///< A check whether a file exists has been done.
      DirectoryExists, ///< A check whether a directory exists has been done.
      FileDelete,      ///< A file was attempted to be deleted.
      FileRead,        ///< From an open file data was read.
      FileWrite,       ///< Data was written to an open file.
      MakeDir,         ///< A path has been created (recursive directory creation).
      FileCopy,        ///< A file has been copied to another location.
      FileStat,        ///< The stats of a file are queried
      FileCasing,      ///< The exact spelling of a file/path is requested
    };
  };

  /// \brief The data that is sent through the event interface.
  struct EventData
  {
    /// \brief The type of information that is sent.
    EventType::Enum m_EventType;

    /// \brief A unique ID for each file access. Reads and writes to the same open file use the same ID. If the same file is opened multiple times,
    /// different IDs are used.
    xiiInt32 m_iFileID;

    /// \brief The name of the file that was operated upon.
    const char* m_szFile;

    /// \brief If a second file was operated upon (FileCopy), that is the second file name.
    const char* m_szFile2;

    /// \brief Mode that a file has been opened in.
    xiiFileOpenMode::Enum m_FileMode;

    /// \brief Whether the operation succeeded (reading, writing, etc.)
    bool m_bSuccess;

    /// \brief How long the operation took.
    xiiTime m_Duration;

    /// \brief How many bytes were transfered (reading, writing)
    xiiUInt64 m_uiBytesAccessed;

    EventData()
    {
      m_EventType       = EventType::None;
      m_iFileID         = 0;
      m_szFile          = nullptr;
      m_szFile2         = nullptr;
      m_FileMode        = xiiFileOpenMode::None;
      m_bSuccess        = true;
      m_uiBytesAccessed = 0;
    }
  };

  using Event = xiiEvent<const EventData&, xiiMutex>;

  /// \brief Allows to register a function as an event receiver. All receivers will be notified in the order that they registered.
  static void AddEventHandler(Event::Handler handler) { s_FileEvents.AddEventHandler(handler); }

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveEventHandler(Event::Handler handler) { s_FileEvents.RemoveEventHandler(handler); }

private:
  /// \brief Manages all the Event Handlers for the OSFile events.
  static Event s_FileEvents;

  // *** Internal Functions that do the platform specific work ***

  xiiResult InternalOpen(const char* szFile, xiiFileOpenMode::Enum OpenMode, xiiFileShareMode::Enum FileShareMode);
  void      InternalClose();
  xiiResult InternalWrite(const void* pBuffer, xiiUInt64 uiBytes);
  xiiUInt64 InternalRead(void* pBuffer, xiiUInt64 uiBytes);
  xiiUInt64 InternalGetFilePosition() const;
  void      InternalSetFilePosition(xiiInt64 iDistance, xiiFileSeekMode::Enum Pos) const;

  static bool      InternalExistsFile(const char* szFile);
  static bool      InternalExistsDirectory(const char* szDirectory);
  static xiiResult InternalDeleteFile(const char* szFile);
  static xiiResult InternalDeleteDirectory(const char* szDirectory);
  static xiiResult InternalCreateDirectory(const char* szFile);
  static xiiResult InternalMoveFileOrDirectory(const char* szDirectoryFrom, const char* szDirectoryTo);

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  static xiiResult InternalGetFileStats(const char* szFileOrFolder, xiiFileStats& out_Stats);
#endif

  // *************************************************************

  /// \brief Stores the mode with which the file was opened.
  xiiFileOpenMode::Enum m_FileMode;

  /// [internal] On win32 when a file is already open, and this is true, xiiOSFile will wait until the file becomes available
  bool m_bRetryOnSharingViolation = true;

  /// \brief Stores the (cleaned up) filename that was used to open the file.
  xiiStringBuilder m_sFileName;

  /// \brief Stores the value of s_FileCounter when the xiiOSFile is created.
  xiiInt32 m_iFileID;

  /// \brief Platform specific data about the open file.
  xiiOSFileData m_FileData;

  /// \brief The application binaries' path.
  static xiiString64 s_sApplicationPath;

  /// \brief The path where user data is stored on this OS
  static xiiString64 s_sUserDataPath;

  /// \brief The path where temp data is stored on this OS
  static xiiString64 s_sTempDataPath;

  /// \brief Counts how many different files are touched.225
  static xiiAtomicInteger32 s_iFileCounter;
};
