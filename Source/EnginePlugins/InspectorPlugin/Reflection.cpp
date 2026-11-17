/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Reflection/Reflection.h>

namespace
{
  static xiiStringView GetParentType(const xiiRTTI* pRTTI)
  {
    if (pRTTI->GetParentType())
    {
      return pRTTI->GetParentType()->GetTypeName();
    }

    if ((pRTTI->GetTypeName() == "bool") || (pRTTI->GetTypeName() == "float") || (pRTTI->GetTypeName() == "double") || (pRTTI->GetTypeName() == "xiiInt8") || (pRTTI->GetTypeName() == "xiiUInt8") || (pRTTI->GetTypeName() == "xiiInt16") || (pRTTI->GetTypeName() == "xiiUInt16") || (pRTTI->GetTypeName() == "xiiInt32") || (pRTTI->GetTypeName() == "xiiUInt32") || (pRTTI->GetTypeName() == "xiiInt64") || (pRTTI->GetTypeName() == "xiiUInt64") || (pRTTI->GetTypeName() == "xiiConstCharPtr") || (pRTTI->GetTypeName() == "xiiVec2") || (pRTTI->GetTypeName() == "xiiVec3") || (pRTTI->GetTypeName() == "xiiVec4") || (pRTTI->GetTypeName() == "xiiMat3") || (pRTTI->GetTypeName() == "xiiMat4") || (pRTTI->GetTypeName() == "xiiVec2d") || (pRTTI->GetTypeName() == "xiiVec3d") || (pRTTI->GetTypeName() == "xiiVec4d") || (pRTTI->GetTypeName() == "xiiMat3d") || (pRTTI->GetTypeName() == "xiiMat4d") || (pRTTI->GetTypeName() == "xiiVec2I") || (pRTTI->GetTypeName() == "xiiVec3I") || (pRTTI->GetTypeName() == "xiiVec4I") || (pRTTI->GetTypeName() == "xiiVec2U") || (pRTTI->GetTypeName() == "xiiVec3U") || (pRTTI->GetTypeName() == "xiiVec4U") || (pRTTI->GetTypeName() == "xiiVec2I64") || (pRTTI->GetTypeName() == "xiiVec3I64") || (pRTTI->GetTypeName() == "xiiVec4I64") || (pRTTI->GetTypeName() == "xiiVec2U64") || (pRTTI->GetTypeName() == "xiiVec3U64") || (pRTTI->GetTypeName() == "xiiVec4U64") || (pRTTI->GetTypeName() == "xiiTime") || (pRTTI->GetTypeName() == "xiiUuid") || (pRTTI->GetTypeName() == "xiiColor") || (pRTTI->GetTypeName() == "xiiVariant") || (pRTTI->GetTypeName() == "xiiQuat") || (pRTTI->GetTypeName() == "xiiQuatd"))
    {
      return "Basic Types";
    }

    return {};
  }

  void Collect(xiiTelemetryService& service, xiiUInt64 source)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = source;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    auto& basic                     = sample.m_Records.ExpandAndGetRef().m_Fields;
    basic.Insert("name", xiiString("Basic Types"));
    basic.Insert("parent", xiiString());
    basic.Insert("plugin", xiiString());
    basic.Insert("size", xiiUInt32(0));
    basic.Insert("properties", xiiUInt32(0));
    xiiRTTI::ForEachType([&](const xiiRTTI* type) {
      if (sample.m_Records.GetCount() == 4096)
      {
        ++sample.m_uiDropped;
        return;
      }
      auto& fields = sample.m_Records.ExpandAndGetRef().m_Fields;
      fields.Insert("name", xiiString(type->GetTypeName()));
      fields.Insert("parent", xiiString(GetParentType(type)));
      fields.Insert("plugin", xiiString(type->GetPluginName()));
      fields.Insert("size", type->GetTypeSize());
      const auto properties = type->GetProperties();
      const auto count      = xiiMath::Min(properties.GetCount(), 64U);
      fields.Insert("properties", count);
      sample.m_uiDropped += properties.GetCount() - count;
      for (xiiUInt32 i = 0; i < count; ++i)
      {
        xiiStringBuilder key;
        key.SetFormat("name{}", i);
        fields.Insert(key, xiiString(properties[i]->GetPropertyName()));
        key.SetFormat("category{}", i);
        fields.Insert(key, static_cast<xiiInt8>(properties[i]->GetCategory()));
        key.SetFormat("type{}", i);
        fields.Insert(key, properties[i]->GetSpecificType() ? xiiString(properties[i]->GetSpecificType()->GetTypeName()) : xiiString("<Unknown Type>"));
      }
    });
    service.Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddReflectionEventHandler()
{
  xiiTelemetrySourceInfo info;
  info.m_uiId                          = xiiTelemetrySources::Reflection;
  info.m_sName                         = "Reflection metadata";
  info.m_sCategory                     = "Runtime";
  info.m_uiMinimumIntervalMilliseconds = 5000;
  info.m_bSnapshot = info.m_bLatestOnly = true;
  XII_VERIFY(xiiApplication::GetApplicationInstance()->CreateTelemetryService().RegisterSource(info, Collect).Succeeded(), "Duplicate reflection source.");
}
void RemoveReflectionEventHandler()
{
  xiiApplication::GetApplicationInstance()->GetTelemetryService()->UnregisterSource(xiiTelemetrySources::Reflection);
}
