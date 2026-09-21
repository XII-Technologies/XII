/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Strings/String.h>

#include <Foundation/Types/UniquePtr.h>

#include <QObject>
#include <QProcess>

class PageDownloader : public QObject
{
  Q_OBJECT

public:
  explicit PageDownloader(const QString& sUrl);

  xiiStringView GetDownloadedData() const { return m_sDownloadedPage; }

signals:
  void FinishedDownload();

private slots:
  void DownloadDone(int exitCode, QProcess::ExitStatus exitStatus);

private:
  xiiUniquePtr<QProcess> m_pProcess;
  xiiStringBuilder       m_sDownloadedPage;
};

/// Downloads a web page and checks whether the latest version online is newer than the current one
class xiiQtVersionChecker : public QObject
{
  Q_OBJECT

public:
  xiiQtVersionChecker();

  void Initialize();

  bool Check(bool bForce);

  const char* GetOwnVersion() const;
  const char* GetKnownLatestVersion() const;

  bool IsLatestNewer() const;

Q_SIGNALS:
  void VersionCheckCompleted(bool bNewRelease, bool bForced);

private slots:
  void PageDownloaded();

  xiiResult StoreKnownVersion();

private:
  bool                         m_bRequireOnlineCheck = true;
  bool                         m_bForceCheck         = false;
  bool                         m_bCheckInProgresss   = false;
  xiiString                    m_sConfigFile;
  xiiString                    m_sKnownLatestVersion;
  xiiUniquePtr<PageDownloader> m_pVersionPage;
};
