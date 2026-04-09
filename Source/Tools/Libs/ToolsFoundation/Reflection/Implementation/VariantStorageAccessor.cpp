#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

xiiVariantStorageAccessor::xiiVariantStorageAccessor(xiiStringView sProperty, xiiVariant& value) :
  m_sProperty(sProperty), m_Value(value)
{
}

xiiVariantStorageAccessor::xiiVariantStorageAccessor(xiiStringView sProperty, const xiiVariant& value) :
  m_sProperty(sProperty), m_Value(const_cast<xiiVariant&>(value))
{
}

xiiVariant xiiVariantStorageAccessor::GetValue(xiiVariant index, xiiStatus* pRes) const
{
  if (!index.IsValid())
    return m_Value;

  if (index.IsNumber())
  {
    if (!m_Value.IsA<xiiVariantArray>())
    {
      if (pRes)
        *pRes = xiiStatus(xiiFmt("Index '{0}' for property '{1}' is invalid as the property is not an array.", index, m_sProperty));
      return xiiVariant();
    }
    const xiiVariantArray& values  = m_Value.Get<xiiVariantArray>();
    xiiUInt32              uiIndex = index.ConvertTo<xiiUInt32>();
    if (uiIndex < values.GetCount())
    {
      return values[uiIndex];
    }
  }
  else if (index.IsA<xiiString>())
  {
    if (!m_Value.IsA<xiiVariantDictionary>())
    {
      if (pRes)
        *pRes = xiiStatus(xiiFmt("Index '{0}' for property '{1}' is invalid as the property is not a dictionary.", index, m_sProperty));
      return xiiVariant();
    }
    const xiiVariantDictionary& values = m_Value.Get<xiiVariantDictionary>();
    const xiiString&            sIndex = index.Get<xiiString>();
    if (const xiiVariant* pValue = values.GetValue(sIndex))
    {
      return *pValue;
    }
  }

  if (pRes)
    *pRes = xiiStatus(xiiFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, m_sProperty));
  return xiiVariant();
}

xiiStatus xiiVariantStorageAccessor::SetValue(const xiiVariant& value, xiiVariant index)
{
  if (!index.IsValid())
  {
    m_Value = value;
    return XII_SUCCESS;
  }

  if (index.IsNumber() && m_Value.IsA<xiiVariantArray>())
  {
    xiiVariantArray& values  = m_Value.GetWritable<xiiVariantArray>();
    xiiUInt32        uiIndex = index.ConvertTo<xiiUInt32>();
    if (uiIndex >= values.GetCount())
    {
      return xiiStatus(xiiFmt("Index '{0}' for property '{1}' is out of bounds.", uiIndex, m_sProperty));
    }
    values[uiIndex] = value;
    return XII_SUCCESS;
  }
  else if (index.IsA<xiiString>() && m_Value.IsA<xiiVariantDictionary>())
  {
    xiiVariantDictionary& values = m_Value.GetWritable<xiiVariantDictionary>();
    const xiiString&      sIndex = index.Get<xiiString>();
    if (!values.Contains(sIndex))
    {
      return xiiStatus(xiiFmt("Index '{0}' for property '{1}' is out of bounds.", sIndex, m_sProperty));
    }
    values[sIndex] = value;
    return XII_SUCCESS;
  }
  return xiiStatus(xiiFmt("Index '{0}' for property '{1}' is invalid.", index, m_sProperty));
}

xiiInt32 xiiVariantStorageAccessor::GetCount() const
{
  if (m_Value.IsA<xiiVariantArray>())
    return m_Value.Get<xiiVariantArray>().GetCount();
  else if (m_Value.IsA<xiiVariantDictionary>())
    return m_Value.Get<xiiVariantDictionary>().GetCount();
  return 0;
}

xiiStatus xiiVariantStorageAccessor::GetKeys(xiiDynamicArray<xiiVariant>& out_keys) const
{
  if (m_Value.IsA<xiiVariantArray>())
  {
    const xiiVariantArray& values = m_Value.Get<xiiVariantArray>();
    out_keys.Reserve(values.GetCount());
    for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
    {
      out_keys.PushBack(i);
    }
    return XII_SUCCESS;
  }
  else if (m_Value.IsA<xiiVariantDictionary>())
  {
    const xiiVariantDictionary& values = m_Value.Get<xiiVariantDictionary>();
    out_keys.Reserve(values.GetCount());
    for (auto it = values.GetIterator(); it.IsValid(); ++it)
    {
      out_keys.PushBack(xiiVariant(it.Key()));
    }
    return XII_SUCCESS;
  }
  return xiiStatus(xiiFmt("Property '{0}' is not a container.", m_sProperty));
}

xiiStatus xiiVariantStorageAccessor::InsertValue(const xiiVariant& index, const xiiVariant& value)
{
  if (index.IsNumber() && m_Value.IsA<xiiVariantArray>())
  {
    xiiVariantArray& values = m_Value.GetWritable<xiiVariantArray>();
    xiiInt32         iIndex = index.ConvertTo<xiiInt32>();
    const xiiInt32   iCount = (xiiInt32)values.GetCount();
    if (iIndex == -1)
    {
      iIndex = iCount;
    }
    if (iIndex > iCount)
      return xiiStatus(xiiFmt("InsertValue: index '{0}' for property '{1}' is out of bounds.", iIndex, m_sProperty));

    values.InsertAt(iIndex, value);
    return XII_SUCCESS;
  }
  else if (index.IsA<xiiString>() && m_Value.IsA<xiiVariantDictionary>())
  {
    xiiVariantDictionary& values = m_Value.GetWritable<xiiVariantDictionary>();
    const xiiString&      sIndex = index.Get<xiiString>();
    if (values.Contains(index.Get<xiiString>()))
      return xiiStatus(xiiFmt("InsertValue: index '{0}' for property '{1}' already exists.", sIndex, m_sProperty));

    values.Insert(sIndex, value);
    return XII_SUCCESS;
  }
  return xiiStatus(xiiFmt("InsertValue: Property '{0}' is not a container or index {1} is invalid.", m_sProperty, index));
}

xiiStatus xiiVariantStorageAccessor::RemoveValue(const xiiVariant& index)
{
  if (index.IsNumber() && m_Value.IsA<xiiVariantArray>())
  {
    xiiVariantArray& values  = m_Value.GetWritable<xiiVariantArray>();
    const xiiUInt32  uiIndex = index.ConvertTo<xiiUInt32>();
    if (uiIndex > values.GetCount())
      return xiiStatus(xiiFmt("RemoveValue: index '{0}' for property '{1}' is out of bounds.", uiIndex, m_sProperty));

    values.RemoveAtAndCopy(uiIndex);
    return XII_SUCCESS;
  }
  else if (index.IsA<xiiString>() && m_Value.IsA<xiiVariantDictionary>())
  {
    xiiVariantDictionary& values = m_Value.GetWritable<xiiVariantDictionary>();
    const xiiString&      sIndex = index.Get<xiiString>();
    if (!values.Contains(index.Get<xiiString>()))
      return xiiStatus(xiiFmt("RemoveValue: index '{0}' for property '{1}' does not exists.", sIndex, m_sProperty));

    values.Remove(sIndex);
    return XII_SUCCESS;
  }
  return xiiStatus(xiiFmt("RemoveValue: Property '{0}' is not a container or index '{1}' is invalid.", m_sProperty, index));
}

xiiStatus xiiVariantStorageAccessor::MoveValue(const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  if (m_Value.IsA<xiiVariantArray>() && oldIndex.IsNumber() && newIndex.IsNumber())
  {
    xiiVariantArray& values     = m_Value.GetWritable<xiiVariantArray>();
    xiiUInt32        uiOldIndex = oldIndex.ConvertTo<xiiUInt32>();
    xiiUInt32        uiNewIndex = newIndex.ConvertTo<xiiUInt32>();
    if (uiOldIndex < values.GetCount() && uiNewIndex <= values.GetCount())
    {
      xiiVariant value = values[uiOldIndex];
      values.RemoveAtAndCopy(uiOldIndex);
      if (uiNewIndex > uiOldIndex)
      {
        uiNewIndex -= 1;
      }
      values.InsertAt(uiNewIndex, value);
      return XII_SUCCESS;
    }
    else
    {
      return xiiStatus(xiiFmt("MoveValue: index '{0}' or '{1}' for property '{2}' is out of bounds.", uiOldIndex, uiNewIndex, m_sProperty));
    }
  }
  else if (m_Value.IsA<xiiVariantDictionary>() && oldIndex.IsA<xiiString>() && newIndex.IsA<xiiString>())
  {
    xiiVariantDictionary& values    = m_Value.GetWritable<xiiVariantDictionary>();
    const xiiString&      sOldIndex = oldIndex.Get<xiiString>();
    const xiiString&      sNewIndex = newIndex.Get<xiiString>();

    if (!values.Contains(sOldIndex))
      return xiiStatus(xiiFmt("MoveValue: old index '{0}' for property '{2}' does not exist.", sOldIndex, m_sProperty));
    else if (values.Contains(sNewIndex))
      return xiiStatus(xiiFmt("MoveValue: new index '{0}' for property '{2}' already exists.", sNewIndex, m_sProperty));

    values.Insert(sNewIndex, values[sOldIndex]);
    values.Remove(sOldIndex);
    return XII_SUCCESS;
  }
  return xiiStatus(xiiFmt("MoveValue: Property '{0}' is not a container or index '{1}' or '{2}' is invalid.", m_sProperty, oldIndex, newIndex));
}
