/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/ThreadUtils.h>

namespace
{
  void FileActivity(const xiiOSFile::EventData& event)
  {
    auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService();
    if (!service || !service->IsCollecting(xiiTelemetrySources::Files))
      return;
    xiiTelemetryFileOperation operation;
    switch (event.m_EventType)
    {
      case xiiOSFile::EventType::FileOpen:
        operation = xiiTelemetryFileOperation::Open;
        break;
      case xiiOSFile::EventType::FileClose:
        operation = xiiTelemetryFileOperation::Close;
        break;
      case xiiOSFile::EventType::FileRead:
        operation = xiiTelemetryFileOperation::Read;
        break;
      case xiiOSFile::EventType::FileWrite:
        operation = xiiTelemetryFileOperation::Write;
        break;
      case xiiOSFile::EventType::FileExists:
      case xiiOSFile::EventType::DirectoryExists:
        operation = xiiTelemetryFileOperation::Exists;
        break;
      case xiiOSFile::EventType::FileDelete:
        operation = xiiTelemetryFileOperation::Delete;
        break;
      case xiiOSFile::EventType::MakeDir:
        operation = xiiTelemetryFileOperation::CreateDirectory;
        break;
      case xiiOSFile::EventType::FileCopy:
        operation = xiiTelemetryFileOperation::Copy;
        break;
      case xiiOSFile::EventType::FileStat:
        operation = xiiTelemetryFileOperation::Stat;
        break;
      case xiiOSFile::EventType::FileCasing:
        operation = xiiTelemetryFileOperation::Casing;
        break;
      default:
        return;
    }
    xiiTelemetrySample sample;
    sample.m_uiSource               = xiiTelemetrySources::Files;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    auto& fields                    = sample.m_Records.ExpandAndGetRef().m_Fields;
    fields.Insert("operation", static_cast<xiiUInt8>(operation));
    fields.Insert("id", event.m_iFileID);
    fields.Insert("path", xiiString(event.m_sFile));
    fields.Insert("destination", xiiString(event.m_sFile2));
    fields.Insert("mode", static_cast<xiiUInt8>(event.m_FileMode));
    fields.Insert("success", event.m_bSuccess);
    fields.Insert("bytes", event.m_uiBytesAccessed);
    fields.Insert("duration", event.m_Duration);
    fields.Insert("threads", xiiUInt8(xiiThreadUtils::IsMainThread() ? 1 : xiiTaskSystem::GetCurrentThreadWorkerType() == xiiWorkerThreadType::FileAccess ? 2 :
                                                                                                                                                            4));
    service->Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddOSFileEventHandler()
{
  xiiTelemetrySourceInfo source;
  source.m_uiId                          = xiiTelemetrySources::Files;
  source.m_sName                         = "File activity";
  source.m_sCategory                     = "IO";
  source.m_uiMinimumIntervalMilliseconds = 33;
  XII_VERIFY(xiiApplication::GetApplicationInstance()->CreateTelemetryService().RegisterSource(source).Succeeded(), "Duplicate file activity source.");
  xiiOSFile::AddEventHandler(FileActivity);
}

void RemoveOSFileEventHandler()
{
  xiiOSFile::RemoveEventHandler(FileActivity);
  xiiApplication::GetApplicationInstance()->GetTelemetryService()->UnregisterSource(xiiTelemetrySources::Files);
}
