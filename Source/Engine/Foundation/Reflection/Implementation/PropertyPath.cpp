#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/UniquePtr.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiPropertyPathStep, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiPropertyPathStep>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Property", m_sProperty),
    XII_MEMBER_PROPERTY("Index", m_Index),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiPropertyPath::xiiPropertyPath()  = default;
xiiPropertyPath::~xiiPropertyPath() = default;

bool xiiPropertyPath::IsValid() const
{
  return m_bIsValid;
}

xiiResult xiiPropertyPath::InitializeFromPath(const xiiRTTI& rootObjectRtti, xiiStringView sPath)
{
  m_bIsValid = false;

  const xiiStringBuilder sPathParts = sPath;
  xiiStringBuilder       sIndex;
  xiiStringBuilder       sFieldName;

  xiiHybridArray<xiiStringView, 4> parts;
  sPathParts.Split(false, parts, "/");

  // an empty path is valid as well

  m_PathSteps.Clear();
  m_PathSteps.Reserve(parts.GetCount());

  const xiiRTTI* pCurRtti = &rootObjectRtti;

  for (const xiiStringView& part : parts)
  {
    if (part.EndsWith("]"))
    {
      const char* szBracket = part.FindSubString("[");

      sIndex.SetSubString_FromTo(szBracket + 1, part.GetEndPointer() - 1);

      sFieldName.SetSubString_FromTo(part.GetStartPointer(), szBracket);
    }
    else
    {
      sFieldName = part;
      sIndex.Clear();
    }

    const xiiAbstractProperty* pAbsProp = pCurRtti->FindPropertyByName(sFieldName);

    if (pAbsProp == nullptr)
      return XII_FAILURE;

    auto& step       = m_PathSteps.ExpandAndGetRef();
    step.m_pProperty = pAbsProp;

    if (pAbsProp->GetCategory() == xiiPropertyCategory::Array)
    {
      if (sIndex.IsEmpty())
      {
        step.m_Index = xiiVariant();
      }
      else
      {
        xiiInt32 iIndex;
        XII_SUCCEED_OR_RETURN(xiiConversionUtils::StringToInt(sIndex, iIndex));
        step.m_Index = iIndex;
      }
    }
    else if (pAbsProp->GetCategory() == xiiPropertyCategory::Set)
    {
      if (sIndex.IsEmpty())
      {
        step.m_Index = xiiVariant();
      }
      else
      {
        return XII_FAILURE;
      }
    }
    else if (pAbsProp->GetCategory() == xiiPropertyCategory::Map)
    {
      step.m_Index = sIndex.IsEmpty() ? xiiVariant() : xiiVariant(sIndex.GetData());
    }

    pCurRtti = pAbsProp->GetSpecificType();
  }

  m_bIsValid = true;
  return XII_SUCCESS;
}

xiiResult xiiPropertyPath::InitializeFromPath(const xiiRTTI* pRootObjectRtti, const xiiArrayPtr<const xiiPropertyPathStep> path)
{
  m_bIsValid = false;

  m_PathSteps.Clear();
  m_PathSteps.Reserve(path.GetCount());

  const xiiRTTI* pCurRtti = pRootObjectRtti;
  for (const xiiPropertyPathStep& pathStep : path)
  {
    const xiiAbstractProperty* pAbsProp = pCurRtti->FindPropertyByName(pathStep.m_sProperty);
    if (pAbsProp == nullptr)
      return XII_FAILURE;

    auto& step       = m_PathSteps.ExpandAndGetRef();
    step.m_pProperty = pAbsProp;
    step.m_Index     = pathStep.m_Index;

    pCurRtti = pAbsProp->GetSpecificType();
  }

  m_bIsValid = true;
  return XII_SUCCESS;
}

xiiResult xiiPropertyPath::WriteToLeafObject(void* pRootObject, const xiiRTTI& type, xiiDelegate<void(void* pLeaf, const xiiRTTI& pType)> func) const
{
  XII_ASSERT_DEBUG(m_PathSteps.IsEmpty() || m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetTypeFlags().IsSet(xiiTypeFlags::Class), "To resolve the leaf object the path needs to be empty or end in a class.");

  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr(), true, func);
}

xiiResult xiiPropertyPath::ReadFromLeafObject(void* pRootObject, const xiiRTTI& type, xiiDelegate<void(void* pLeaf, const xiiRTTI& pType)> func) const
{
  XII_ASSERT_DEBUG(m_PathSteps.IsEmpty() || m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetTypeFlags().IsSet(xiiTypeFlags::Class), "To resolve the leaf object the path needs to be empty or end in a class.");

  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr(), false, func);
}

xiiResult xiiPropertyPath::WriteProperty(void* pRootObject, const xiiRTTI& type, xiiDelegate<void(void* pLeafObject, const xiiRTTI& pLeafType, const xiiAbstractProperty* pProp, const xiiVariant& index)> func) const
{
  XII_ASSERT_DEBUG(!m_PathSteps.IsEmpty(), "Call InitializeFromPath before WriteToObject");

  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr().GetSubArray(0, m_PathSteps.GetCount() - 1), true,
                     [this, &func](void* pLeafObject, const xiiRTTI& leafType) {
                       auto& lastStep = m_PathSteps[m_PathSteps.GetCount() - 1];
                       func(pLeafObject, leafType, lastStep.m_pProperty, lastStep.m_Index);
                     });
}

xiiResult xiiPropertyPath::ReadProperty(void* pRootObject, const xiiRTTI& type, xiiDelegate<void(void* pLeafObject, const xiiRTTI& pLeafType, const xiiAbstractProperty* pProp, const xiiVariant& index)> func) const
{
  XII_ASSERT_DEBUG(m_bIsValid, "Call InitializeFromPath before WriteToObject");

  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr().GetSubArray(0, m_PathSteps.GetCount() - 1), false,
                     [this, &func](void* pLeafObject, const xiiRTTI& leafType) {
                       auto& lastStep = m_PathSteps[m_PathSteps.GetCount() - 1];
                       func(pLeafObject, leafType, lastStep.m_pProperty, lastStep.m_Index);
                     });
}

void xiiPropertyPath::SetValue(void* pRootObject, const xiiRTTI& type, const xiiVariant& value) const
{
  // XII_ASSERT_DEBUG(!m_PathSteps.IsEmpty() && value.CanConvertTo(m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetVariantType()), "The given value does not match the type at the given path.");

  WriteProperty(pRootObject, type, [&value](void* pLeaf, const xiiRTTI& type, const xiiAbstractProperty* pProp, const xiiVariant& index) {
    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Member:
        xiiReflectionUtils::SetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProp), pLeaf, value);
        break;
      case xiiPropertyCategory::Array:
        xiiReflectionUtils::SetArrayPropertyValue(static_cast<const xiiAbstractArrayProperty*>(pProp), pLeaf, index.Get<xiiInt32>(), value);
        break;
      case xiiPropertyCategory::Map:
        xiiReflectionUtils::SetMapPropertyValue(static_cast<const xiiAbstractMapProperty*>(pProp), pLeaf, index.Get<xiiString>(), value);
        break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }).IgnoreResult();
}

void xiiPropertyPath::GetValue(void* pRootObject, const xiiRTTI& type, xiiVariant& out_value) const
{
  // XII_ASSERT_DEBUG(!m_PathSteps.IsEmpty() && m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetVariantType() != xiiVariantType::Invalid, "The property path of value {} cannot be stored in a xiiVariant.", m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetTypeName());

  ReadProperty(pRootObject, type, [&out_value](void* pLeaf, const xiiRTTI& type, const xiiAbstractProperty* pProp, const xiiVariant& index) {
    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Member:
        out_value = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProp), pLeaf);
        break;
      case xiiPropertyCategory::Array:
        out_value = xiiReflectionUtils::GetArrayPropertyValue(static_cast<const xiiAbstractArrayProperty*>(pProp), pLeaf, index.Get<xiiInt32>());
        break;
      case xiiPropertyCategory::Map:
        out_value = xiiReflectionUtils::GetMapPropertyValue(static_cast<const xiiAbstractMapProperty*>(pProp), pLeaf, index.Get<xiiString>());
        break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }).IgnoreResult();
}

xiiResult xiiPropertyPath::ResolvePath(void* pCurrentObject, const xiiRTTI* pType, const xiiArrayPtr<const ResolvedStep> path, bool bWriteToObject, const xiiDelegate<void(void* pLeaf, const xiiRTTI& pType)>& func)
{
  if (path.IsEmpty())
  {
    func(pCurrentObject, *pType);
    return XII_SUCCESS;
  }
  else // Recurse
  {
    const xiiAbstractProperty* pProp     = path[0].m_pProperty;
    const xiiRTTI*             pPropType = pProp->GetSpecificType();

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Member:
      {
        auto pSpecific = static_cast<const xiiAbstractMemberProperty*>(pProp);
        if (pPropType->GetProperties().GetCount() > 0)
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pCurrentObject);
          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            return ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            void* pRetrievedSubObject = pPropType->GetAllocator()->Allocate<void>();
            pSpecific->GetValuePtr(pCurrentObject, pRetrievedSubObject);

            xiiResult res = ResolvePath(pRetrievedSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

            if (bWriteToObject)
              pSpecific->SetValuePtr(pCurrentObject, pRetrievedSubObject);

            pPropType->GetAllocator()->Deallocate(pRetrievedSubObject);
            return res;
          }
          else
          {
            XII_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
          }
        }
      }
      break;
      case xiiPropertyCategory::Array:
      {
        auto pSpecific = static_cast<const xiiAbstractArrayProperty*>(pProp);

        if (pPropType->GetAllocator()->CanAllocate())
        {
          const xiiUInt32 uiIndex = path[0].m_Index.ConvertTo<xiiUInt32>();
          if (uiIndex >= pSpecific->GetCount(pCurrentObject))
            return XII_FAILURE;

          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
          pSpecific->GetValue(pCurrentObject, uiIndex, pSubObject);

          xiiResult res = ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

          if (bWriteToObject)
            pSpecific->SetValue(pCurrentObject, uiIndex, pSubObject);

          pPropType->GetAllocator()->Deallocate(pSubObject);
          return res;
        }
        else
        {
          XII_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
        }
      }
      break;
      case xiiPropertyCategory::Map:
      {
        auto             pSpecific = static_cast<const xiiAbstractMapProperty*>(pProp);
        const xiiString& sKey      = path[0].m_Index.Get<xiiString>();
        if (!pSpecific->Contains(pCurrentObject, sKey))
          return XII_FAILURE;

        if (pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

          pSpecific->GetValue(pCurrentObject, sKey, pSubObject);

          xiiResult res = ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

          if (bWriteToObject)
            pSpecific->Insert(pCurrentObject, sKey, pSubObject);

          pPropType->GetAllocator()->Deallocate(pSubObject);
          return res;
        }
        else
        {
          XII_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
        }
      }
      break;
      case xiiPropertyCategory::Set:
      default:
      {
        XII_REPORT_FAILURE("Property of type Set should not be part of an object chain!");
      }
      break;
    }
    return XII_FAILURE;
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyPath);
