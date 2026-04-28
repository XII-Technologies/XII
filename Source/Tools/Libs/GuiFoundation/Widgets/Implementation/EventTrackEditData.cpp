/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Tracks/EventTrack.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEventTrackControlPointData, 1, xiiRTTIDefaultAllocator<xiiEventTrackControlPointData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Tick", m_iTick),
    XII_ACCESSOR_PROPERTY("Event", GetEventName, SetEventName),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEventTrackData, 3, xiiRTTIDefaultAllocator<xiiEventTrackData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiEventTrackControlPointData::SetTickFromTime(xiiTime time, xiiInt64 iFps)
{
  const xiiUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick                        = (xiiInt64)xiiMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

xiiInt64 xiiEventTrackData::TickFromTime(xiiTime time) const
{
  const xiiUInt32 uiTicksPerStep = 4800 / m_uiFramesPerSecond;
  return (xiiInt64)xiiMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void xiiEventTrackData::ConvertToRuntimeData(xiiEventTrack& out_result) const
{
  out_result.Clear();

  for (const auto& cp : m_ControlPoints)
  {
    out_result.AddControlPoint(cp.GetTickAsTime(), cp.m_sEvent);
  }
}

void xiiEventSet::AddAvailableEvent(xiiStringView sEvent)
{
  if (sEvent.IsEmpty())
    return;

  if (m_AvailableEvents.Contains(sEvent))
    return;

  m_bModified = true;
  m_AvailableEvents.Insert(sEvent);
}

xiiResult xiiEventSet::WriteToDDL(xiiStringView sFile)
{
  xiiDeferredFileWriter file;
  file.SetOutput(sFile);

  xiiOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  for (const auto& s : m_AvailableEvents)
  {
    ddl.BeginObject("Event", s.GetData());
    ddl.EndObject();
  }

  if (file.Close().Succeeded())
  {
    m_bModified = false;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiEventSet::ReadFromDDL(xiiStringView sFile)
{
  m_AvailableEvents.Clear();

  xiiFileReader file;
  if (file.Open(sFile).Failed())
    return XII_FAILURE;

  xiiOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return XII_FAILURE;

  auto* pRoot = ddl.GetRootElement();

  for (auto* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Event"))
    {
      AddAvailableEvent(pChild->GetName());
    }
  }

  m_bModified = false;
  return XII_SUCCESS;
}
