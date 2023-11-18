#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <ToolsFoundation/FileSystem/DataDirPath.h>

bool xiiDataDirPath::UpdateDataDirInfos(xiiArrayPtr<xiiString> dataDirRoots, xiiUInt32 uiLastKnownDataDirIndex /*= 0*/) const
{
  const xiiUInt32 uiCount = dataDirRoots.GetCount();
  for (xiiUInt32 i = 0; i < uiCount; ++i)
  {
    xiiUInt32 uiCurrentIndex = (uiLastKnownDataDirIndex + i) % uiCount;
    XII_ASSERT_DEBUG(!dataDirRoots[uiCurrentIndex].EndsWith_NoCase("/"), "");
    if (m_sAbsolutePath.StartsWith_NoCase(dataDirRoots[uiCurrentIndex]) && !dataDirRoots[uiCurrentIndex].IsEmpty())
    {
      m_uiDataDirIndex           = uiCurrentIndex;
      const char* szParentFolder = xiiPathUtils::FindPreviousSeparator(m_sAbsolutePath.GetData(), m_sAbsolutePath.GetData() + dataDirRoots[uiCurrentIndex].GetElementCount());
      m_uiDataDirParent          = szParentFolder - m_sAbsolutePath.GetData();
      m_uiDataDirLength          = dataDirRoots[uiCurrentIndex].GetElementCount() - m_uiDataDirParent;
      return true;
    }
  }

  m_uiDataDirParent = 0;
  m_uiDataDirLength = 0;
  m_uiDataDirIndex  = 0;
  return false;
}
