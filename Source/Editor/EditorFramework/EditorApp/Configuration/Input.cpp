#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void xiiQtEditorApp::GetKnownInputSlots(xiiDynamicArray<xiiString>& slotList) const
{
  if (slotList.IndexOf("") == xiiInvalidIndex)
    slotList.PushBack("");

  xiiStringBuilder               sFile;
  xiiDynamicArray<xiiStringView> Lines;

  xiiStringBuilder sSearchDir = xiiApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("InputSlots/*.txt");

  xiiFileSystemIterator it;
  for (it.StartSearch(sSearchDir, xiiFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
  {
    sFile = it.GetCurrentPath();
    sFile.AppendPath(it.GetStats().m_sName);

    xiiFileReader reader;
    if (reader.Open(sFile).Succeeded())
    {
      sFile.ReadAll(reader);

      Lines.Clear();
      sFile.Split(false, Lines, "\n", "\r");

      xiiString sSlot;
      for (xiiUInt32 s = 0; s < Lines.GetCount(); ++s)
      {
        sSlot = Lines[s];

        if (slotList.IndexOf(sSlot) == xiiInvalidIndex)
          slotList.PushBack(sSlot);
      }
    }
  }
}
