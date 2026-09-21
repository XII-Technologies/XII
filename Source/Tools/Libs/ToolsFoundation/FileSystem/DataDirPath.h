/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/IO/Stream.h>

/// A reference to a file or folder inside a data directory.
///
/// Allows quick access to various sub-parts of the path as well as the data dir index.
/// To construct a xiiDataDirPath, the list of absolute data directory root directories must be supplied in order to determine whether the path is inside a data directory and in which. After calling the constructor, IsValid() should be called to determine if the file is inside a data directory.
/// The various sub-parts look like this with "Testing Chambers" being the data directory in this example:
///
///  GetAbsolutePath() == "C:/xiiEngine/Data/Samples/Testing Chambers/Objects/Barrel.xiiPrefab"
///  GetDataDir() == "C:/xiiEngine/Data/Samples/Testing Chambers"
///  GetDataDirParentRelativePath() == "Testing Chambers/Objects/Barrel.xiiPrefab"
///  GetDataDirRelativePath() == "Objects/Barrel.xiiPrefab"
class XII_TOOLSFOUNDATION_DLL xiiDataDirPath
{
public:
  /// \name Constructor
  ///@{

  /// Default ctor, creates an invalid data directory path.
  xiiDataDirPath();
  /// Tries to create a new data directory path from an absolute path. Check IsValid afterwards to confirm this path is inside a data directory.
  /// \param sAbsPath Absolute path to the file or folder. Must be normalized. Must not end with "/".
  /// \param dataDirRoots A list of normalized absolute paths to the roots of the data directories. These must not end in a "/" character.
  /// \param uiLastKnownDataDirIndex A hint to accelerate the search if the data directory index is known.
  xiiDataDirPath(xiiStringView sAbsPath, xiiArrayPtr<xiiString> dataDirRoots, xiiUInt32 uiLastKnownDataDirIndex = 0);
  /// Overload for xiiStringBuilder to fix ambiguity between implicit conversions.
  xiiDataDirPath(const xiiStringBuilder& sAbsPath, xiiArrayPtr<xiiString> dataDirRoots, xiiUInt32 uiLastKnownDataDirIndex = 0);
  /// Move constructor overload for the absolute path.
  xiiDataDirPath(xiiString&& sAbsPath, xiiArrayPtr<xiiString> dataDirRoots, xiiUInt32 uiLastKnownDataDirIndex = 0);


  ///@}
  /// \name Misc
  ///@{

  /// Returns the same path this instance was created with. Calling this function is always valid.
  const xiiString& GetAbsolutePath() const;
  /// Returns whether this path is inside a data directory. If not, none of the Get* functions except for GetAbsolutePath are allowed to be called.
  bool IsValid() const;
  /// Same as the default constructor. Creates an empty, invalid path.
  void Clear();

  ///@}
  /// \name Operators
  ///@{

       operator xiiStringView() const;
  bool operator==(xiiStringView rhs) const;
  bool operator!=(xiiStringView rhs) const;

  ///@}
  /// \name Data directory access. Only allowed to be called if IsValid() is true.
  ///@{

  /// Returns a relative path including the data directory the path belongs to, e.g. "Testing Chambers/Objects/Barrel.xiiPrefab".
  xiiStringView GetDataDirParentRelativePath() const;
  /// Returns a path relative to the data directory the path belongs to, e.g. "Objects/Barrel.xiiPrefab".
  xiiStringView GetDataDirRelativePath() const;
  /// Returns absolute path to the data directory this path belongs to, e.g.  "C:/xiiEngine/Data/Samples/Testing Chambers".
  xiiStringView GetDataDir() const;
  /// Returns the index of the data directory the path belongs to.
  xiiUInt8 GetDataDirIndex() const;

  ///@}
  /// \name Data directory update
  ///@{

  /// If a xiiDataDirPath is de-serialized, it might not be correct anymore and its data directory reference must be updated. It could potentially no longer be part of any data directory at all and become invalid so after calling this function, IsValid will match the return value of this function. On failure, the invalid data directory paths should then be destroyed.
  /// \param dataDirRoots A list of normalized absolute paths to the roots of the data directories. These must not end in a "/" character.
  /// \param uiLastKnownDataDirIndex A hint to accelerate the search if nothing has changed.
  /// \return Returns whether the data directory path is valid, i.e. it is still under one of the dataDirRoots.
  bool UpdateDataDirInfos(xiiArrayPtr<xiiString> dataDirRoots, xiiUInt32 uiLastKnownDataDirIndex = 0) const;

  ///@}
  /// \name Serialization
  ///@{

  xiiStreamWriter& Write(xiiStreamWriter& inout_stream) const;
  xiiStreamReader& Read(xiiStreamReader& inout_stream);

  ///@}

private:
  xiiString         m_sAbsolutePath;
  mutable xiiUInt16 m_uiDataDirParent = 0;
  mutable xiiUInt8  m_uiDataDirLength = 0;
  mutable xiiUInt8  m_uiDataDirIndex  = 0;
};

xiiStreamWriter& operator<<(xiiStreamWriter& inout_stream, const xiiDataDirPath& value);
xiiStreamReader& operator>>(xiiStreamReader& inout_stream, xiiDataDirPath& out_value);

/// Comparator that first sort case-insensitive and then case-sensitive if necessary for a unique ordering.
///
/// Use this comparator when sorting e.g. files on disk like they would appear in a windows explorer.
/// This comparator is using xiiStringView instead of xiiDataDirPath as all string and xiiDataDirPath can be implicitly converted to xiiStringView.
struct XII_TOOLSFOUNDATION_DLL xiiCompareDataDirPath
{
  static inline bool Less(xiiStringView lhs, xiiStringView rhs);
  static inline bool Equal(xiiStringView lhs, xiiStringView rhs);
};

#include <ToolsFoundation/FileSystem/Implementation/DataDirPath_inl.h>
