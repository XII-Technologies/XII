/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Network/Reflection/NetworkAttributes.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Types/ScopeExit.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

namespace
{
  constexpr xiiUInt32 MaximumNetworkAttributes = 64;

  bool IsNetworkAttributeType(const xiiRTTI* pType)
  {
    const xiiRTTI* const types[] = {
      xiiGetStaticRTTI<xiiReadOnlyAttribute>(), xiiGetStaticRTTI<xiiHiddenAttribute>(),
      xiiGetStaticRTTI<xiiCategoryAttribute>(), xiiGetStaticRTTI<xiiTitleAttribute>(),
      xiiGetStaticRTTI<xiiColorAttribute>(), xiiGetStaticRTTI<xiiExposeColorAlphaAttribute>(),
      xiiGetStaticRTTI<xiiSuffixAttribute>(), xiiGetStaticRTTI<xiiMinValueTextAttribute>(),
      xiiGetStaticRTTI<xiiDefaultValueAttribute>(), xiiGetStaticRTTI<xiiClampValueAttribute>(),
      xiiGetStaticRTTI<xiiGroupAttribute>(), xiiGetStaticRTTI<xiiInDevelopmentAttribute>(),
      xiiGetStaticRTTI<xiiTagSetWidgetAttribute>(), xiiGetStaticRTTI<xiiNoTemporaryTransactionsAttribute>(),
      xiiGetStaticRTTI<xiiExposedParametersAttribute>(), xiiGetStaticRTTI<xiiDynamicDefaultValueAttribute>(),
      xiiGetStaticRTTI<xiiContainerAttribute>(), xiiGetStaticRTTI<xiiFileBrowserAttribute>(),
      xiiGetStaticRTTI<xiiExternalFileBrowserAttribute>(), xiiGetStaticRTTI<xiiAssetBrowserAttribute>(),
      xiiGetStaticRTTI<xiiDynamicEnumAttribute>(), xiiGetStaticRTTI<xiiDynamicStringEnumAttribute>(),
      xiiGetStaticRTTI<xiiDynamicBitflagsAttribute>(), xiiGetStaticRTTI<xiiSphereManipulatorAttribute>(),
      xiiGetStaticRTTI<xiiCapsuleManipulatorAttribute>(), xiiGetStaticRTTI<xiiBoxManipulatorAttribute>(),
      xiiGetStaticRTTI<xiiNonUniformBoxManipulatorAttribute>(), xiiGetStaticRTTI<xiiConeLengthManipulatorAttribute>(),
      xiiGetStaticRTTI<xiiConeAngleManipulatorAttribute>(), xiiGetStaticRTTI<xiiTransformManipulatorAttribute>(),
      xiiGetStaticRTTI<xiiBoneManipulatorAttribute>(), xiiGetStaticRTTI<xiiBoxVisualizerAttribute>(),
      xiiGetStaticRTTI<xiiSphereVisualizerAttribute>(), xiiGetStaticRTTI<xiiCapsuleVisualizerAttribute>(),
      xiiGetStaticRTTI<xiiCylinderVisualizerAttribute>(), xiiGetStaticRTTI<xiiDirectionVisualizerAttribute>(),
      xiiGetStaticRTTI<xiiConeVisualizerAttribute>()};
    return xiiMakeArrayPtr(types).Contains(pType);
  }

  bool IsNetworkAttributeProperty(const xiiAbstractProperty* pProperty)
  {
    return pProperty->GetCategory() == xiiPropertyCategory::Member &&
      !pProperty->GetFlags().IsAnySet(xiiPropertyFlags::Pointer | xiiPropertyFlags::ReadOnly) &&
      (pProperty->GetFlags().IsAnySet(xiiPropertyFlags::StandardType | xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags) ||
       pProperty->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>());
  }

  bool IsNetworkAttributeValue(const xiiVariant& value, xiiUInt32 uiDepth = 0)
  {
    if (uiDepth > 4)
      return false;
    switch (value.GetType())
    {
      case xiiVariantType::TypedObject:
      case xiiVariantType::TypedPointer:
        return false;
      case xiiVariantType::String:
        return value.Get<xiiString>().GetElementCount() <= 4096;
      case xiiVariantType::StringView:
        return value.Get<xiiStringView>().GetElementCount() <= 4096;
      case xiiVariantType::VariantArray:
        if (value.Get<xiiVariantArray>().GetCount() > MaximumNetworkAttributes)
          return false;
        for (const auto& child : value.Get<xiiVariantArray>())
          if (!IsNetworkAttributeValue(child, uiDepth + 1))
            return false;
        return true;
      case xiiVariantType::VariantDictionary:
        if (value.Get<xiiVariantDictionary>().GetCount() > MaximumNetworkAttributes)
          return false;
        for (const auto& child : value.Get<xiiVariantDictionary>())
          if (!IsNetworkAttributeValue(child.Value(), uiDepth + 1))
            return false;
        return true;
      default:
        return true;
    }
  }

  xiiNetworkAttribute* NetworkAttributeListField()
  {
    auto* attribute                   = new xiiNetworkAttribute(1000, true);
    attribute->m_uiMaximumElements    = MaximumNetworkAttributes;
    attribute->m_uiMaximumStringBytes = 4096;
    return attribute;
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAttributeHolder, xiiNoBase, 1, xiiRTTINoAllocator)
{
  flags.Add(xiiTypeFlags::Abstract);
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_ACCESSOR_PROPERTY("Attributes", GetCount, GetValue, SetValue, Insert, Remove)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_ACCESSOR_PROPERTY("AuthoringMetadata", GetNetworkAttributes, SetNetworkAttributes)->AddAttributes(new xiiTemporaryAttribute(), new xiiHiddenAttribute(), NetworkAttributeListField()),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiAttributeHolder::xiiAttributeHolder() = default;

xiiVariantArray xiiAttributeHolder::GetNetworkAttributes() const
{
  xiiVariantArray result;
  for (xiiUInt32 i = 0; i < GetCount() && result.GetCount() < MaximumNetworkAttributes; ++i)
  {
    const auto* attribute = GetValue(i);
    if (attribute == nullptr || !IsNetworkAttributeType(attribute->GetDynamicRTTI()))
      continue;
    const auto*          type = attribute->GetDynamicRTTI();
    xiiVariantDictionary fields;
    fields.Insert("type", xiiString(type->GetTypeName()));
    xiiHybridArray<const xiiAbstractProperty*, 16> properties;
    type->GetAllProperties(properties);
    for (const auto* property : properties)
    {
      if (!IsNetworkAttributeProperty(property))
        continue;
      const auto value = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(property), attribute);
      if (IsNetworkAttributeValue(value))
        fields.Insert(property->GetPropertyName(), value);
    }
    result.PushBack(std::move(fields));
  }
  return result;
}

void xiiAttributeHolder::SetNetworkAttributes(xiiVariantArray attributes)
{
  for (const auto* attribute : m_Attributes)
    if (attribute)
      attribute->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<xiiPropertyAttribute*>(attribute));
  m_Attributes.Clear();
  m_ReferenceAttributes.Clear();
  for (const auto& entry : attributes.GetArrayPtr().GetSubArray(0, xiiMath::Min(attributes.GetCount(), MaximumNetworkAttributes)))
  {
    if (!entry.IsA<xiiVariantDictionary>())
      continue;
    const auto& fields = entry.Get<xiiVariantDictionary>();
    const auto* name   = fields.GetValue("type");
    if (name == nullptr || !name->IsA<xiiString>())
      continue;
    const auto* type = xiiRTTI::FindTypeByName(name->Get<xiiString>());
    if (!IsNetworkAttributeType(type) || !type->GetAllocator()->CanAllocate())
      continue;
    xiiPropertyAttribute* attribute = type->GetAllocator()->Allocate<xiiPropertyAttribute>();
    for (const auto& field : fields)
    {
      const auto* property = type->FindPropertyByName(field.Key());
      if (property == nullptr || !IsNetworkAttributeProperty(property) || !IsNetworkAttributeValue(field.Value()))
        continue;
      const auto* member   = static_cast<const xiiAbstractMemberProperty*>(property);
      const auto  original = xiiReflectionUtils::GetMemberPropertyValue(member, attribute);
      if (property->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>() || original.GetType() == field.Value().GetType() ||
          (original.IsA<xiiStringView>() && field.Value().IsA<xiiString>()))
        xiiReflectionUtils::SetMemberPropertyValue(member, attribute, field.Value());
    }
    m_Attributes.PushBack(attribute);
  }
}

xiiAttributeHolder::xiiAttributeHolder(const xiiAttributeHolder& rhs)
{
  m_Attributes = rhs.m_Attributes;
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

xiiAttributeHolder::~xiiAttributeHolder()
{
  for (const xiiPropertyAttribute* pAttr : m_Attributes)
  {
    if (pAttr)
    {
      pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<xiiPropertyAttribute*>(pAttr));
    }
  }
}

void xiiAttributeHolder::operator=(const xiiAttributeHolder& rhs)
{
  if (this == &rhs)
    return;

  m_Attributes = rhs.m_Attributes;
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

xiiUInt32 xiiAttributeHolder::GetCount() const
{
  return xiiMath::Max(m_ReferenceAttributes.GetCount(), m_Attributes.GetCount());
}

const xiiPropertyAttribute* xiiAttributeHolder::GetValue(xiiUInt32 uiIndex) const
{
  if (!m_ReferenceAttributes.IsEmpty())
    return m_ReferenceAttributes[uiIndex];

  return m_Attributes[uiIndex];
}

void xiiAttributeHolder::SetValue(xiiUInt32 uiIndex, const xiiPropertyAttribute* value)
{
  m_Attributes[uiIndex] = value;
}

void xiiAttributeHolder::Insert(xiiUInt32 uiIndex, const xiiPropertyAttribute* value)
{
  m_Attributes.InsertAt(uiIndex, value);
}

void xiiAttributeHolder::Remove(xiiUInt32 uiIndex)
{
  m_Attributes.RemoveAtAndCopy(uiIndex);
}

////////////////////////////////////////////////////////////////////////
// xiiReflectedPropertyDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiReflectedPropertyDescriptor, xiiAttributeHolder, 2, xiiRTTIDefaultAllocator<xiiReflectedPropertyDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Category", xiiPropertyCategory, m_Category)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Type", m_sType)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiPropertyFlags, m_Flags)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("ConstantValue", m_ConstantValue)->AddAttributes(new xiiNetworkAttribute(5)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

class xiiReflectedPropertyDescriptorPatch_1_2 : public xiiGraphPatch
{
public:
  xiiReflectedPropertyDescriptorPatch_1_2() :
    xiiGraphPatch("xiiReflectedPropertyDescriptor", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    if (xiiAbstractObjectNode::Property* pProp = pNode->FindProperty("Flags"))
    {
      xiiStringBuilder                  sValue = pProp->m_Value.Get<xiiString>();
      xiiHybridArray<xiiStringView, 32> values;
      sValue.Split(false, values, "|");

      xiiStringBuilder sNewValue;
      for (xiiInt32 i = (xiiInt32)values.GetCount() - 1; i >= 0; i--)
      {
        if (values[i].IsEqual("xiiPropertyFlags::Constant"))
        {
          values.RemoveAtAndCopy(i);
        }
        else if (values[i].IsEqual("xiiPropertyFlags::EmbeddedClass"))
        {
          values[i] = xiiStringView("xiiPropertyFlags::Class");
        }
        else if (values[i].IsEqual("xiiPropertyFlags::Pointer"))
        {
          values.PushBack(xiiStringView("xiiPropertyFlags::Class"));
        }
      }
      for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
      {
        if (i != 0)
          sNewValue.Append("|");
        sNewValue.Append(values[i]);
      }
      pProp->m_Value = sNewValue.GetData();
    }
  }
};

xiiReflectedPropertyDescriptorPatch_1_2 g_xiiReflectedPropertyDescriptorPatch_1_2;


xiiReflectedPropertyDescriptor::xiiReflectedPropertyDescriptor(xiiPropertyCategory::Enum category, xiiStringView sName, xiiStringView sType, xiiBitflags<xiiPropertyFlags> flags) :
  m_Category(category), m_sName(sName), m_sType(sType), m_Flags(flags)
{
}

xiiReflectedPropertyDescriptor::xiiReflectedPropertyDescriptor(xiiPropertyCategory::Enum category, xiiStringView sName, xiiStringView sType, xiiBitflags<xiiPropertyFlags> flags, xiiArrayPtr<const xiiPropertyAttribute* const> attributes) :
  m_Category(category), m_sName(sName), m_sType(sType), m_Flags(flags)
{
  m_ReferenceAttributes = attributes;
}

xiiReflectedPropertyDescriptor::xiiReflectedPropertyDescriptor(xiiStringView sName, const xiiVariant& constantValue, xiiArrayPtr<const xiiPropertyAttribute* const> attributes) :
  m_Category(xiiPropertyCategory::Constant), m_sName(sName), m_sType(), m_Flags(xiiPropertyFlags::StandardType | xiiPropertyFlags::ReadOnly), m_ConstantValue(constantValue)
{
  m_ReferenceAttributes = attributes;
  const xiiRTTI* pType  = xiiReflectionUtils::GetTypeFromVariant(constantValue);
  if (pType)
    m_sType = pType->GetTypeName();
}

xiiReflectedPropertyDescriptor::xiiReflectedPropertyDescriptor(const xiiReflectedPropertyDescriptor& rhs)
{
  operator=(rhs);
}

void xiiReflectedPropertyDescriptor::operator=(const xiiReflectedPropertyDescriptor& rhs)
{
  m_Category = rhs.m_Category;
  m_sName    = rhs.m_sName;

  m_sType = rhs.m_sType;

  m_Flags         = rhs.m_Flags;
  m_ConstantValue = rhs.m_ConstantValue;

  xiiAttributeHolder::operator=(rhs);
}

xiiReflectedPropertyDescriptor::~xiiReflectedPropertyDescriptor() = default;


////////////////////////////////////////////////////////////////////////
// xiiFunctionParameterDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiFunctionArgumentDescriptor, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiFunctionArgumentDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sType)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiPropertyFlags, m_Flags)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiFunctionArgumentDescriptor::xiiFunctionArgumentDescriptor() = default;

xiiFunctionArgumentDescriptor::xiiFunctionArgumentDescriptor(xiiStringView sType, xiiBitflags<xiiPropertyFlags> flags) :
  m_sType(sType), m_Flags(flags)
{
}


////////////////////////////////////////////////////////////////////////
// xiiReflectedFunctionDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiReflectedFunctionDescriptor, xiiAttributeHolder, 1, xiiRTTIDefaultAllocator<xiiReflectedFunctionDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiPropertyFlags, m_Flags)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiFunctionType, m_Type)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("ReturnValue", m_ReturnValue)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_ARRAY_MEMBER_PROPERTY("Arguments", m_Arguments)->AddAttributes(new xiiNetworkAttribute(5)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiReflectedFunctionDescriptor::xiiReflectedFunctionDescriptor() = default;

xiiReflectedFunctionDescriptor::xiiReflectedFunctionDescriptor(xiiStringView sName, xiiBitflags<xiiPropertyFlags> flags, xiiEnum<xiiFunctionType> type, xiiArrayPtr<const xiiPropertyAttribute* const> attributes) :
  m_sName(sName), m_Flags(flags), m_Type(type)
{
  m_ReferenceAttributes = attributes;
}

xiiReflectedFunctionDescriptor::xiiReflectedFunctionDescriptor(const xiiReflectedFunctionDescriptor& rhs)
{
  operator=(rhs);
}

xiiReflectedFunctionDescriptor::~xiiReflectedFunctionDescriptor() = default;

void xiiReflectedFunctionDescriptor::operator=(const xiiReflectedFunctionDescriptor& rhs)
{
  m_sName       = rhs.m_sName;
  m_Flags       = rhs.m_Flags;
  m_Type        = rhs.m_Type;
  m_ReturnValue = rhs.m_ReturnValue;
  m_Arguments   = rhs.m_Arguments;
  xiiAttributeHolder::operator=(rhs);
}

////////////////////////////////////////////////////////////////////////
// xiiReflectedTypeDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiReflectedTypeDescriptor, xiiAttributeHolder, 1, xiiRTTIDefaultAllocator<xiiReflectedTypeDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TypeName", m_sTypeName)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("PluginName", m_sPluginName)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("ParentTypeName", m_sParentTypeName)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiTypeFlags, m_Flags)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_ARRAY_MEMBER_PROPERTY("Properties", m_Properties)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_ARRAY_MEMBER_PROPERTY("Functions", m_Functions)->AddAttributes(new xiiNetworkAttribute(6)),
    XII_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion)->AddAttributes(new xiiNetworkAttribute(7)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiReflectedTypeDescriptor::~xiiReflectedTypeDescriptor() = default;
