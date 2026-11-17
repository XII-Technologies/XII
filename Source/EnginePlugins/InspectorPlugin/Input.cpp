/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>

namespace
{
  void Collect(xiiTelemetryService& service, xiiUInt64 id)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = id;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();

    xiiDynamicArray<xiiStringView> slots;
    xiiInputManager::RetrieveAllKnownInputSlots(slots);
    for (const auto& name : slots)
    {
      if (sample.m_Records.GetCount() == 4096)
      {
        ++sample.m_uiDropped;
        continue;
      }
      auto& row   = sample.m_Records.ExpandAndGetRef().m_Fields;
      float value = 0;
      row.Insert("kind", xiiUInt8(0));
      row.Insert("name", xiiString(name));
      row.Insert("state", static_cast<xiiUInt8>(xiiInputManager::GetInputSlotState(name, &value)));
      row.Insert("value", value);
      row.Insert("flags", static_cast<xiiUInt32>(xiiInputManager::GetInputSlotFlags(name).GetValue()));
      row.Insert("deadZone", xiiInputManager::GetInputSlotDeadZone(name));
    }
    xiiDynamicArray<xiiString> sets;
    xiiInputManager::GetAllInputSets(sets);
    for (const auto& set : sets)
    {
      xiiHybridArray<xiiString, 24> actions;
      xiiInputManager::GetAllInputActions(set, actions);
      for (const auto& name : actions)
      {
        if (sample.m_Records.GetCount() == 4096)
        {
          ++sample.m_uiDropped;
          continue;
        }
        auto&      row    = sample.m_Records.ExpandAndGetRef().m_Fields;
        const auto config = xiiInputManager::GetInputActionConfig(set, name);
        float      value  = 0;
        row.Insert("kind", xiiUInt8(1));
        row.Insert("name", name);
        row.Insert("set", set);
        row.Insert("state", static_cast<xiiUInt8>(xiiInputManager::GetInputActionState(set, name, &value)));
        row.Insert("value", value);
        row.Insert("timeScaling", config.m_bApplyTimeScaling);
        for (xiiUInt32 i = 0; i < xiiInputActionConfig::MaxInputSlotAlternatives; ++i)
        {
          xiiStringBuilder key;
          key.SetFormat("trigger{}", i);
          row.Insert(key, config.m_sInputSlotTrigger[i]);
          key.SetFormat("scale{}", i);
          row.Insert(key, config.m_fInputSlotScale[i]);
        }
      }
    }
    service.Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddInputEventHandler()
{
  auto&                  service = xiiApplication::GetApplicationInstance()->CreateTelemetryService();
  xiiTelemetrySourceInfo info;
  info.m_uiId                          = xiiTelemetrySources::Input;
  info.m_sName                         = "Input slots and actions";
  info.m_sCategory                     = "Input";
  info.m_uiMinimumIntervalMilliseconds = 100;
  info.m_bSnapshot                     = true;
  info.m_bLatestOnly                   = true;
  XII_VERIFY(service.RegisterSource(info, Collect).Succeeded(), "Duplicate diagnostics source.");
}

void RemoveInputEventHandler()
{
  if (auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService())
    service->UnregisterSource(xiiTelemetrySources::Input);
}
