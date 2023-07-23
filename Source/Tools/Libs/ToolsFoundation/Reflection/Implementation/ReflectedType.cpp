#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAttributeHolder, xiiNoBase, 1, xiiRTTINoAllocator)
{
  flags.Add(xiiTypeFlags::Abstract);
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_ACCESSOR_PROPERTY("Attributes", GetCount, GetValue, SetValue, Insert, Remove)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiAttributeHolder::xiiAttributeHolder() = default;

xiiAttributeHolder::xiiAttributeHolder(const xiiAttributeHolder& rhs)
{
  m_Attributes = rhs.m_Attributes;
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

xiiAttributeHolder::~xiiAttributeHolder()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
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

xiiPropertyAttribute* xiiAttributeHolder::GetValue(xiiUInt32 uiIndex) const
{
  if (!m_ReferenceAttributes.IsEmpty())
    return m_ReferenceAttributes[uiIndex];

  return m_Attributes[uiIndex];
}

void xiiAttributeHolder::SetValue(xiiUInt32 uiIndex, xiiPropertyAttribute* value)
{
  m_Attributes[uiIndex] = value;
}

void xiiAttributeHolder::Insert(xiiUInt32 uiIndex, xiiPropertyAttribute* value)
{
  m_Attributes.Insert(value, uiIndex);
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
    XII_ENUM_MEMBER_PROPERTY("Category", xiiPropertyCategory, m_Category),
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Type", m_sType),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiPropertyFlags, m_Flags),
    XII_MEMBER_PROPERTY("ConstantValue", m_ConstantValue),
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

xiiReflectedPropertyDescriptor::xiiReflectedPropertyDescriptor(xiiPropertyCategory::Enum category, xiiStringView sName, xiiStringView sType, xiiBitflags<xiiPropertyFlags> flags, const xiiArrayPtr<xiiPropertyAttribute* const> attributes) :
  m_Category(category), m_sName(sName), m_sType(sType), m_Flags(flags)
{
  m_ReferenceAttributes = attributes;
}

xiiReflectedPropertyDescriptor::xiiReflectedPropertyDescriptor(xiiStringView sName, const xiiVariant& constantValue, const xiiArrayPtr<xiiPropertyAttribute* const> attributes) :
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
    XII_MEMBER_PROPERTY("Type", m_sType),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiPropertyFlags, m_Flags),
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
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiPropertyFlags, m_Flags),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiFunctionType, m_Type),
    XII_MEMBER_PROPERTY("ReturnValue", m_ReturnValue),
    XII_ARRAY_MEMBER_PROPERTY("Arguments", m_Arguments),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiReflectedFunctionDescriptor::xiiReflectedFunctionDescriptor() = default;

xiiReflectedFunctionDescriptor::xiiReflectedFunctionDescriptor(xiiStringView sName, xiiBitflags<xiiPropertyFlags> flags, xiiEnum<xiiFunctionType> type, const xiiArrayPtr<xiiPropertyAttribute* const> attributes) :
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
  m_sName                     = rhs.m_sName;
  m_Flags                     = rhs.m_Flags;
  m_ReturnValue               = rhs.m_ReturnValue;
  m_Arguments                 = rhs.m_Arguments;
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
    XII_MEMBER_PROPERTY("TypeName", m_sTypeName),
    XII_MEMBER_PROPERTY("PluginName", m_sPluginName),
    XII_MEMBER_PROPERTY("ParentTypeName", m_sParentTypeName),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiTypeFlags, m_Flags),
    XII_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
    XII_ARRAY_MEMBER_PROPERTY("Functions", m_Functions),
    XII_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiReflectedTypeDescriptor::~xiiReflectedTypeDescriptor() = default;
