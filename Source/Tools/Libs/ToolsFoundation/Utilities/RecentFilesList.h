/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>

/// \brief Maintains a list of recently used files and the container window ID they previously resided in.
class XII_TOOLSFOUNDATION_DLL xiiRecentFilesList
{
public:
  xiiRecentFilesList(xiiUInt32 uiMaxElements) :
    m_uiMaxElements(uiMaxElements)
  {
  }

  /// \brief Struct that defines the file and container window of the recent file list.
  struct RecentFile
  {
    RecentFile() :
      m_iContainerWindow(0)
    {
    }
    RecentFile(xiiStringView sFile, xiiInt32 iContainerWindow) :
      m_File(sFile), m_iContainerWindow(iContainerWindow)
    {
    }

    xiiString m_File;
    xiiInt32  m_iContainerWindow;
  };

  /// \brief Moves the inserted file to the front with the given container ID.
  void Insert(xiiStringView sFile, xiiInt32 iContainerWindow);

  /// \brief Returns all files in the list.
  const xiiDeque<RecentFile>& GetFileList() const { return m_Files; }

  /// \brief Clears the list.
  void Clear() { m_Files.Clear(); }

  /// \brief Saves the recent files list to the given file. Uses a simple text file format (one line per item).
  void Save(xiiStringView sFile);

  /// \brief Loads the recent files list from the given file. Uses a simple text file format (one line per item).
  void Load(xiiStringView sFile);

private:
  xiiUInt32            m_uiMaxElements;
  xiiDeque<RecentFile> m_Files;
};
