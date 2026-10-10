/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineRuntime.h>

namespace
{
  xiiResult ApplySettings(const xiiVariantDictionary& properties, const xiiRTTI* type, void* settings, bool display = false)
  {
    for (const auto& entry : properties)
    {
      if (display && (entry.Key() == "Exposure" || entry.Key() == "ColorGrading")) continue;
      const auto* property = type->FindPropertyByName(entry.Key());
      if (!property || property->GetCategory() != xiiPropertyCategory::Member || property->GetFlags().IsSet(xiiPropertyFlags::ReadOnly)) return XII_FAILURE;
      const auto* member  = static_cast<const xiiAbstractMemberProperty*>(property);
      const auto  current = xiiReflectionUtils::GetMemberPropertyValue(member, settings);
      if (!current.IsValid()) return XII_FAILURE;
      xiiResult conversion = XII_FAILURE;
      auto      converted  = entry.Value().ConvertTo(current.GetType(), &conversion);
      XII_SUCCEED_OR_RETURN(conversion);
      xiiReflectionUtils::SetMemberPropertyValue(member, settings, converted);
    }
    return XII_SUCCESS;
  }
} // namespace

xiiResult xiiResolvePipelineDisplaySettings(const xiiVariantDictionary& parameters, xiiDisplayOutputSettings& inout_settings)
{
  const auto it = parameters.Find("DisplayOutput");
  if (!it.IsValid()) return XII_SUCCESS;
  if (!it.Value().IsA<xiiVariantDictionary>()) return XII_FAILURE;
  auto        replacement = inout_settings;
  const auto& properties  = it.Value().Get<xiiVariantDictionary>();
  for (const auto& entry : properties)
  {
    if (entry.Key() != "Exposure" && entry.Key() != "ColorGrading") continue;
    if (!entry.Value().IsA<xiiVariantDictionary>()) return XII_FAILURE;
    if (entry.Key() == "Exposure")
    {
      XII_SUCCEED_OR_RETURN(ApplySettings(entry.Value().Get<xiiVariantDictionary>(), xiiGetStaticRTTI<xiiExposureSettings>(), &replacement.m_Exposure));
    }
    else
    {
      XII_SUCCEED_OR_RETURN(ApplySettings(entry.Value().Get<xiiVariantDictionary>(), xiiGetStaticRTTI<xiiColorGradingSettings>(), &replacement.m_ColorGrading));
    }
  }
  XII_SUCCEED_OR_RETURN(ApplySettings(properties, xiiGetStaticRTTI<xiiDisplayOutputSettings>(), &replacement, true));
  if (!xiiDisplayOutputManager::IsValid(replacement)) return XII_FAILURE;
  inout_settings = replacement;
  return XII_SUCCESS;
}
