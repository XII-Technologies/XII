/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderSettingsBindingAttribute, 1, xiiRTTIDefaultAllocator<xiiRenderSettingsBindingAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY_READ_ONLY("Pass", GetPass),
    XII_ACCESSOR_PROPERTY_READ_ONLY("Property", GetProperty),
    XII_ACCESSOR_PROPERTY_READ_ONLY("Changes", GetChanges),
  } XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiUInt32),
  } XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewRenderSettingsAttribute, 1, xiiRTTIDefaultAllocator<xiiViewRenderSettingsAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderSubsystemSettings, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Overrides", m_Overrides)->AddAttributes(new xiiHiddenAttribute()),
  } XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRenderSettingsBindingAttribute::xiiRenderSettingsBindingAttribute(xiiStringView pass, xiiStringView property, xiiUInt32 changes) :
  m_sPass(pass), m_sProperty(property), m_uiChanges(changes)
{
}

namespace
{
  void GatherProperties(const xiiRTTI* type, xiiStringView prefix, xiiDynamicArray<xiiRenderSettingProperty>& properties, xiiUInt32 depth)
  {
    if (depth >= 16)
      return;
    xiiHybridArray<const xiiAbstractProperty*, 32> members;
    type->GetAllProperties(members);
    for (const auto* property : members)
    {
      if (property->GetCategory() != xiiPropertyCategory::Member || property->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) || property->GetAttributeByType<xiiHiddenAttribute>())
        continue;
      xiiStringBuilder path(prefix);
      if (!path.IsEmpty())
        path.Append("/");
      path.Append(property->GetPropertyName());
      if (xiiReflectionUtils::IsValueType(property) || property->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        properties.PushBack({path, property});
      else if (property->GetFlags().IsSet(xiiPropertyFlags::Class) && !property->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        GatherProperties(property->GetSpecificType(), path, properties, depth + 1);
    }
  }
} // namespace

void xiiRenderSubsystemSettings::GetProperties(const xiiRTTI* type, xiiDynamicArray<xiiRenderSettingProperty>& out_properties)
{
  out_properties.Clear();
  GatherProperties(type, {}, out_properties, 0);
}

void xiiRenderSubsystemSettings::GetProperties(xiiDynamicArray<xiiRenderSettingProperty>& out_properties) const
{
  GetProperties(GetDynamicRTTI(), out_properties);
}

bool xiiRenderSubsystemSettings::IsOverridden(xiiStringView path) const
{
  for (const auto& property : m_Overrides)
    if (property == path)
      return true;
  return false;
}

xiiResult xiiRenderSubsystemSettings::ReadProperty(const xiiRenderSubsystemSettings& settings, xiiStringView path, xiiVariant& out_value)
{
  xiiPropertyPath resolved;
  XII_SUCCEED_OR_RETURN(resolved.InitializeFromPath(*settings.GetDynamicRTTI(), path));
  resolved.GetValue(const_cast<xiiRenderSubsystemSettings*>(&settings), *settings.GetDynamicRTTI(), out_value);
  return out_value.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiRenderSubsystemSettings::Validate() const
{
  XII_SUCCEED_OR_RETURN(SUPER::Validate());
  xiiDynamicArray<xiiRenderSettingProperty> properties;
  GetProperties(properties);
  for (const auto& property : properties)
  {
    xiiVariant value;
    XII_SUCCEED_OR_RETURN(ReadProperty(*this, property.m_sPath, value));
    if ((value.IsA<float>() && !xiiMath::IsFinite(value.Get<float>())) || (value.IsA<double>() && !xiiMath::IsFinite(value.Get<double>())))
      return XII_FAILURE;
    if (const auto* clamp = property.m_pProperty->GetAttributeByType<xiiClampValueAttribute>())
    {
      auto clamped = value;
      if (xiiReflectionUtils::ClampValue(clamped, clamp).Failed() || clamped != value)
        return XII_FAILURE;
    }
    if (property.m_pProperty->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
    {
      const auto enumeration = value.ConvertTo<xiiInt64>();
      if (xiiReflectionUtils::MakeEnumerationValid(property.m_pProperty->GetSpecificType(), enumeration) != enumeration)
        return XII_FAILURE;
    }
  }
  for (xiiUInt32 i = 0; i < m_Overrides.GetCount(); ++i)
  {
    bool found = false;
    for (const auto& property : properties)
      found |= property.m_sPath == m_Overrides[i];
    if (!found || m_Overrides.IndexOf(m_Overrides[i]) != i)
      return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiRenderSubsystemSettings::SetOverride(xiiStringView path, const xiiVariant& value)
{
  xiiDynamicArray<xiiRenderSettingProperty> properties;
  GetProperties(properties);
  bool found = false;
  for (const auto& property : properties)
    found |= property.m_sPath == path;
  if (!found)
    return XII_FAILURE;
  auto* candidate = static_cast<xiiRenderSubsystemSettings*>(xiiReflectionSerializer::Clone(this, GetDynamicRTTI()));
  XII_SCOPE_EXIT(candidate->GetDynamicRTTI()->GetAllocator()->Deallocate(candidate));
  xiiPropertyPath resolved;
  XII_SUCCEED_OR_RETURN(resolved.InitializeFromPath(*GetDynamicRTTI(), path));
  xiiVariant current;
  resolved.GetValue(candidate, *GetDynamicRTTI(), current);
  xiiResult  conversion = XII_FAILURE;
  const auto converted  = value.ConvertTo(current.GetType(), &conversion);
  XII_SUCCEED_OR_RETURN(conversion);
  resolved.SetValue(candidate, *GetDynamicRTTI(), converted);
  if (!candidate->IsOverridden(path))
    candidate->m_Overrides.PushBack(xiiString(path));
  candidate->m_Overrides.Sort();
  XII_SUCCEED_OR_RETURN(candidate->Validate());
  return Apply(*candidate);
}

void xiiRenderSubsystemSettings::RemoveOverride(xiiStringView path)
{
  for (xiiUInt32 i = 0; i < m_Overrides.GetCount(); ++i)
    if (m_Overrides[i] == path)
    {
      m_Overrides.RemoveAtAndCopy(i);
      NotifyChanged();
      return;
    }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderSettings);
