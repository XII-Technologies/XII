#include <GuiFoundation/GuiFoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <GuiFoundation/UIServices/UIServices.moc.h>

#  include <ShlObj_core.h>

void xiiQtUiServices::OpenInExplorer(xiiStringView sPath, bool bIsFile)
{
  QStringList args;

  if (bIsFile)
    args << "/select,";

  args << QDir::toNativeSeparators(xiiMakeQString(sPath));

  QProcess::startDetached("explorer", args);
}

void xiiQtUiServices::OpenWith(xiiStringView sPath)
{
  xiiStringBuilder sPath0 = sPath;
  sPath0.MakeCleanPath();
  sPath0.MakePathSeparatorsNative();

  xiiStringWChar wpath(sPath0);
  OPENASINFO     oi;
  oi.pcszFile    = wpath.GetData();
  oi.pcszClass   = NULL;
  oi.oaifInFlags = OAIF_EXEC;
  SHOpenWithDialog(NULL, &oi);
}

xiiStatus xiiQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  QString sVsCodeExe;

  sVsCodeExe = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "Programs/Microsoft VS Code/Code.exe", QStandardPaths::LocateOption::LocateFile);

  if (!QFile().exists(sVsCodeExe))
  {
    QSettings settings("\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command", QSettings::NativeFormat);
    QString   sVsCodeExeKey = settings.value(".", "").value<QString>();

    if (sVsCodeExeKey.length() > 5)
    {
      // Remove shell parameter and normalize QT Compatible path, QFile expects the file separator to be '/' regardless of operating system
      sVsCodeExe = sVsCodeExeKey.left(sVsCodeExeKey.length() - 5).replace("\\", "/").replace("\"", "");
    }
  }

  if (sVsCodeExe.isEmpty() || !QFile().exists(sVsCodeExe))
  {
    // Try code executable in PATH
    if (QProcess::execute("code", {"--version"}) == 0)
    {
      sVsCodeExe = "code";
    }
    else
    {
      return xiiStatus("Installation of Visual Studio Code could not be located.\n"
                       "Please visit 'https://code.visualstudio.com/download' to download the 'User Installer' of Visual Studio Code.");
    }
  }

  QProcess proc;
  if (proc.startDetached(sVsCodeExe, arguments) == false)
  {
    return xiiStatus("Failed to launch Visual Studio Code.");
  }

  return xiiStatus(XII_SUCCESS);
}

#endif
