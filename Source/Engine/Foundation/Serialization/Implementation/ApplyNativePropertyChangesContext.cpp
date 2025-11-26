#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>

xiiApplyNativePropertyChangesContext::xiiApplyNativePropertyChangesContext(xiiRttiConverterContext& ref_source, const xiiAbstractObjectGraph& originalGraph) :
  m_NativeContext(ref_source), m_OriginalGraph(originalGraph)
{
}

xiiUuid xiiApplyNativePropertyChangesContext::GenerateObjectGuid(const xiiUuid& parentGuid, const xiiAbstractProperty* pProp, xiiVariant index, void* pObject) const
{
  if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
  {
    // If the object is already known by the native context (a pointer that existed before the native changes)
    // we can just return it. Any other pointer will get a new guid assigned.
    xiiUuid guid = m_NativeContext.GetObjectGUID(pProp->GetSpecificType(), pObject);
    if (guid.IsValid())
      return guid;
  }
  else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
  {
    // In case of by-value classes we lookup the guid in the object manager graph by using
    // the index as the identify of the object. If the index is not valid (e.g. the array was expanded by native changes)
    // a new guid is assigned.
    if (const xiiAbstractObjectNode* originalNode = m_OriginalGraph.GetNode(parentGuid))
    {
      if (const xiiAbstractObjectNode::Property* originalProp = originalNode->FindProperty(pProp->GetPropertyName()))
      {
        switch (pProp->GetCategory())
        {
          case xiiPropertyCategory::Member:
          {
            if (originalProp->m_Value.IsA<xiiUuid>() && originalProp->m_Value.Get<xiiUuid>().IsValid())
              return originalProp->m_Value.Get<xiiUuid>();
          }
          break;
          case xiiPropertyCategory::Array:
          {
            xiiUInt32 uiIndex = index.Get<xiiUInt32>();
            if (originalProp->m_Value.IsA<xiiVariantArray>())
            {
              const xiiVariantArray& values = originalProp->m_Value.Get<xiiVariantArray>();
              if (uiIndex < values.GetCount())
              {
                const auto& originalElemValue = values[uiIndex];
                if (originalElemValue.IsA<xiiUuid>() && originalElemValue.Get<xiiUuid>().IsValid())
                  return originalElemValue.Get<xiiUuid>();
              }
            }
          }
          break;
          case xiiPropertyCategory::Map:
          {
            const xiiString& sIndex = index.Get<xiiString>();
            if (originalProp->m_Value.IsA<xiiVariantDictionary>())
            {
              const xiiVariantDictionary& values = originalProp->m_Value.Get<xiiVariantDictionary>();
              if (values.Contains(sIndex))
              {
                const auto& originalElemValue = *values.GetValue(sIndex);
                if (originalElemValue.IsA<xiiUuid>() && originalElemValue.Get<xiiUuid>().IsValid())
                  return originalElemValue.Get<xiiUuid>();
              }
            }
          }
          break;

          default:
            break;
        }
      }
    }
  }

  return xiiUuid::MakeUuid();
}


XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_ApplyNativePropertyChangesContext);
