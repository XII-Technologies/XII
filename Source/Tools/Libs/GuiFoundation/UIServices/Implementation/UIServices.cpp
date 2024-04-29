#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Stopwatch.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QUrl>

XII_IMPLEMENT_SINGLETON(xiiQtUiServices);

xiiEvent<const xiiQtUiServices::Event&>     xiiQtUiServices::s_Events;
xiiEvent<const xiiQtUiServices::TickEvent&> xiiQtUiServices::s_TickEvent;

xiiMap<xiiString, QIcon>   xiiQtUiServices::s_IconsCache;
xiiMap<xiiString, QImage>  xiiQtUiServices::s_ImagesCache;
xiiMap<xiiString, QPixmap> xiiQtUiServices::s_PixmapsCache;
bool                       xiiQtUiServices::s_bHeadless;
xiiQtUiServices::TickEvent xiiQtUiServices::s_LastTickEvent;

static xiiQtUiServices* g_pInstance = nullptr;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtUiServices)

  ON_CORESYSTEMS_STARTUP
  {
    g_pInstance = XII_DEFAULT_NEW(xiiQtUiServices);
    xiiQtUiServices::GetSingleton()->Init();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    XII_DEFAULT_DELETE(g_pInstance);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiQtUiServices::xiiQtUiServices() :
  m_SingletonRegistrar(this)
{
  qRegisterMetaType<xiiUuid>();
  m_pColorDlg = nullptr;
}


bool xiiQtUiServices::IsHeadless()
{
  return s_bHeadless;
}


void xiiQtUiServices::SetHeadless(bool bHeadless)
{
  s_bHeadless = true;
}

void xiiQtUiServices::SaveState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    Settings.setValue("ColorDlgGeom", m_ColorDlgGeometry);
  }
  Settings.endGroup();
}

xiiTime g_Total = xiiTime::Zero();

const QIcon& xiiQtUiServices::GetCachedIconResource(xiiStringView sIdentifier, xiiColor svgTintColor)
{
  xiiStringBuilder sFullIdentifier = sIdentifier;
  auto&            map             = s_IconsCache;

  const bool bNeedsColoring = svgTintColor != xiiColor::ZeroColor() && sIdentifier.EndsWith_NoCase(".svg");

  if (bNeedsColoring)
  {
    sFullIdentifier.AppendFormat("-{}", xiiColorGammaUB(svgTintColor));
  }

  auto it = map.Find(sFullIdentifier);

  if (it.IsValid())
    return it.Value();

  if (bNeedsColoring)
  {
    xiiStopwatch sw;

    // read the icon from the Qt virtual file system (QResource)
    QFile file(xiiString(sIdentifier).GetData());
    if (!file.open(QIODeviceBase::OpenModeFlag::ReadOnly))
    {
      // if it doesn't exist, return an empty QIcon

      map[sFullIdentifier] = QIcon();
      return map[sFullIdentifier];
    }

    // get the entire SVG file content
    xiiStringBuilder sContent = QString(file.readAll()).toUtf8().data();

    // replace the occurrence of the color white ("#FFFFFF") with the desired target color
    {
      const xiiColorGammaUB color8 = svgTintColor;

      xiiStringBuilder rep;
      rep.SetFormat("#{}{}{}", xiiArgI((int)color8.r, 2, true, 16), xiiArgI((int)color8.g, 2, true, 16), xiiArgI((int)color8.b, 2, true, 16));

      sContent.ReplaceAll_NoCase("#ffffff", rep);
    }

    // hash the content AFTER the color replacement, so it includes the custom color change
    const xiiUInt32 uiSrcHash = xiiHashingUtils::xxHash32String(sContent);

    // file the path to the temp file, including the source hash
    const xiiStringBuilder sTempFolder = xiiOSFile::GetTempDataFolder("xiiEditor/QIcons");
    xiiStringBuilder       sTempIconFile(sTempFolder, "/", sIdentifier.GetFileName());
    sTempIconFile.AppendFormat("-{}.svg", uiSrcHash);

    // only write to the file system, if the target file doesn't exist yet, this saves more than half the time
    if (!xiiOSFile::ExistsFile(sTempIconFile))
    {
      // now write the new SVG file back to a dummy file
      // yes, this is as stupid as it sounds, we really write the file BACK TO THE FILESYSTEM, rather than doing this stuff in-memory
      // that's because I wasn't able to figure out whether we can somehow read a QIcon from a string rather than from file
      // it doesn't appear to be easy at least, since we can only give it a path, not a memory stream or anything like that
      {
        // necessary for Qt to be able to write to the folder
        xiiOSFile::CreateDirectoryStructure(sTempFolder).AssertSuccess();

        QFile fileOut(sTempIconFile.GetData());
        fileOut.open(QIODeviceBase::OpenModeFlag::WriteOnly);
        fileOut.write(sContent.GetData(), sContent.GetElementCount());
        fileOut.flush();
        fileOut.close();
      }
    }

    QIcon icon(sTempIconFile.GetData());

    if (!icon.pixmap(QSize(16, 16)).isNull())
      map[sFullIdentifier] = icon;
    else
      map[sFullIdentifier] = QIcon();

    xiiTime local = sw.GetRunningTotal();
    g_Total += local;

    // kept here for debug purposes, but don't waste time on logging
    // xiiLog::Info("Icon load time: {}, total = {}", local, g_Total);
  }
  else
  {
    const QString sFile = xiiString(sIdentifier).GetData();

    if (QFile::exists(sFile)) // prevent Qt from spamming warnings about non-existing files by checking this manually
    {
      QIcon icon(sFile);

      // Workaround for QIcon being stupid and treating failed to load icons as not-null.
      if (!icon.pixmap(QSize(16, 16)).isNull())
        map[sFullIdentifier] = icon;
      else
        map[sFullIdentifier] = QIcon();
    }
    else
      map[sFullIdentifier] = QIcon();
  }

  return map[sFullIdentifier];
}


const QImage& xiiQtUiServices::GetCachedImageResource(xiiStringView sIdentifier)
{
  auto& map = s_ImagesCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QImage(xiiMakeQString(sIdentifier));

  return map[sIdentifier];
}

const QPixmap& xiiQtUiServices::GetCachedPixmapResource(xiiStringView sIdentifier)
{
  auto& map = s_PixmapsCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QPixmap(xiiMakeQString(sIdentifier));

  return map[sIdentifier];
}

xiiResult xiiQtUiServices::AddToGitIgnore(xiiStringView sGitIgnoreFile, xiiStringView sPattern)
{
  xiiStringBuilder ignoreFile;

  {
    xiiFileReader file;
    if (file.Open(sGitIgnoreFile).Succeeded())
    {
      ignoreFile.ReadAll(file);
    }
  }

  ignoreFile.Trim("\n\r");

  const xiiUInt32 len = sPattern.GetElementCount();

  // pattern already present ?
  if (const char* szFound = ignoreFile.FindSubString(sPattern))
  {
    if (szFound == ignoreFile.GetData() || // right at the start
        *(szFound - 1) == '\n')            // after a new line
    {
      const char end = *(szFound + len);

      if (end == '\0' || end == '\r' || end == '\n') // line does not continue with an extended pattern
      {
        return XII_SUCCESS;
      }
    }
  }

  ignoreFile.AppendWithSeparator("\n", sPattern);
  ignoreFile.Append("\n\n");

  {
    xiiFileWriter file;
    XII_SUCCEED_OR_RETURN(file.Open(sGitIgnoreFile));

    XII_SUCCEED_OR_RETURN(file.WriteBytes(ignoreFile.GetData(), ignoreFile.GetElementCount()));
  }

  return XII_SUCCESS;
}

void xiiQtUiServices::CheckForUpdates()
{
  Event e;
  e.m_Type = Event::Type::CheckForUpdates;
  s_Events.Broadcast(e);
}

void xiiQtUiServices::Init()
{
  s_LastTickEvent.m_fRefreshRate = 60.0;
  if (QScreen* pScreen = QApplication::primaryScreen())
  {
    s_LastTickEvent.m_fRefreshRate = pScreen->refreshRate();
  }

  QTimer::singleShot((xiiInt32)xiiMath::Floor(1000.0 / s_LastTickEvent.m_fRefreshRate), this, SLOT(TickEventHandler()));
}

void xiiQtUiServices::TickEventHandler()
{
  XII_PROFILE_SCOPE("TickEvent");

  XII_ASSERT_DEV(!m_bIsDrawingATM, "Implementation error");
  xiiTime startTime = xiiTime::Now();

  m_bIsDrawingATM = true;
  s_LastTickEvent.m_uiFrame++;
  s_LastTickEvent.m_Time = startTime;
  s_LastTickEvent.m_Type = TickEvent::Type::StartFrame;
  s_TickEvent.Broadcast(s_LastTickEvent);

  s_LastTickEvent.m_Type = TickEvent::Type::EndFrame;
  s_TickEvent.Broadcast(s_LastTickEvent);
  m_bIsDrawingATM = false;

  const xiiTime endTime       = xiiTime::Now();
  xiiTime       lastFrameTime = endTime - startTime;

  xiiTime delay = xiiTime::Milliseconds(1000.0 / s_LastTickEvent.m_fRefreshRate);
  delay -= lastFrameTime;
  delay = xiiMath::Max(delay, xiiTime::Zero());

  QTimer::singleShot((xiiInt32)xiiMath::Floor(delay.GetMilliseconds()), this, SLOT(TickEventHandler()));
}

void xiiQtUiServices::LoadState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    m_ColorDlgGeometry = Settings.value("ColorDlgGeom").toByteArray();
  }
  Settings.endGroup();
}

void xiiQtUiServices::ShowAllDocumentsTemporaryStatusBarMessage(const xiiFormatString& msg, xiiTime timeOut)
{
  xiiStringBuilder tmp;

  Event e;
  e.m_Type  = Event::ShowDocumentTemporaryStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time  = timeOut;

  s_Events.Broadcast(e, 1);
}

void xiiQtUiServices::ShowAllDocumentsPermanentStatusBarMessage(const xiiFormatString& msg, Event::TextType type)
{
  xiiStringBuilder tmp;

  Event e;
  e.m_Type     = Event::ShowDocumentPermanentStatusBarText;
  e.m_sText    = msg.GetText(tmp);
  e.m_TextType = type;

  s_Events.Broadcast(e, 1);
}

void xiiQtUiServices::ShowGlobalStatusBarMessage(const xiiFormatString& msg)
{
  xiiStringBuilder tmp;

  Event e;
  e.m_Type  = Event::ShowGlobalStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time  = xiiTime::Seconds(0);

  s_Events.Broadcast(e);
}


bool xiiQtUiServices::OpenFileInDefaultProgram(xiiStringView sPath)
{
  return QDesktopServices::openUrl(QUrl::fromLocalFile(xiiMakeQString(sPath)));
}

void xiiQtUiServices::OpenInExplorer(xiiStringView sPath, bool bIsFile)
{
  QStringList args;

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  if (bIsFile)
    args << "/select,";

  args << QDir::toNativeSeparators(xiiMakeQString(sPath));

  QProcess::startDetached("explorer", args);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  xiiStringBuilder parentDir;

  if (bIsFile)
  {
    parentDir = sPath;
    parentDir = parentDir.GetFileDirectory();
    sPath     = parentDir.GetView();
  }
  args << QDir::toNativeSeparators(sPath);

  QProcess::startDetached("xdg-open", args);
#else
  XII_ASSERT_NOT_IMPLEMENTED
#endif
}

xiiStatus xiiQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  QString sVsCodeExe =
    QStandardPaths::locate(QStandardPaths::GenericDataLocation, "Programs/Microsoft VS Code/Code.exe", QStandardPaths::LocateOption::LocateFile);

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

  if (!QFile().exists(sVsCodeExe))
  {
    return xiiStatus("Installation of Visual Studio Code could not be located.\n"
                     "Please visit 'https://code.visualstudio.com/download' to download the 'User Installer' of Visual Studio Code.");
  }

  QProcess proc;
  if (proc.startDetached(sVsCodeExe, arguments) == false)
  {
    return xiiStatus("Failed to launch Visual Studio Code.");
  }

  return xiiStatus(XII_SUCCESS);
}
