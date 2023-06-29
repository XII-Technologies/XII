#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiEventTrack;

class XII_GUIFOUNDATION_DLL xiiEventTrackControlPointData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEventTrackControlPointData, xiiReflectedClass);

public:
  xiiTime     GetTickAsTime() const { return xiiTime::Seconds(m_iTick / 4800.0); }
  void        SetTickFromTime(xiiTime time, xiiInt64 iFps);
  const char* GetEventName() const { return m_sEvent.GetData(); }
  void        SetEventName(const char* szSz) { m_sEvent.Assign(szSz); }

  xiiInt64        m_iTick; // 4800 ticks per second
  xiiHashedString m_sEvent;
};

class XII_GUIFOUNDATION_DLL xiiEventTrackData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEventTrackData, xiiReflectedClass);

public:
  xiiInt64 TickFromTime(xiiTime time) const;
  void     ConvertToRuntimeData(xiiEventTrack& out_result) const;

  xiiUInt16                                      m_uiFramesPerSecond = 60;
  xiiDynamicArray<xiiEventTrackControlPointData> m_ControlPoints;
};

class XII_GUIFOUNDATION_DLL xiiEventSet
{
public:
  bool IsModified() const { return m_bModified; }

  const xiiSet<xiiString>& GetAvailableEvents() const { return m_AvailableEvents; }

  void AddAvailableEvent(xiiStringView sEvent);

  xiiResult WriteToDDL(const char* szFile);
  xiiResult ReadFromDDL(const char* szFile);

private:
  bool              m_bModified = false;
  xiiSet<xiiString> m_AvailableEvents;
};
