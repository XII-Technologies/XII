#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/CheckVersion.moc.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <QNetworkReply>
#include <QProcess>

static xiiString GetVersionFilePath()
{
  xiiStringBuilder sTemp = xiiOSFile::GetTempDataFolder();
  sTemp.AppendPath("xiiEditor/version-page.htm");
  return sTemp;
}

PageDownloader::PageDownloader(const QString& sUrl)
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  QStringList args;

  args << "-Command";
  args << QString("(Invoke-webrequest -URI \"%1\").Content > \"%2\"").arg(sUrl).arg(GetVersionFilePath().GetData());

  m_pProcess = XII_DEFAULT_NEW(QProcess);
  connect(m_pProcess.Borrow(), &QProcess::finished, this, &PageDownloader::DownloadDone);
  m_pProcess->start("C:\\Windows\\System32\\WindowsPowershell\\v1.0\\powershell.exe", args);
#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

void PageDownloader::DownloadDone(int exitCode, QProcess::ExitStatus exitStatus)
{
  m_pProcess = nullptr;

  xiiOSFile file;
  if (file.Open(GetVersionFilePath(), xiiFileOpenMode::Read).Failed())
    return;

  xiiDataBuffer content;
  file.ReadAll(content);

  content.PushBack('\0');
  content.PushBack('\0');

  const xiiUInt16* pStart = (xiiUInt16*)content.GetData();
  if (xiiUnicodeUtils::SkipUtf16BomLE(pStart))
  {
    m_sDownloadedPage = xiiStringWChar(pStart);
  }
  else
  {
    const char* szUtf8 = (const char*)content.GetData();
    m_sDownloadedPage  = xiiStringWChar(szUtf8);
  }

  xiiOSFile::DeleteFile(GetVersionFilePath()).IgnoreResult();

  Q_EMIT FinishedDownload();
}

xiiQtVersionChecker::xiiQtVersionChecker()
{
  m_sKnownLatestVersion = GetOwnVersion();
  m_sConfigFile         = ":appdata/VersionCheck.ddl";
}

void xiiQtVersionChecker::Initialize()
{
  m_bRequireOnlineCheck = true;

  xiiFileStats fs;
  if (xiiFileSystem::GetFileStats(m_sConfigFile, fs).Failed())
    return;

  xiiFileReader file;
  if (file.Open(m_sConfigFile).Failed())
    return;

  xiiOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return;

  auto pRoot = ddl.GetRootElement();
  if (pRoot == nullptr)
    return;

  auto pLatest = pRoot->FindChild("KnownLatest");
  if (pLatest == nullptr || !pLatest->HasPrimitives(xiiOpenDdlPrimitiveType::String))
    return;

  m_sKnownLatestVersion = pLatest->GetPrimitivesString()[0];

  const xiiTimestamp nextCheck = fs.m_LastModificationTime + xiiTime::Hours(24);

  if (nextCheck.Compare(xiiTimestamp::CurrentTimestamp(), xiiTimestamp::CompareMode::Newer))
  {
    // everything fine, we already checked within the last 24 hours

    m_bRequireOnlineCheck = false;
    return;
  }
}

xiiResult xiiQtVersionChecker::StoreKnownVersion()
{
  xiiFileWriter file;
  if (file.Open(m_sConfigFile).Failed())
    return XII_FAILURE;

  xiiOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);
  xiiOpenDdlUtils::StoreString(ddl, m_sKnownLatestVersion, "KnownLatest");

  return XII_SUCCESS;
}

bool xiiQtVersionChecker::Check(bool bForce)
{
#if XII_DISABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  XII_ASSERT_DEV(!bForce, "The version check is not yet implemented on this platform.");
  return false;
#endif

  if (bForce)
  {
    // to trigger a 'new release available' signal
    m_sKnownLatestVersion = GetOwnVersion();
    m_bForceCheck         = true;
  }

  if (m_bCheckInProgresss)
    return false;

  if (!bForce && !m_bRequireOnlineCheck)
  {
    Q_EMIT VersionCheckCompleted(false, false);
    return false;
  }

  m_bCheckInProgresss = true;

  m_pVersionPage = XII_DEFAULT_NEW(PageDownloader, "https://xiiengine.net/pages/getting-started/binaries.html");

  connect(m_pVersionPage.Borrow(), &PageDownloader::FinishedDownload, this, &xiiQtVersionChecker::PageDownloaded);

  return true;
}

const char* xiiQtVersionChecker::GetOwnVersion() const
{
  return XII_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_MAJOR) "." XII_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_MINOR) "." XII_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_PATCH);
}

const char* xiiQtVersionChecker::GetKnownLatestVersion() const
{
  return m_sKnownLatestVersion;
}

bool xiiQtVersionChecker::IsLatestNewer() const
{
  const char* szParsePos;
  xiiUInt32   own[3] = {0, 0, 0};
  xiiUInt32   cur[3] = {0, 0, 0};

  szParsePos = GetOwnVersion();
  for (xiiUInt32 i : {0, 1, 2})
  {
    if (xiiConversionUtils::StringToUInt(szParsePos, own[i], &szParsePos).Failed())
      break;

    if (*szParsePos == '.')
      ++szParsePos;
    else
      break;
  }

  szParsePos = GetKnownLatestVersion();
  for (xiiUInt32 i : {0, 1, 2})
  {
    if (xiiConversionUtils::StringToUInt(szParsePos, cur[i], &szParsePos).Failed())
      break;

    if (*szParsePos == '.')
      ++szParsePos;
    else
      break;
  }

  // 'major'
  if (own[0] > cur[0])
    return false;
  if (own[0] < cur[0])
    return true;

  // 'minor'
  if (own[1] > cur[1])
    return false;
  if (own[1] < cur[1])
    return true;

  // 'patch'
  return own[2] < cur[2];
}

void xiiQtVersionChecker::PageDownloaded()
{
  m_bCheckInProgresss    = false;
  xiiStringBuilder sPage = m_pVersionPage->GetDownloadedData();

  m_pVersionPage = nullptr;

  if (sPage.IsEmpty())
  {
    xiiLog::Warning("Could not download release notes page.");
    return;
  }

  const char* szVersionStartTag = "<!--<VERSION>-->";
  const char* szVersionEndTag   = "<!--</VERSION>-->";

  const char* pVersionStart = sPage.FindSubString(szVersionStartTag);
  const char* pVersionEnd   = sPage.FindSubString(szVersionEndTag, pVersionStart);

  if (pVersionStart == nullptr || pVersionEnd == nullptr)
  {
    xiiLog::Warning("Version check failed.");
    return;
  }

  xiiStringBuilder sVersion;
  sVersion.SetSubString_FromTo(pVersionStart + xiiStringUtils::GetStringElementCount(szVersionStartTag), pVersionEnd);

  const bool bNewRelease = m_sKnownLatestVersion != sVersion;

  // make sure to modify the file, even if the version is the same
  m_sKnownLatestVersion = sVersion;
  if (StoreKnownVersion().Failed())
  {
    xiiLog::Warning("Could not store the last known version file.");
  }

  Q_EMIT VersionCheckCompleted(bNewRelease, m_bForceCheck);
}
