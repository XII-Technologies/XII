#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Utilities/RecentFilesList.h>

void xiiRecentFilesList::Insert(xiiStringView sFile, xiiInt32 iContainerWindow)
{
  xiiStringBuilder sCleanPath = sFile;
  sCleanPath.MakeCleanPath();

  xiiString s = sCleanPath;

  for (xiiUInt32 i = 0; i < m_Files.GetCount(); i++)
  {
    if (m_Files[i].m_File == s)
    {
      m_Files.RemoveAtAndCopy(i);
      break;
    }
  }
  m_Files.PushFront(RecentFile(s, iContainerWindow));

  if (m_Files.GetCount() > m_uiMaxElements)
    m_Files.SetCount(m_uiMaxElements);
}

void xiiRecentFilesList::Save(xiiStringView sFile)
{
  xiiDeferredFileWriter File;
  File.SetOutput(sFile);

  for (const RecentFile& file : m_Files)
  {
    xiiStringBuilder sTemp;
    sTemp.Format("{0}|{1}", file.m_File, file.m_iContainerWindow);
    File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
    File.WriteBytes("\n", sizeof(char)).IgnoreResult();
  }

  if (File.Close().Failed())
    xiiLog::Error("Unable to open file '{0}' for writing!", sFile);
}

void xiiRecentFilesList::Load(xiiStringView sFile)
{
  m_Files.Clear();

  xiiFileReader File;
  if (File.Open(sFile).Failed())
    return;

  xiiStringBuilder sAllLines;
  sAllLines.ReadAll(File);

  xiiHybridArray<xiiStringView, 16> Lines;
  sAllLines.Split(false, Lines, "\n");

  xiiStringBuilder sTemp, sTemp2;

  for (const xiiStringView& sv : Lines)
  {
    sTemp = sv;
    xiiHybridArray<xiiStringView, 2> Parts;
    sTemp.Split(false, Parts, "|");

    if (!xiiOSFile::ExistsFile(Parts[0].GetData(sTemp2)))
      continue;

    if (Parts.GetCount() == 1)
    {
      m_Files.PushBack(RecentFile(Parts[0], 0));
    }
    else if (Parts.GetCount() == 2)
    {
      xiiStringBuilder sContainer       = Parts[1];
      xiiInt32         iContainerWindow = 0;
      xiiConversionUtils::StringToInt(sContainer, iContainerWindow).IgnoreResult();
      m_Files.PushBack(RecentFile(Parts[0], iContainerWindow));
    }
  }
}
