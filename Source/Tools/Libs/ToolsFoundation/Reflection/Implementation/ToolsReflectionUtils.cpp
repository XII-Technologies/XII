#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  struct GetDoubleFunc
  {
    GetDoubleFunc(const xiiVariant& value) :
      m_Value(value)
    {
    }
    template <typename T>
    void operator()()
    {
      if (m_Value.CanConvertTo<double>())
      {
        m_fValue = m_Value.ConvertTo<double>();
        m_bValid = true;
      }
    }

    const xiiVariant& m_Value;
    double            m_fValue = 0;
    bool              m_bValid = false;
  };

  template <>
  void GetDoubleFunc::operator()<xiiAngle>()
  {
    m_fValue = m_Value.Get<xiiAngle>().GetDegree();
    m_bValid = true;
  }

  template <>
  void GetDoubleFunc::operator()<xiiAngled>()
  {
    m_fValue = m_Value.Get<xiiAngled>().GetDegree();
    m_bValid = true;
  }

  template <>
  void GetDoubleFunc::operator()<xiiTime>()
  {
    m_fValue = m_Value.Get<xiiTime>().GetSeconds();
    m_bValid = true;
  }

  struct GetVariantFunc
  {
    GetVariantFunc(double fValue, xiiVariantType::Enum type, xiiVariant& out_value) :
      m_fValue(fValue), m_Type(type), m_Value(out_value)
    {
    }
    template <typename T>
    void operator()()
    {
      m_Value = m_fValue;
      if (m_Value.CanConvertTo(m_Type))
      {
        m_Value  = m_Value.ConvertTo(m_Type);
        m_bValid = true;
      }
      else
      {
        m_Value = xiiVariant();
      }
    }

    double               m_fValue;
    xiiVariantType::Enum m_Type;
    xiiVariant&          m_Value;
    bool                 m_bValid = false;
  };

  template <>
  void GetVariantFunc::operator()<xiiAngle>()
  {
    m_Value  = xiiAngle::Degree((float)m_fValue);
    m_bValid = true;
  }

  template <>
  void GetVariantFunc::operator()<xiiAngled>()
  {
    m_Value  = xiiAngled::Degree(m_fValue);
    m_bValid = true;
  }

  template <>
  void GetVariantFunc::operator()<xiiTime>()
  {
    m_Value  = xiiTime::Seconds(m_fValue);
    m_bValid = true;
  }
} // namespace
////////////////////////////////////////////////////////////////////////
// xiiToolsReflectionUtils public functions
////////////////////////////////////////////////////////////////////////

xiiVariant xiiToolsReflectionUtils::GetStorageDefault(const xiiAbstractProperty* pProperty)
{
  const xiiDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<xiiDefaultValueAttribute>();
  auto                            type    = pProperty->GetFlags().IsSet(xiiPropertyFlags::StandardType) ? pProperty->GetSpecificType()->GetVariantType() : xiiVariantType::Uuid;

  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProperty);

  switch (pProperty->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      return xiiReflectionUtils::GetDefaultValue(pProperty);
    }
    break;
    case xiiPropertyCategory::Array:
    case xiiPropertyCategory::Set:
    {
      if (bIsValueType && pAttrib && pAttrib->GetValue().IsA<xiiVariantArray>())
      {
        const xiiVariantArray& value = pAttrib->GetValue().Get<xiiVariantArray>();
        xiiVariantArray        ret;
        ret.SetCount(value.GetCount());
        for (xiiUInt32 i = 0; i < value.GetCount(); i++)
        {
          ret[i] = value[i].ConvertTo(type);
        }
        return ret;
      }
      return xiiVariantArray();
    }
    break;
    case xiiPropertyCategory::Map:
    {
      return xiiVariantDictionary();
    }
    break;
    case xiiPropertyCategory::Constant:
    case xiiPropertyCategory::Function:
      break; // no defaults
  }
  return xiiVariant();
}

bool xiiToolsReflectionUtils::GetFloatFromVariant(const xiiVariant& val, double& out_fValue)
{
  if (val.IsValid())
  {
    GetDoubleFunc func(val);
    xiiVariant::DispatchTo(func, val.GetType());
    out_fValue = func.m_fValue;
    return func.m_bValid;
  }
  return false;
}


bool xiiToolsReflectionUtils::GetVariantFromFloat(double fValue, xiiVariantType::Enum type, xiiVariant& out_val)
{
  GetVariantFunc func(fValue, type, out_val);
  xiiVariant::DispatchTo(func, type);

  return func.m_bValid;
}

void xiiToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(const xiiRTTI* pRtti, xiiReflectedTypeDescriptor& out_desc)
{
  GetMinimalReflectedTypeDescriptorFromRtti(pRtti, out_desc);
  out_desc.m_Flags.Remove(xiiTypeFlags::Minimal);

  auto            rttiProps = pRtti->GetProperties();
  const xiiUInt32 uiCount   = rttiProps.GetCount();
  out_desc.m_Properties.Reserve(uiCount);
  for (xiiUInt32 i = 0; i < uiCount; ++i)
  {
    const xiiAbstractProperty* prop = rttiProps[i];

    switch (prop->GetCategory())
    {
      case xiiPropertyCategory::Constant:
      {
        auto           constantProp = static_cast<const xiiAbstractConstantProperty*>(prop);
        const xiiRTTI* pPropRtti    = constantProp->GetSpecificType();
        if (xiiReflectionUtils::IsBasicType(pPropRtti))
        {
          xiiVariant value = constantProp->GetConstant();
          XII_ASSERT_DEV(pPropRtti->GetVariantType() == value.GetType(), "Variant value type and property type should always match!");
          out_desc.m_Properties.PushBack(xiiReflectedPropertyDescriptor(constantProp->GetPropertyName(), value, prop->GetAttributes()));
        }
        else
        {
          XII_ASSERT_DEV(false, "Non-pod constants are not supported yet!");
        }
      }
      break;

      case xiiPropertyCategory::Member:
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      case xiiPropertyCategory::Map:
      {
        const xiiRTTI* pPropRtti = prop->GetSpecificType();
        out_desc.m_Properties.PushBack(xiiReflectedPropertyDescriptor(prop->GetCategory(), prop->GetPropertyName(), pPropRtti->GetTypeName(), prop->GetFlags(), prop->GetAttributes()));
      }
      break;

      case xiiPropertyCategory::Function:
        break;

      default:
        break;
    }
  }

  auto            rttiFunc    = pRtti->GetFunctions();
  const xiiUInt32 uiFuncCount = rttiFunc.GetCount();
  out_desc.m_Functions.Reserve(uiFuncCount);

  for (xiiUInt32 i = 0; i < uiFuncCount; ++i)
  {
    const xiiAbstractFunctionProperty* prop = rttiFunc[i];
    out_desc.m_Functions.PushBack(xiiReflectedFunctionDescriptor(prop->GetPropertyName(), prop->GetFlags(), prop->GetFunctionType(), prop->GetAttributes()));
    xiiReflectedFunctionDescriptor& desc = out_desc.m_Functions.PeekBack();
    desc.m_ReturnValue                   = xiiFunctionArgumentDescriptor(prop->GetReturnType() ? prop->GetReturnType()->GetTypeName() : "", prop->GetReturnFlags());
    const xiiUInt32 uiArguments          = prop->GetArgumentCount();
    desc.m_Arguments.Reserve(uiArguments);
    for (xiiUInt32 a = 0; a < uiArguments; ++a)
    {
      desc.m_Arguments.PushBack(xiiFunctionArgumentDescriptor(prop->GetArgumentType(a)->GetTypeName(), prop->GetArgumentFlags(a)));
    }
  }

  out_desc.m_ReferenceAttributes = pRtti->GetAttributes();
}


void xiiToolsReflectionUtils::GetMinimalReflectedTypeDescriptorFromRtti(const xiiRTTI* pRtti, xiiReflectedTypeDescriptor& out_desc)
{
  XII_ASSERT_DEV(pRtti != nullptr, "Type to process must not be null!");
  out_desc.m_sTypeName       = pRtti->GetTypeName();
  out_desc.m_sPluginName     = pRtti->GetPluginName();
  out_desc.m_Flags           = pRtti->GetTypeFlags() | xiiTypeFlags::Minimal;
  out_desc.m_uiTypeVersion   = pRtti->GetTypeVersion();
  const xiiRTTI* pParentRtti = pRtti->GetParentType();
  out_desc.m_sParentTypeName = pParentRtti ? pParentRtti->GetTypeName() : nullptr;

  out_desc.m_Properties.Clear();
  out_desc.m_Functions.Clear();
  out_desc.m_Attributes.Clear();
  out_desc.m_ReferenceAttributes = xiiArrayPtr<xiiPropertyAttribute* const>();
}

static void GatherObjectTypesInternal(const xiiDocumentObject* pObject, xiiSet<const xiiRTTI*>& inout_types)
{
  inout_types.Insert(pObject->GetTypeAccessor().GetType());
  xiiReflectionUtils::GatherDependentTypes(pObject->GetTypeAccessor().GetType(), inout_types);

  for (const xiiDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<xiiTemporaryAttribute>() != nullptr)
      continue;

    GatherObjectTypesInternal(pChild, inout_types);
  }
}

void xiiToolsReflectionUtils::GatherObjectTypes(const xiiDocumentObject* pObject, xiiSet<const xiiRTTI*>& inout_types)
{
  GatherObjectTypesInternal(pObject, inout_types);
}

bool xiiToolsReflectionUtils::DependencySortTypeDescriptorArray(xiiDynamicArray<xiiReflectedTypeDescriptor*>& ref_descriptors)
{
  xiiMap<xiiReflectedTypeDescriptor*, xiiSet<xiiString>> dependencies;

  xiiSet<xiiString> typesInArray;
  // Gather all types in array
  for (xiiReflectedTypeDescriptor* desc : ref_descriptors)
  {
    typesInArray.Insert(desc->m_sTypeName);
  }

  // Find all direct dependencies to types in the array for each type.
  for (xiiReflectedTypeDescriptor* desc : ref_descriptors)
  {
    auto it = dependencies.Insert(desc, xiiSet<xiiString>());

    if (typesInArray.Contains(desc->m_sParentTypeName))
    {
      it.Value().Insert(desc->m_sParentTypeName);
    }
    for (xiiReflectedPropertyDescriptor& propDesc : desc->m_Properties)
    {
      if (typesInArray.Contains(propDesc.m_sType))
      {
        it.Value().Insert(propDesc.m_sType);
      }
    }
  }

  xiiSet<xiiString>                            accu;
  xiiDynamicArray<xiiReflectedTypeDescriptor*> sorted;
  sorted.Reserve(ref_descriptors.GetCount());
  // Build new sorted types array.
  while (!ref_descriptors.IsEmpty())
  {
    bool bDeadEnd = true;
    for (xiiReflectedTypeDescriptor* desc : ref_descriptors)
    {
      // Are the types dependencies met?
      if (accu.ContainsSet(dependencies[desc]))
      {
        sorted.PushBack(desc);
        bDeadEnd = false;
        ref_descriptors.RemoveAndCopy(desc);
        accu.Insert(desc->m_sTypeName);
        break;
      }
    }

    if (bDeadEnd)
    {
      return false;
    }
  }

  ref_descriptors = sorted;
  return true;
}
