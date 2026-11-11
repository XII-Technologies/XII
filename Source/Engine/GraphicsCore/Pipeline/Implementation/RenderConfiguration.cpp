/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <GraphicsCore/Lighting/DDGIRenderSettings.h>
#include <GraphicsCore/Lighting/GlobalIlluminationRenderSettings.h>
#include <GraphicsCore/Lighting/PostProcessRenderSettings.h>
#include <GraphicsCore/Lighting/ReflectionRenderSettings.h>
#include <GraphicsCore/Lighting/ShadowRenderSettings.h>
#include <GraphicsCore/Pipeline/DynamicResolutionSettings.h>
#include <GraphicsCore/Pipeline/RenderConfiguration.h>

#include <bit>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderGlobalIllumination, 1)
  XII_ENUM_CONSTANTS(xiiRenderGlobalIllumination::PipelineDefault, xiiRenderGlobalIllumination::Disabled,
    xiiRenderGlobalIllumination::ScreenSpace, xiiRenderGlobalIllumination::DDGI, xiiRenderGlobalIllumination::RayTraced)
XII_END_STATIC_REFLECTED_ENUM;
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderFeaturePolicy, 1)
  XII_ENUM_CONSTANTS(xiiRenderFeaturePolicy::Inherit, xiiRenderFeaturePolicy::PipelineDefault, xiiRenderFeaturePolicy::Disabled)
XII_END_STATIC_REFLECTED_ENUM;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderSettingSource, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderSettingSource>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SubsystemType", m_sType),
    XII_MEMBER_PROPERTY("PropertyPath", m_sPath),
    XII_MEMBER_PROPERTY("Scope", m_uiScope),
  } XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderConfiguration, xiiNoBase, 6, xiiRTTIDefaultAllocator<xiiRenderConfiguration>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Sources", m_Sources)->AddAttributes(new xiiTemporaryAttribute(), new xiiHiddenAttribute()),
    XII_ARRAY_MEMBER_PROPERTY("Subsystems", m_Subsystems)->AddFlags(xiiPropertyFlags::PointerOwner),
  } XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool xiiRenderConfiguration::IsValid() const
{
  for (xiiUInt32 i = 0; i < m_Subsystems.GetCount(); ++i)
  {
    const auto* subsystem = m_Subsystems[i];
    if (!subsystem || subsystem->Validate().Failed())
      return false;
    for (xiiUInt32 j = 0; j < i; ++j)
      if (m_Subsystems[j]->GetDynamicRTTI() == subsystem->GetDynamicRTTI())
        return false;
  }
  return true;
}

xiiRenderPipelineVariant xiiRenderConfiguration::GetPipelineVariant() const
{
  xiiRenderPipelineVariant variant;
  if (const auto* gi = FindSubsystem<xiiGlobalIlluminationRenderSettings>(); gi && gi->IsOverridden("Technique"))
    variant.m_GlobalIllumination = gi->m_Technique;
  if (const auto* shadows = FindSubsystem<xiiShadowRenderSettings>(); shadows && shadows->IsOverridden("Enabled"))
    variant.m_Disabled.AddOrRemove(xiiOptionalRenderFeature::Shadows, !shadows->m_bEnabled);
  if (const auto* reflections = FindSubsystem<xiiReflectionRenderSettings>(); reflections && reflections->IsOverridden("Enabled"))
    variant.m_Disabled.AddOrRemove(xiiOptionalRenderFeature::ScreenSpaceReflections, !reflections->m_bEnabled);
  return variant;
}

void xiiRenderConfiguration::ApplyOverrides(const xiiRenderConfiguration& overrides, xiiRenderSettingScope::Enum scope)
{
  ApplySubsystemOverrides(overrides, scope);
}

namespace
{
  // Compatibility input only. Numeric sentinels never enter the current configuration model.
  struct LegacyConfiguration
  {
    float     m_fBloom = -1.0f, m_fScale = 0.0f;
    xiiUInt8  m_uiGI = 0, m_uiShadows = 0, m_uiReflections = 0;
    xiiUInt32 m_Counts[4]      = {};
    float     m_ProbeValues[3] = {0.0f, -1.0f, -1.0f};

    xiiResult Convert(xiiRenderConfiguration& target) const
    {
      if (!xiiMath::IsFinite(m_fBloom) || m_fBloom < -1.0f || (m_fBloom < 0 && m_fBloom != -1.0f) ||
          !xiiMath::IsFinite(m_fScale) || (m_fScale != 0 && (m_fScale < 0.1f || m_fScale > 1)) ||
          m_uiGI >= xiiRenderGlobalIllumination::ENUM_COUNT || m_uiShadows > 2 || m_uiReflections > 2)
        return XII_FAILURE;
      auto write = [&](const xiiRTTI* type, xiiStringView path, const xiiVariant& value) -> xiiResult {
        auto* subsystem = target.EditSubsystem(type);
        if (!subsystem)
          return XII_FAILURE;
        xiiPropertyPath property;
        XII_SUCCEED_OR_RETURN(property.InitializeFromPath(*type, path));
        property.SetValue(subsystem, *type, value);
        if (!subsystem->IsOverridden(path))
          subsystem->m_Overrides.PushBack(xiiString(path));
        return XII_SUCCESS;
      };
      if (m_fBloom != -1.0f)
        XII_SUCCEED_OR_RETURN(write(xiiGetStaticRTTI<xiiPostProcessRenderSettings>(), "Display/BloomStrength", m_fBloom));
      if (m_fScale != 0.0f)
        XII_SUCCEED_OR_RETURN(write(xiiGetStaticRTTI<xiiDynamicResolutionSettings>(), "RenderScale", m_fScale));
      if (m_uiGI != 0)
        XII_SUCCEED_OR_RETURN(write(xiiGetStaticRTTI<xiiGlobalIlluminationRenderSettings>(), "Technique", xiiInt64(m_uiGI)));
      if (m_uiShadows != 0)
        XII_SUCCEED_OR_RETURN(write(xiiGetStaticRTTI<xiiShadowRenderSettings>(), "Enabled", m_uiShadows != 2));
      if (m_uiReflections != 0)
        XII_SUCCEED_OR_RETURN(write(xiiGetStaticRTTI<xiiReflectionRenderSettings>(), "Enabled", m_uiReflections != 2));
      const xiiStringView counts[] = {"Probes/ProbeCountX", "Probes/ProbeCountY", "Probes/ProbeCountZ", "Probes/ProbeUpdateBudget"};
      for (xiiUInt32 i = 0; i < 4; ++i)
        if (m_Counts[i] != 0)
          XII_SUCCEED_OR_RETURN(write(xiiGetStaticRTTI<xiiDDGIRenderSettings>(), counts[i], m_Counts[i]));
      const xiiStringView values[] = {"Probes/ProbeSpacing", "Probes/TemporalHysteresis", "Probes/MaximumRelocationDistance"};
      for (xiiUInt32 i = 0; i < 3; ++i)
        if (m_ProbeValues[i] != (i == 0 ? 0.0f : -1.0f))
          XII_SUCCEED_OR_RETURN(write(xiiGetStaticRTTI<xiiDDGIRenderSettings>(), values[i], m_ProbeValues[i]));
      for (auto* subsystem : target.m_Subsystems)
        subsystem->m_Overrides.Sort();
      return target.IsValid() ? XII_SUCCESS : XII_FAILURE;
    }
  };

  constexpr xiiUInt32 s_uiMaximumSettingsBytes = 1024 * 1024;
} // namespace

xiiResult xiiRenderConfiguration::Serialize(xiiStreamWriter& stream) const
{
  if (!IsValid())
    return XII_FAILURE;
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  xiiAbstractObjectGraph           graph;
  xiiRttiConverterContext          context;
  xiiRttiConverterWriter           converter(&graph, &context, [](const void*, const xiiAbstractProperty* property) {
    return !property->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) && property->GetAttributeByType<xiiTemporaryAttribute>() == nullptr;
  });
  const auto*                      type = xiiGetStaticRTTI<xiiRenderConfiguration>();
  context.RegisterObject(xiiUuid::MakeStableUuidFromString("RenderConfiguration"), type, const_cast<xiiRenderConfiguration*>(this));
  converter.AddObjectToGraph(type, this, "root");
  xiiAbstractGraphDdlSerializer::Write(writer, &graph);
  const auto size = storage.GetStorageSize32();
  if (size > s_uiMaximumSettingsBytes)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&size));
  return stream.WriteBytes(storage.GetData(), size);
}

xiiResult xiiRenderConfiguration::Deserialize(xiiStreamReader& stream, xiiUInt32 uiVersion)
{
  if (uiVersion == 0 || uiVersion > s_uiSerializationVersion)
    return XII_FAILURE;
  xiiRenderConfiguration configuration;
  if (uiVersion == 5)
    return XII_FAILURE; // Unreleased intermediate format.
  if (uiVersion <= 4)
  {
    LegacyConfiguration legacy;
    xiiUInt32           bloom = 0, scale = 0;
    XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&bloom));
    XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&scale));
    legacy.m_fBloom = std::bit_cast<float>(bloom);
    legacy.m_fScale = std::bit_cast<float>(scale);
    if (uiVersion >= 2 && stream.ReadBytes(&legacy.m_uiGI, 1) != 1)
      return XII_FAILURE;
    if (uiVersion >= 3)
    {
      for (auto& count : legacy.m_Counts)
        XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&count));
      for (auto& value : legacy.m_ProbeValues)
      {
        xiiUInt32 bits = 0;
        XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&bits));
        value = std::bit_cast<float>(bits);
      }
    }
    if (uiVersion >= 4 && (stream.ReadBytes(&legacy.m_uiShadows, 1) != 1 || stream.ReadBytes(&legacy.m_uiReflections, 1) != 1))
      return XII_FAILURE;
    XII_SUCCEED_OR_RETURN(legacy.Convert(configuration));
  }
  else
  {
    xiiUInt32 size = 0;
    XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&size));
    if (size == 0 || size > s_uiMaximumSettingsBytes)
      return XII_FAILURE;
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(size);
    if (stream.ReadBytes(bytes.GetData(), size) != size)
      return XII_FAILURE;
    xiiRawMemoryStreamReader reader(bytes.GetData(), size);
    xiiAbstractObjectGraph   graph;
    XII_SUCCEED_OR_RETURN(xiiAbstractGraphDdlSerializer::Read(reader, &graph));
    const auto* root = graph.GetNodeByName("root");
    const auto* type = xiiGetStaticRTTI<xiiRenderConfiguration>();
    if (!root || root->GetType() != type->GetTypeName())
      return XII_FAILURE;
    const auto* subsystems = root->FindProperty("Subsystems");
    if (!subsystems || !subsystems->m_Value.IsA<xiiVariantArray>())
      return XII_FAILURE;
    const auto& entries = subsystems->m_Value.Get<xiiVariantArray>();
    if (entries.GetCount() > 256)
      return XII_FAILURE;
    for (const auto& entry : entries)
    {
      const auto* node      = entry.IsA<xiiUuid>() ? graph.GetNode(entry.Get<xiiUuid>()) : nullptr;
      const auto* subsystem = node ? xiiRTTI::FindTypeByName(node->GetType()) : nullptr;
      if (!subsystem || !subsystem->IsDerivedFrom<xiiRenderSubsystemSettings>() || !subsystem->GetAllocator()->CanAllocate())
        return XII_FAILURE;
    }
    xiiRttiConverterContext context;
    xiiRttiConverterReader  converter(&graph, &context);
    converter.ApplyPropertiesToObject(root, type, &configuration);
  }
  if (!configuration.IsValid())
    return XII_FAILURE;
  *this = std::move(configuration);
  return XII_SUCCESS;
}

namespace
{
  class RenderConfigurationPatch final : public xiiGraphPatch
  {
  public:
    explicit RenderConfigurationPatch(PatchType type) : xiiGraphPatch("xiiRenderConfiguration", 6, type) {}
    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph* graph, xiiAbstractObjectNode* node) const override
    {
      if (node)
        return;
      xiiHybridArray<xiiAbstractObjectNode*, 16> targets;
      for (const auto& entry : graph->GetAllNodes())
        if (entry.Value()->GetType() == "xiiRenderConfiguration" && entry.Value()->GetTypeVersion() < 6)
          targets.PushBack(entry.Value());
      for (auto* target : targets)
      {
        LegacyConfiguration legacy;
        auto                read = [](const xiiAbstractObjectNode* object, xiiStringView name, auto& value) {
          if (const auto* property = object->FindProperty(name))
            value = property->m_Value.ConvertTo<std::remove_reference_t<decltype(value)>>();
        };
        read(target, "BloomStrength", legacy.m_fBloom);
        read(target, "RenderScale", legacy.m_fScale);
        auto readEnum = [&](xiiStringView name, const xiiRTTI* type, xiiUInt8& value) {
          if (const auto* property = target->FindProperty(name))
          {
            if (property->m_Value.IsA<xiiString>())
            {
              xiiInt64 enumeration = 0;
              if (xiiReflectionUtils::StringToEnumeration(type, property->m_Value.Get<xiiString>(), enumeration))
                value = static_cast<xiiUInt8>(enumeration);
            }
            else
              value = property->m_Value.ConvertTo<xiiUInt8>();
          }
        };
        readEnum("GlobalIllumination", xiiGetStaticRTTI<xiiRenderGlobalIllumination>(), legacy.m_uiGI);
        readEnum("Shadows", xiiGetStaticRTTI<xiiRenderFeaturePolicy>(), legacy.m_uiShadows);
        readEnum("ScreenSpaceReflections", xiiGetStaticRTTI<xiiRenderFeaturePolicy>(), legacy.m_uiReflections);
        xiiUuid oldDDGI;
        if (const auto* property = target->FindProperty("DDGI"); property && property->m_Value.IsA<xiiUuid>())
        {
          oldDDGI = property->m_Value.Get<xiiUuid>();
          if (const auto* ddgi = graph->GetNode(oldDDGI))
          {
            const xiiStringView names[] = {"ProbeCountX", "ProbeCountY", "ProbeCountZ", "ProbeUpdateBudget"};
            for (xiiUInt32 i = 0; i < 4; ++i)
              read(ddgi, names[i], legacy.m_Counts[i]);
            read(ddgi, "ProbeSpacing", legacy.m_ProbeValues[0]);
            read(ddgi, "TemporalHysteresis", legacy.m_ProbeValues[1]);
            read(ddgi, "MaximumRelocationDistance", legacy.m_ProbeValues[2]);
          }
        }
        xiiRenderConfiguration migrated;
        if (legacy.Convert(migrated).Failed())
        {
          xiiLog::Error("Cannot migrate invalid legacy render configuration. Correct its rendering values before loading.");
          continue;
        }
        xiiAbstractObjectGraph  converted;
        xiiRttiConverterContext context;
        xiiRttiConverterWriter  writer(&converted, &context, true, true);
        context.RegisterObject(xiiUuid::MakeUuid(), xiiGetStaticRTTI<xiiRenderConfiguration>(), &migrated);
        const auto*     root       = writer.AddObjectToGraph(xiiGetStaticRTTI<xiiRenderConfiguration>(), &migrated, "root");
        xiiVariantArray subsystems = root->FindProperty("Subsystems")->m_Value.Get<xiiVariantArray>();
        if (const auto* existing = target->FindProperty("Subsystems"); existing && existing->m_Value.IsA<xiiVariantArray>())
          subsystems.PushBackRange(existing->m_Value.Get<xiiVariantArray>());
        for (const auto& entry : converted.GetAllNodes())
          if (entry.Value() != root)
            graph->CopyNodeIntoGraph(entry.Value());
        if (target->FindProperty("Subsystems"))
          target->ChangeProperty("Subsystems", subsystems);
        else
          target->AddProperty("Subsystems", subsystems);
        for (const auto name : {"BloomStrength", "RenderScale", "GlobalIllumination", "Shadows", "ScreenSpaceReflections", "DDGI"})
          target->RemoveProperty(name);
        if (oldDDGI.IsValid())
          graph->RemoveNode(oldDDGI);
        target->SetTypeVersion(6);
      }
    }
  };
  RenderConfigurationPatch s_RenderConfigurationGraphPatch(xiiGraphPatch::PatchType::GraphPatch);
  RenderConfigurationPatch s_RenderConfigurationNodePatch(xiiGraphPatch::PatchType::NodePatch);
} // namespace

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderConfiguration);
