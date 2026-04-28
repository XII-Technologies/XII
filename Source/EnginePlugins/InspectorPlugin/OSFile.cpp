/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/ThreadUtils.h>

static void OSFileEventHandler(const xiiOSFile::EventData& e)
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  xiiTelemetryMessage Msg;
  Msg.GetWriter() << e.m_iFileID;

  switch (e.m_EventType)
  {
    case xiiOSFile::EventType::FileOpen:
    {
      Msg.SetMessageID('FILE', 'OPEN');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << (xiiUInt8)e.m_FileMode;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case xiiOSFile::EventType::FileRead:
    {
      Msg.SetMessageID('FILE', 'READ');
      Msg.GetWriter() << e.m_uiBytesAccessed;
    }
    break;

    case xiiOSFile::EventType::FileWrite:
    {
      Msg.SetMessageID('FILE', 'WRIT');
      Msg.GetWriter() << e.m_uiBytesAccessed;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case xiiOSFile::EventType::FileClose:
    {
      Msg.SetMessageID('FILE', 'CLOS');
    }
    break;

    case xiiOSFile::EventType::FileExists:
    case xiiOSFile::EventType::DirectoryExists:
    {
      Msg.SetMessageID('FILE', 'EXST');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case xiiOSFile::EventType::FileDelete:
    {
      Msg.SetMessageID('FILE', ' DEL');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case xiiOSFile::EventType::MakeDir:
    {
      Msg.SetMessageID('FILE', 'CDIR');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case xiiOSFile::EventType::FileCopy:
    {
      Msg.SetMessageID('FILE', 'COPY');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_sFile2;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case xiiOSFile::EventType::FileStat:
    {
      Msg.SetMessageID('FILE', 'STAT');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case xiiOSFile::EventType::FileCasing:
    {
      Msg.SetMessageID('FILE', 'CASE');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case xiiOSFile::EventType::None:
      break;
  }

  xiiUInt8 uiThreadType = 0;

  if (xiiThreadUtils::IsMainThread())
    uiThreadType = 1 << 0;
  else if (xiiTaskSystem::GetCurrentThreadWorkerType() == xiiWorkerThreadType::FileAccess)
    uiThreadType = 1 << 1;
  else
    uiThreadType = 1 << 2;

  Msg.GetWriter() << e.m_Duration.GetSeconds();
  Msg.GetWriter() << uiThreadType;

  xiiTelemetry::Broadcast(xiiTelemetry::Reliable, Msg);
}

void AddOSFileEventHandler()
{
  xiiOSFile::AddEventHandler(OSFileEventHandler);
}

void RemoveOSFileEventHandler()
{
  xiiOSFile::RemoveEventHandler(OSFileEventHandler);
}


XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_OSFile);
