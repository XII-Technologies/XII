#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>

xiiStatus xiiObjectPropertyPath::CreatePath(const xiiObjectPropertyPathContext& context, const xiiPropertyReference& prop, xiiStringBuilder& ref_sObjectSearchSequence, xiiStringBuilder& ref_sComponentType, xiiStringBuilder& ref_sPropertyPath)
{
  XII_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  const xiiRTTI* pObjType = xiiGetStaticRTTI<xiiGameObject>();

  const xiiAbstractProperty* pName   = pObjType->FindPropertyByName("Name");
  const xiiDocumentObject*   pObject = context.m_pAccessor->GetObjectManager()->GetObject(prop.m_Object);
  if (!pObject || !prop.m_pProperty)
    return xiiStatus(XII_FAILURE);

  {
    // Build property part of the path from the next parent node / component.
    pObject = FindParentNodeComponent(pObject);
    if (!pObject)
      return xiiStatus("No parent node or component found.");
    xiiObjectPropertyPathContext context2 = context;
    context2.m_pContextObject             = pObject;
    xiiStatus res                         = CreatePropertyPath(context2, prop, ref_sPropertyPath);
    if (res.Failed())
      return res;
  }

  {
    // Component part
    ref_sComponentType.Clear();
    if (pObject->GetType()->IsDerivedFrom(xiiGetStaticRTTI<xiiComponent>()))
    {
      ref_sComponentType = pObject->GetType()->GetTypeName();
      pObject            = pObject->GetParent();
    }
  }

  // Node path
  while (pObject != context.m_pContextObject)
  {
    if (pObject == nullptr)
    {
      ref_sObjectSearchSequence.Clear();
      ref_sComponentType.Clear();
      ref_sPropertyPath.Clear();
      return xiiStatus("Property is not under the given context object, no path exists.");
    }

    if (pObject->GetType() == xiiGetStaticRTTI<xiiGameObject>())
    {
      xiiString sName = context.m_pAccessor->Get<xiiString>(pObject, pName);
      if (!sName.IsEmpty())
      {
        if (!ref_sObjectSearchSequence.IsEmpty())
          ref_sObjectSearchSequence.Prepend("/");
        ref_sObjectSearchSequence.Prepend(sName);
      }
    }
    else
    {
      ref_sObjectSearchSequence.Clear();
      ref_sComponentType.Clear();
      ref_sPropertyPath.Clear();
      return xiiStatus(xiiFmt("Only xiiGameObject objects should be found in the hierarchy, found '{0}' instead.", pObject->GetType()->GetTypeName()));
    }

    pObject = pObject->GetParent();
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiObjectPropertyPath::CreatePropertyPath(
  const xiiObjectPropertyPathContext& context,
  const xiiPropertyReference&         prop,
  xiiStringBuilder&                   out_sPropertyPath)
{
  XII_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  const xiiDocumentObject* pObject = context.m_pAccessor->GetObjectManager()->GetObject(prop.m_Object);
  if (!pObject || !prop.m_pProperty)
    return xiiStatus(XII_FAILURE);

  out_sPropertyPath.Clear();
  xiiStatus res = PrependProperty(pObject, prop.m_pProperty, prop.m_Index, out_sPropertyPath);
  if (res.Failed())
    return res;

  while (pObject != context.m_pContextObject)
  {
    xiiStatus result = PrependProperty(pObject->GetParent(), pObject->GetParentPropertyType(), pObject->GetPropertyIndex(), out_sPropertyPath);
    if (result.Failed())
      return result;

    pObject = pObject->GetParent();
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiObjectPropertyPath::ResolvePath(const xiiObjectPropertyPathContext& context, xiiDynamicArray<xiiPropertyReference>& ref_keys, const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath)
{
  XII_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  ref_keys.Clear();
  const xiiDocumentObject*                    pContext = context.m_pContextObject;
  xiiDocumentObjectVisitor                    visitor(context.m_pAccessor->GetObjectManager(), "Children", context.m_sRootProperty);
  xiiHybridArray<const xiiDocumentObject*, 8> input;
  input.PushBack(pContext);
  xiiHybridArray<const xiiDocumentObject*, 8> output;

  // Find objects that match the search path
  xiiStringBuilder                 sObjectSearchSequence = szObjectSearchSequence;
  xiiHybridArray<xiiStringView, 4> names;
  sObjectSearchSequence.Split(false, names, "/");
  for (const xiiStringView& sName : names)
  {
    for (const xiiDocumentObject* pObj : input)
    {
      visitor.Visit(pObj, false, [&output, &sName](const xiiDocumentObject* pObject) -> bool {
        const auto& sObjectName = pObject->GetTypeAccessor().GetValue("Name").Get<xiiString>();
        if (sObjectName == sName)
        {
          output.PushBack(pObject);
          return false;
        }
        return true; //
      });
    }
    input.Clear();
    input.Swap(output);
  }

  if (input.IsEmpty())
    return xiiStatus(xiiFmt("ObjectSearchSequence: '{}' could not be resolved", szObjectSearchSequence));

  // Test found objects for component
  for (const xiiDocumentObject* pObject : input)
  {
    // Could also be the root object in which case we found nothing.
    if (pObject->GetType() == xiiGetStaticRTTI<xiiGameObject>())
    {
      if (xiiStringUtils::IsNullOrEmpty(szComponentType))
      {
        // We are animating the game object directly
        output.PushBack(pObject);
      }
      else
      {
        const xiiInt32 iComponents = pObject->GetTypeAccessor().GetCount("Components");
        for (xiiInt32 i = 0; i < iComponents; i++)
        {
          xiiVariant value  = pObject->GetTypeAccessor().GetValue("Components", i);
          auto       pChild = context.m_pAccessor->GetObjectManager()->GetObject(value.Get<xiiUuid>());
          if (pChild->GetType()->GetTypeName() == szComponentType)
          {
            output.PushBack(pChild);
            continue; // #TODO: break on found component?
          }
        }
      }
    }
  }
  input.Clear();
  input.Swap(output);

  if (input.IsEmpty())
    return xiiStatus(xiiFmt("Component '{}' not found on the search path '{}'", szComponentType, szObjectSearchSequence));

  xiiStatus lastError = xiiResult(XII_FAILURE);
  // Test found objects / components for property
  for (const xiiDocumentObject* pObject : input)
  {
    xiiObjectPropertyPathContext context2 = context;
    context2.m_pContextObject             = pObject;
    xiiPropertyReference key;
    xiiStatus            res = ResolvePropertyPath(context2, szPropertyPath, key);
    if (res.Succeeded())
    {
      ref_keys.PushBack(key);
    }

    if (lastError.Failed())
    {
      lastError = res;
    }
  }
  return lastError;
}

xiiStatus xiiObjectPropertyPath::ResolvePropertyPath(
  const xiiObjectPropertyPathContext& context,
  const char*                         szPropertyPath,
  xiiPropertyReference&               out_key)
{
  XII_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && szPropertyPath != nullptr, "All context fields must be valid.");
  const xiiDocumentObject*         pObject = context.m_pContextObject;
  xiiStringBuilder                 sPath   = szPropertyPath;
  xiiHybridArray<xiiStringView, 3> parts;
  sPath.Split(false, parts, "/");
  for (xiiUInt32 i = 0; i < parts.GetCount(); i++)
  {
    xiiStringBuilder                    sPart = parts[i];
    xiiHybridArray<xiiStringBuilder, 2> parts2;
    sPart.Split(false, parts2, "[", "]");
    if (parts2.GetCount() == 0 || parts2.GetCount() > 2)
    {
      return xiiStatus(xiiFmt("Malformed property path part: {0}", sPart));
    }
    const xiiAbstractProperty* pProperty = pObject->GetType()->FindPropertyByName(parts2[0]);
    if (!pProperty)
      return xiiStatus(xiiFmt("Property not found: {0}", parts2[0]));
    xiiVariant index;
    if (parts2.GetCount() == 2)
    {
      xiiInt32 iIndex = 0;
      if (xiiConversionUtils::StringToInt(parts2[1], iIndex).Succeeded())
      {
        index = iIndex; // Array index
      }
      else
      {
        index = parts2[1].GetData(); // Map index
      }
    }

    xiiVariant value;
    xiiStatus  res;
    if (const xiiExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<xiiExposedParametersAttribute>())
    {
      const xiiAbstractProperty* pParameterSourceProp = pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
      XII_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(),
                     pObject->GetType()->GetTypeName());
      xiiExposedParameterCommandAccessor proxy(context.m_pAccessor, pProperty, pParameterSourceProp);
      res = proxy.GetValue(pObject, pProperty, value, index);
    }
    else
    {
      res = context.m_pAccessor->GetValue(pObject, pProperty, value, index);
    }

    if (res.Failed())
      return res;

    if (i == parts.GetCount() - 1)
    {
      out_key.m_Object    = pObject->GetGuid();
      out_key.m_pProperty = pProperty;
      out_key.m_Index     = index;
      return xiiStatus(XII_SUCCESS);
    }
    else
    {
      if (value.IsA<xiiUuid>())
      {
        xiiUuid id = value.Get<xiiUuid>();
        pObject    = context.m_pAccessor->GetObjectManager()->GetObject(id);
      }
      else
      {
        return xiiStatus(xiiFmt("Property '{0}' of type '{1}' is not an object and can't be traversed further.", pProperty->GetPropertyName(),
                                pProperty->GetSpecificType()->GetTypeName()));
      }
    }
  }
  return xiiStatus(XII_FAILURE);
}

xiiStatus xiiObjectPropertyPath::PrependProperty(
  const xiiDocumentObject*   pObject,
  const xiiAbstractProperty* pProperty,
  xiiVariant                 index,
  xiiStringBuilder&          out_sPropertyPath)
{
  switch (pProperty->GetCategory())
  {
    case xiiPropertyCategory::Enum::Member:
    {
      if (!out_sPropertyPath.IsEmpty())
        out_sPropertyPath.Prepend("/");
      out_sPropertyPath.Prepend(pProperty->GetPropertyName());
      return xiiStatus(XII_SUCCESS);
    }
    case xiiPropertyCategory::Enum::Array:
    case xiiPropertyCategory::Enum::Map:
    {
      if (!out_sPropertyPath.IsEmpty())
        out_sPropertyPath.Prepend("/");
      if (index.IsValid())
        out_sPropertyPath.PrependFormat("{0}[{1}]", pProperty->GetPropertyName(), index);
      else
        out_sPropertyPath.PrependFormat("{0}", pProperty->GetPropertyName());
      return xiiStatus(XII_SUCCESS);
    }
    default:
      return xiiStatus(xiiFmt(
        "The property '{0}' of category '{1}' which is not supported in property paths", pProperty->GetPropertyName(), pProperty->GetCategory()));
  }
}

const xiiDocumentObject* xiiObjectPropertyPath::FindParentNodeComponent(const xiiDocumentObject* pObject)
{
  const xiiRTTI*           pObjType  = xiiGetStaticRTTI<xiiGameObject>();
  const xiiRTTI*           pCompType = xiiGetStaticRTTI<xiiComponent>();
  const xiiDocumentObject* pObj      = pObject;
  while (pObj != nullptr)
  {
    if (pObj->GetType() == pObjType)
    {
      return pObj;
    }
    else if (pObj->GetType()->IsDerivedFrom(pCompType))
    {
      return pObj;
    }
    pObj = pObj->GetParent();
  }
  return nullptr;
}
