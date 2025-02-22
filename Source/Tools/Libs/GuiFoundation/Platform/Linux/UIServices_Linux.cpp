#include <GuiFoundation/GuiFoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)

#  include <GuiFoundation/UIServices/UIServices.moc.h>

void xiiQtUiServices::OpenInExplorer(xiiStringView sPath, bool bIsFile)
{
  QStringList      args;
  xiiStringBuilder parentDir;

  if (bIsFile)
  {
    parentDir = sPath;
    parentDir = parentDir.GetFileDirectory();
    sPath     = parentDir.GetView();
  }
  args << QDir::toNativeSeparators(xiiMakeQString(sPath));

  QProcess::startDetached("xdg-open", args);
}

void xiiQtUiServices::OpenWith(xiiStringView sPath)
{
  xiiStringBuilder sPath0 = sPath;
  sPath0.MakeCleanPath();
  sPath0.MakePathSeparatorsNative();

  xiiLog::Error("xiiQtUiServices::OpenWith() not implemented on Linux.");
}

xiiStatus xiiQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  xiiLog::Error("xiiQtUiServices::OpenInVsCode() not implemented on Linux.");
  return xiiStatus(XII_FAILURE);
}

#endif
