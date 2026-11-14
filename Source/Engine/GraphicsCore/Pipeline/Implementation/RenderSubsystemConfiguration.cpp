/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsCore/Pipeline/RenderConfiguration.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

xiiRenderConfiguration::xiiRenderConfiguration(const xiiRenderConfiguration& other)
{
  *this = other;
}

xiiRenderConfiguration::xiiRenderConfiguration(xiiRenderConfiguration&& other) noexcept
{
  *this = std::move(other);
}

xiiRenderConfiguration::~xiiRenderConfiguration()
{
  ClearSubsystems();
}

void xiiRenderConfiguration::ClearSubsystems()
{
  for (auto* subsystem : m_Subsystems)
    if (subsystem)
      subsystem->GetDynamicRTTI()->GetAllocator()->Deallocate(subsystem);
  m_Subsystems.Clear();
  m_Sources.Clear();
}

xiiRenderConfiguration& xiiRenderConfiguration::operator=(const xiiRenderConfiguration& other)
{
  if (this == &other)
    return *this;
  ClearSubsystems();
  for (const auto* subsystem : other.m_Subsystems)
    m_Subsystems.PushBack(subsystem ? static_cast<xiiRenderSubsystemSettings*>(xiiReflectionSerializer::Clone(subsystem, subsystem->GetDynamicRTTI())) : nullptr);
  m_Sources = other.m_Sources;
  return *this;
}

xiiRenderConfiguration& xiiRenderConfiguration::operator=(xiiRenderConfiguration&& other) noexcept
{
  if (this == &other)
    return *this;
  ClearSubsystems();
  m_Subsystems.Swap(other.m_Subsystems);
  m_Sources.Swap(other.m_Sources);
  return *this;
}

bool xiiRenderConfiguration::operator==(const xiiRenderConfiguration& other) const
{
  if (m_Subsystems.GetCount() != other.m_Subsystems.GetCount())
    return false;
  for (const auto* subsystem : m_Subsystems)
  {
    if (!subsystem)
      return false;
    const auto* counterpart = other.FindSubsystem(subsystem->GetDynamicRTTI());
    if (!counterpart || !subsystem->HasSameValues(*counterpart))
      return false;
  }
  return true;
}

const xiiRenderSubsystemSettings* xiiRenderConfiguration::FindSubsystem(const xiiRTTI* type) const
{
  for (const auto* subsystem : m_Subsystems)
    if (subsystem && subsystem->GetDynamicRTTI() == type)
      return subsystem;
  return nullptr;
}

xiiRenderSubsystemSettings* xiiRenderConfiguration::EditSubsystem(const xiiRTTI* type)
{
  if (const auto* subsystem = FindSubsystem(type))
    return const_cast<xiiRenderSubsystemSettings*>(subsystem);
  if (!type || !type->IsDerivedFrom<xiiRenderSubsystemSettings>() || !type->GetAllocator()->CanAllocate())
    return nullptr;
  auto* subsystem = static_cast<xiiRenderSubsystemSettings*>(xiiSettings::Create(type));
  if (subsystem)
    m_Subsystems.PushBack(subsystem);
  return subsystem;
}

xiiResult xiiRenderConfiguration::SetOverride(const xiiRTTI* type, xiiStringView path, const xiiVariant& value)
{
  xiiRenderConfiguration candidate(*this);
  auto*                  subsystem = candidate.EditSubsystem(type);
  if (!subsystem)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(subsystem->SetOverride(path, value));
  if (!candidate.IsValid())
    return XII_FAILURE;
  candidate.m_Sources.Clear();
  *this = std::move(candidate);
  return XII_SUCCESS;
}

void xiiRenderConfiguration::RemoveOverride(const xiiRTTI* type, xiiStringView path)
{
  if (auto* subsystem = const_cast<xiiRenderSubsystemSettings*>(FindSubsystem(type)))
    subsystem->RemoveOverride(path);
}

xiiRenderSettingScope::Enum xiiRenderConfiguration::GetSource(const xiiRTTI* type, xiiStringView path) const
{
  for (const auto& source : m_Sources)
    if (source.m_sType == type->GetTypeName() && source.m_sPath == path)
      return static_cast<xiiRenderSettingScope::Enum>(source.m_uiScope);
  return xiiRenderSettingScope::SubsystemDefault;
}

void xiiRenderConfiguration::ApplySubsystemOverrides(const xiiRenderConfiguration& overrides, xiiRenderSettingScope::Enum scope)
{
  for (const auto* source : overrides.m_Subsystems)
  {
    if (!source)
      continue;
    auto* target = EditSubsystem(source->GetDynamicRTTI());
    if (!target)
      continue;
    for (const auto& path : source->m_Overrides)
    {
      xiiVariant      value;
      xiiPropertyPath resolved;
      if (xiiRenderSubsystemSettings::ReadProperty(*source, path, value).Failed() || resolved.InitializeFromPath(*target->GetDynamicRTTI(), path).Failed())
        continue;
      resolved.SetValue(target, *target->GetDynamicRTTI(), value);
      if (!target->IsOverridden(path))
        target->m_Overrides.PushBack(path);
      bool found = false;
      for (auto& origin : m_Sources)
        if (origin.m_sType == source->GetDynamicRTTI()->GetTypeName() && origin.m_sPath == path)
        {
          origin.m_uiScope = scope;
          found            = true;
          break;
        }
      if (!found)
        m_Sources.PushBack({xiiString(source->GetDynamicRTTI()->GetTypeName()), path, static_cast<xiiUInt8>(scope)});
    }
    target->m_Overrides.Sort();
  }
}

void xiiRenderConfiguration::CopySourcesFrom(const xiiRenderConfiguration& snapshot)
{
  for (const auto& source : snapshot.m_Sources)
  {
    bool found = false;
    for (auto& target : m_Sources)
      if (target.m_sType == source.m_sType && target.m_sPath == source.m_sPath)
      {
        target.m_uiScope = source.m_uiScope;
        found            = true;
        break;
      }
    if (!found)
      m_Sources.PushBack(source);
  }
}

void xiiRenderConfiguration::ApplySnapshot(const xiiRenderConfiguration& snapshot)
{
  ApplyOverrides(snapshot);
  CopySourcesFrom(snapshot);
}

xiiBitflags<xiiRenderSettingsChange> xiiRenderConfiguration::GetChanges(const xiiRenderConfiguration& previous) const
{
  xiiBitflags<xiiRenderSettingsChange> changes;
  for (const auto* subsystem : m_Subsystems)
  {
    if (!subsystem)
      continue;
    const auto*                               before = previous.FindSubsystem(subsystem->GetDynamicRTTI());
    xiiDynamicArray<xiiRenderSettingProperty> properties;
    subsystem->GetProperties(properties);
    for (const auto& property : properties)
    {
      xiiVariant value, oldValue;
      if (xiiRenderSubsystemSettings::ReadProperty(*subsystem, property.m_sPath, value).Failed())
        continue;
      if (before && xiiRenderSubsystemSettings::ReadProperty(*before, property.m_sPath, oldValue).Succeeded() && value == oldValue && subsystem->IsOverridden(property.m_sPath) == before->IsOverridden(property.m_sPath))
        continue;
      for (const auto* attribute : property.m_pProperty->GetAttributes())
        if (attribute->GetDynamicRTTI()->IsDerivedFrom<xiiRenderSettingsBindingAttribute>())
          changes.Add(static_cast<xiiRenderSettingsChange::Enum>(static_cast<const xiiRenderSettingsBindingAttribute*>(attribute)->GetChanges()));
    }
  }
  return changes;
}


namespace
{
  bool MatchesPass(const xiiRenderSettingsBindingAttribute& binding, const xiiRenderPipelinePass& pass)
  {
    return xiiStringView(binding.GetPass()) == pass.GetDynamicRTTI()->GetTypeName();
  }
} // namespace

xiiResult xiiRenderConfiguration::InitializeSubsystemDefaults(xiiArrayPtr<xiiRenderPipelinePass* const> passes)
{
  ClearSubsystems();
  xiiResult status = XII_SUCCESS;
  xiiRTTI::ForEachDerivedType<xiiRenderSubsystemSettings>([&](const xiiRTTI* type) {
    if (status.Failed())
      return;
    xiiDynamicArray<xiiRenderSettingProperty> properties;
    xiiRenderSubsystemSettings::GetProperties(type, properties);
    bool supported = type->GetAttributeByType<xiiViewRenderSettingsAttribute>() != nullptr;
    for (const auto* attribute : type->GetAttributes())
      if (const auto* binding = xiiDynamicCast<const xiiRenderSettingsBindingAttribute*>(attribute))
        for (const auto* pass : passes)
          supported |= MatchesPass(*binding, *pass);
    for (const auto& property : properties)
      for (const auto* attribute : property.m_pProperty->GetAttributes())
        if (const auto* binding = xiiDynamicCast<const xiiRenderSettingsBindingAttribute*>(attribute))
          for (const auto* pass : passes)
            supported |= MatchesPass(*binding, *pass);
    if (!supported)
      return;
    auto* settings = EditSubsystem(type);
    if (!settings)
    {
      status = XII_FAILURE;
      return;
    }
    for (const auto& property : properties)
    {
      bool captured = false;
      for (const auto* attribute : property.m_pProperty->GetAttributes())
      {
        const auto* binding = xiiDynamicCast<const xiiRenderSettingsBindingAttribute*>(attribute);
        if (!binding || xiiStringUtils::IsNullOrEmpty(binding->GetProperty()))
          continue;
        for (const auto* pass : passes)
        {
          if (!MatchesPass(*binding, *pass))
            continue;
          xiiPropertyPath source, target;
          if (source.InitializeFromPath(*pass->GetDynamicRTTI(), binding->GetProperty()).Failed() || target.InitializeFromPath(*type, property.m_sPath).Failed())
          {
            status = XII_FAILURE;
            return;
          }
          xiiVariant value;
          source.GetValue(const_cast<xiiRenderPipelinePass*>(pass), *pass->GetDynamicRTTI(), value);
          if (!value.IsValid())
          {
            status = XII_FAILURE;
            return;
          }
          target.SetValue(settings, *type, value);
          captured = true;
          break;
        }
        if (captured)
          break;
      }
    }
    if (settings->Validate().Failed())
      status = XII_FAILURE;
  },
                                                          xiiRTTI::ForEachOptions::ExcludeNotConcrete);
  return status;
}

bool xiiRenderConfiguration::HasPassOverrides(const xiiRTTI* type) const
{
  for (const auto* settings : m_Subsystems)
  {
    if (settings->m_Overrides.IsEmpty())
      continue;
    xiiDynamicArray<xiiRenderSettingProperty> properties;
    settings->GetProperties(properties);
    for (const auto& property : properties)
    {
      if (!settings->IsOverridden(property.m_sPath))
        continue;
      for (const auto* attribute : property.m_pProperty->GetAttributes())
        if (const auto* binding = xiiDynamicCast<const xiiRenderSettingsBindingAttribute*>(attribute);
            binding && type->GetTypeName() == binding->GetPass() && !xiiStringUtils::IsNullOrEmpty(binding->GetProperty()))
          return true;
    }
  }
  return false;
}

xiiResult xiiRenderConfiguration::ApplyToPass(xiiRenderPipelinePass& pass, xiiBitflags<xiiRenderSettingsChange>& out_changes) const
{
  for (const auto* settings : m_Subsystems)
  {
    if (!settings)
      return XII_FAILURE;
    xiiDynamicArray<xiiRenderSettingProperty> properties;
    settings->GetProperties(properties);
    for (const auto& property : properties)
    {
      if (!settings->IsOverridden(property.m_sPath))
        continue;
      for (const auto* attribute : property.m_pProperty->GetAttributes())
      {
        const auto* binding = xiiDynamicCast<const xiiRenderSettingsBindingAttribute*>(attribute);
        if (!binding || !MatchesPass(*binding, pass) || xiiStringUtils::IsNullOrEmpty(binding->GetProperty()))
          continue;
        xiiPropertyPath target;
        xiiVariant      value, previous;
        XII_SUCCEED_OR_RETURN(target.InitializeFromPath(*pass.GetDynamicRTTI(), binding->GetProperty()));
        XII_SUCCEED_OR_RETURN(xiiRenderSubsystemSettings::ReadProperty(*settings, property.m_sPath, value));
        target.GetValue(&pass, *pass.GetDynamicRTTI(), previous);
        if (value != previous)
        {
          target.SetValue(&pass, *pass.GetDynamicRTTI(), value);
          out_changes.Add(static_cast<xiiRenderSettingsChange::Enum>(binding->GetChanges()));
        }
      }
    }
  }
  return XII_SUCCESS;
}


xiiResult xiiRenderConfiguration::CommitPassSettings(const xiiRenderPipelinePass& source, xiiRenderPipelinePass& target, xiiBitflags<xiiRenderSettingsChange>& out_changes) const
{
  for (const auto* settings : m_Subsystems)
  {
    xiiDynamicArray<xiiRenderSettingProperty> properties;
    settings->GetProperties(properties);
    for (const auto& property : properties)
      for (const auto* attribute : property.m_pProperty->GetAttributes())
      {
        const auto* binding = xiiDynamicCast<const xiiRenderSettingsBindingAttribute*>(attribute);
        if (!binding || !MatchesPass(*binding, target) || xiiStringUtils::IsNullOrEmpty(binding->GetProperty()))
          continue;
        xiiPropertyPath path;
        XII_SUCCEED_OR_RETURN(path.InitializeFromPath(*target.GetDynamicRTTI(), binding->GetProperty()));
        xiiVariant value, previous;
        path.GetValue(const_cast<xiiRenderPipelinePass*>(&source), *source.GetDynamicRTTI(), value);
        path.GetValue(&target, *target.GetDynamicRTTI(), previous);
        if (!value.IsValid() || !previous.IsValid())
          return XII_FAILURE;
        if (value != previous)
        {
          path.SetValue(&target, *target.GetDynamicRTTI(), value);
          out_changes.Add(static_cast<xiiRenderSettingsChange::Enum>(binding->GetChanges()));
        }
      }
  }
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderSubsystemConfiguration);
