/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Logging/Log.h>

void xiiFileserverApp::FileserverEventHandlerConsole(const xiiFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case xiiFileserverEvent::Type::None:
      xiiLog::Error("Invalid Fileserver event type");
      break;

    case xiiFileserverEvent::Type::ServerStarted:
    {
      xiiLog::Info("xiiFileserver is running");
    }
    break;

    case xiiFileserverEvent::Type::ServerStopped:
    {
      xiiLog::Info("xiiFileserver was shut down");
    }
    break;

    case xiiFileserverEvent::Type::ClientConnected:
    {
      xiiLog::Success("Client connected");
    }
    break;

    case xiiFileserverEvent::Type::MountDataDir:
    {
      xiiLog::Info("Mounted data directory '{0}' ({1})", e.m_szName, e.m_szPath);
    }
    break;

    case xiiFileserverEvent::Type::UnmountDataDir:
    {
      xiiLog::Info("Unmount request for data directory '{0}' ({1})", e.m_szName, e.m_szPath);
    }
    break;

    case xiiFileserverEvent::Type::FileDownloadRequest:
    {
      if (e.m_FileState == xiiFileserveFileState::NonExistant)
        xiiLog::Dev("Request: (N/A) '{0}'", e.m_szPath);

      if (e.m_FileState == xiiFileserveFileState::SameHash)
        xiiLog::Dev("Request: (HASH) '{0}'", e.m_szPath);

      if (e.m_FileState == xiiFileserveFileState::SameTimestamp)
        xiiLog::Dev("Request: (TIME) '{0}'", e.m_szPath);

      if (e.m_FileState == xiiFileserveFileState::NonExistantEither)
        xiiLog::Dev("Request: (N/AE) '{0}'", e.m_szPath);

      if (e.m_FileState == xiiFileserveFileState::Different)
        xiiLog::Info("Request: '{0}' ({1} bytes)", e.m_szPath, e.m_uiSizeTotal);
    }
    break;

    case xiiFileserverEvent::Type::FileDownloading:
    {
      xiiLog::Debug("Transfer: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
    }
    break;

    case xiiFileserverEvent::Type::FileDownloadFinished:
    {
      if (e.m_FileState == xiiFileserveFileState::Different)
        xiiLog::Info("Transfer done.");
    }
    break;

    case xiiFileserverEvent::Type::FileDeleteRequest:
    {
      xiiLog::Warning("File Deletion: '{0}'", e.m_szPath);
    }
    break;

    case xiiFileserverEvent::Type::FileUploading:
      xiiLog::Debug("Upload: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
      break;

    case xiiFileserverEvent::Type::FileUploadFinished:
      xiiLog::Info("Upload finished: {0}", e.m_szPath);
      break;

    default:
      break;
  }
}
