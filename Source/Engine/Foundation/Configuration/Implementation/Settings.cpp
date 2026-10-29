/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Settings.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSettingsScopeAttribute, 1, xiiRTTIDefaultAllocator<xiiSettingsScopeAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Scope", m_iScope),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSettingsRestartAttribute, 1, xiiRTTIDefaultAllocator<xiiSettingsRestartAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSettings, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  // The general RTTI converter assumes valid object references. Check settings files before invoking it,
  // so a truncated graph or an unavailable nested plugin type cannot reach an assertion or allocator.
  xiiResult ValidateSettingsNode(const xiiAbstractObjectGraph& graph, const xiiAbstractObjectNode& node, const xiiRTTI* pType, xiiSet<xiiUuid>& ref_visited, xiiUInt32 uiDepth);

  xiiResult ValidateSettingsValue(const xiiAbstractObjectGraph& graph, const xiiVariant& value, const xiiAbstractProperty* pProperty, xiiSet<xiiUuid>& ref_visited, xiiUInt32 uiDepth)
  {
    const auto* pType = pProperty->GetSpecificType();
    if (pProperty->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
    {
      xiiInt64 iValue = 0;
      if (value.IsA<xiiString>())
      {
        if (!xiiReflectionUtils::StringToEnumeration(pType, value.Get<xiiString>(), iValue))
          return XII_FAILURE;
      }
      else
      {
        xiiResult result = XII_FAILURE;
        iValue           = value.ConvertTo<xiiInt64>(&result);
        XII_SUCCEED_OR_RETURN(result);
      }
      return xiiReflectionUtils::MakeEnumerationValid(pType, iValue) == iValue ? XII_SUCCESS : XII_FAILURE;
    }
    if (xiiReflectionUtils::IsValueType(pProperty))
    {
      if (pType == xiiGetStaticRTTI<xiiVariant>())
        return XII_SUCCESS;
      xiiResult  result    = XII_FAILURE;
      xiiVariant converted = value.ConvertTo(static_cast<xiiVariantType::Enum>(pType->GetVariantType()), &result);
      XII_SUCCEED_OR_RETURN(result);
      if ((converted.IsA<float>() && !xiiMath::IsFinite(converted.Get<float>())) || (converted.IsA<double>() && !xiiMath::IsFinite(converted.Get<double>())))
        return XII_FAILURE;
      if (const auto* pClamp = pProperty->GetAttributeByType<xiiClampValueAttribute>())
      {
        xiiVariant clamped = converted;
        if (xiiReflectionUtils::ClampValue(clamped, pClamp).Failed() || clamped != converted)
          return XII_FAILURE;
      }
      return XII_SUCCESS;
    }
    if (!value.IsA<xiiUuid>())
      return XII_FAILURE;
    const auto guid = value.Get<xiiUuid>();
    if (!guid.IsValid())
      return pProperty->GetFlags().IsSet(xiiPropertyFlags::Pointer) ? XII_SUCCESS : XII_FAILURE;
    // Non-owning references are resolved by the converter context, never allocated here.
    if (pProperty->GetFlags().IsSet(xiiPropertyFlags::Pointer) && !pProperty->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
      return XII_SUCCESS;
    const auto* pNode = graph.GetNode(guid);
    if (pNode == nullptr)
      return XII_FAILURE;
    const auto* pActualType = xiiRTTI::FindTypeByName(pNode->GetType());
    if (pActualType == nullptr || !pActualType->IsDerivedFrom(pType) ||
        (pProperty->GetFlags().IsSet(xiiPropertyFlags::PointerOwner) && !pActualType->GetAllocator()->CanAllocate()))
      return XII_FAILURE;
    return ValidateSettingsNode(graph, *pNode, pActualType, ref_visited, uiDepth + 1);
  }

  xiiResult ValidateSettingsNode(const xiiAbstractObjectGraph& graph, const xiiAbstractObjectNode& node, const xiiRTTI* pType, xiiSet<xiiUuid>& ref_visited, xiiUInt32 uiDepth)
  {
    if (ref_visited.Contains(node.GetGuid()) || uiDepth >= 64)
      return XII_FAILURE;
    ref_visited.Insert(node.GetGuid());
    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (const auto* pProperty : properties)
    {
      if (pProperty->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) || pProperty->GetCategory() == xiiPropertyCategory::Constant)
        continue;
      const auto* pStored = node.FindProperty(pProperty->GetPropertyName());
      if (pStored == nullptr)
        continue;
      switch (pProperty->GetCategory())
      {
        case xiiPropertyCategory::Member:
          XII_SUCCEED_OR_RETURN(ValidateSettingsValue(graph, pStored->m_Value, pProperty, ref_visited, uiDepth));
          break;
        case xiiPropertyCategory::Array:
        case xiiPropertyCategory::Set:
          if (!pStored->m_Value.IsA<xiiVariantArray>())
            return XII_FAILURE;
          for (const auto& value : pStored->m_Value.Get<xiiVariantArray>())
            XII_SUCCEED_OR_RETURN(ValidateSettingsValue(graph, value, pProperty, ref_visited, uiDepth));
          break;
        case xiiPropertyCategory::Map:
          if (!pStored->m_Value.IsA<xiiVariantDictionary>())
            return XII_FAILURE;
          for (const auto& entry : pStored->m_Value.Get<xiiVariantDictionary>())
            XII_SUCCEED_OR_RETURN(ValidateSettingsValue(graph, entry.Value(), pProperty, ref_visited, uiDepth));
          break;
        default:
          break;
      }
    }
    return XII_SUCCESS;
  }
} // namespace

xiiSettings* xiiSettings::Create(const xiiRTTI* pType)
{
  if (pType == nullptr || !pType->IsDerivedFrom<xiiSettings>() || pType->GetTypeFlags().IsSet(xiiTypeFlags::Abstract) || !pType->GetAllocator()->CanAllocate())
    return nullptr;

  xiiSettings*                                   pSettings = pType->GetAllocator()->Allocate<xiiSettings>();
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);
  for (const auto* pProperty : properties)
  {
    if (pProperty->GetCategory() == xiiPropertyCategory::Member && !pProperty->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) && pProperty->GetAttributeByType<xiiDefaultValueAttribute>() != nullptr)
      xiiReflectionUtils::SetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProperty), pSettings, xiiReflectionUtils::GetDefaultValue(pProperty));
  }
  return pSettings;
}

void xiiSettings::GatherTypes(xiiSettingsScope::Enum scope, xiiDynamicArray<const xiiRTTI*>& out_types)
{
  out_types.Clear();
  xiiRTTI::ForEachDerivedType<xiiSettings>([&](const xiiRTTI* pType) {
    const auto* pScope = pType->GetAttributeByType<xiiSettingsScopeAttribute>();
    if (pScope != nullptr && pScope->GetScope() == scope)
      out_types.PushBack(pType);
  },
                                           xiiRTTI::ForEachOptions::ExcludeNotConcrete);
  out_types.Sort([](const xiiRTTI* pLeft, const xiiRTTI* pRight) { return pLeft->GetTypeName().Compare(pRight->GetTypeName()) < 0; });
}

xiiResult xiiSettings::Validate() const
{
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  GetDynamicRTTI()->GetAllProperties(properties);
  for (const auto* pProperty : properties)
  {
    if (pProperty->GetCategory() != xiiPropertyCategory::Member || pProperty->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) ||
        (!xiiReflectionUtils::IsValueType(pProperty) && !pProperty->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags)))
      continue;

    xiiVariant value = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProperty), this);
    if ((value.IsA<float>() && !xiiMath::IsFinite(value.Get<float>())) || (value.IsA<double>() && !xiiMath::IsFinite(value.Get<double>())))
      return XII_FAILURE;

    if (const auto* pClamp = pProperty->GetAttributeByType<xiiClampValueAttribute>())
    {
      xiiVariant clamped = value;
      if (xiiReflectionUtils::ClampValue(clamped, pClamp).Failed() || clamped != value)
        return XII_FAILURE;
    }

    if (pProperty->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
    {
      const xiiInt64 iValue = value.ConvertTo<xiiInt64>();
      if (xiiReflectionUtils::MakeEnumerationValid(pProperty->GetSpecificType(), iValue) != iValue)
        return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiSettings::Apply(const xiiSettings& source)
{
  if (source.GetDynamicRTTI() != GetDynamicRTTI())
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(source.Validate());
  if (!HasSameValues(source))
  {
    xiiReflectionSerializer::Clone(&source, this, GetDynamicRTTI());
    NotifyChanged();
  }
  return XII_SUCCESS;
}

bool xiiSettings::HasSameValues(const xiiSettings& other) const
{
  if (GetDynamicRTTI() != other.GetDynamicRTTI())
    return false;
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  GetDynamicRTTI()->GetAllProperties(properties);
  for (const auto* pProperty : properties)
  {
    if (!pProperty->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) && !xiiReflectionUtils::IsEqual(this, &other, pProperty))
      return false;
  }
  return true;
}

xiiResult xiiSettings::ReadFromDDL(xiiStreamReader& ref_stream)
{
  xiiAbstractObjectGraph graph;
  XII_SUCCEED_OR_RETURN(xiiAbstractGraphDdlSerializer::Read(ref_stream, &graph));
  return ReadFromGraph(graph);
}

xiiResult xiiSettings::ReadFromGraph(const xiiAbstractObjectGraph& graph)
{
  if (!GetDynamicRTTI()->GetAllocator()->CanAllocate())
    return XII_FAILURE;
  const auto* pRoot = graph.GetNodeByName("root");
  if (pRoot == nullptr || pRoot->GetType() != GetDynamicRTTI()->GetTypeName())
    return XII_FAILURE;

  xiiSet<xiiUuid> visited;
  XII_SUCCEED_OR_RETURN(ValidateSettingsNode(graph, *pRoot, GetDynamicRTTI(), visited, 0));

  auto* pCandidate = xiiReflectionSerializer::Clone(this);
  XII_SCOPE_EXIT(pCandidate->GetDynamicRTTI()->GetAllocator()->Deallocate(pCandidate));
  xiiRttiConverterContext context;
  context.RegisterObject(pRoot->GetGuid(), GetDynamicRTTI(), pCandidate);
  xiiRttiConverterReader reader(&graph, &context);
  reader.ApplyPropertiesToObject(pRoot, GetDynamicRTTI(), pCandidate);
  XII_SUCCEED_OR_RETURN(pCandidate->Validate());
  xiiReflectionSerializer::Clone(pCandidate, this, GetDynamicRTTI());
  return XII_SUCCESS;
}

void xiiSettings::NotifyChanged()
{
  OnChanged();
  m_SettingsChangedEvent.Broadcast(this);
}

XII_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Settings);
