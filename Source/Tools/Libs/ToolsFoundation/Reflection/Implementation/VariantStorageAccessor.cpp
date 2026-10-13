/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Math/Math.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

namespace
{
  bool TryGetArrayIndex(const xiiVariant& index, xiiUInt32& out_index)
  {
    if (!index.IsNumber())
      return false;
    const double value = index.ConvertTo<double>();
    if (!xiiMath::IsFinite(value) || value < 0 || value > xiiMath::MaxValue<xiiUInt32>() || value != xiiMath::Floor(value))
      return false;
    out_index = static_cast<xiiUInt32>(value);
    return true;
  }
} // namespace

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
  if (pRes)
    *pRes = XII_SUCCESS;
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
    const xiiVariantArray& values = m_Value.Get<xiiVariantArray>();
    xiiUInt32              uiIndex;
    if (TryGetArrayIndex(index, uiIndex) && uiIndex < values.GetCount())
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
    xiiUInt32 uiIndex;
    if (!TryGetArrayIndex(index, uiIndex) || uiIndex >= m_Value.Get<xiiVariantArray>().GetCount())
    {
      return xiiStatus(xiiFmt("Index '{0}' for property '{1}' is out of bounds.", index, m_sProperty));
    }
    xiiVariantArray& values = m_Value.GetWritable<xiiVariantArray>();
    values[uiIndex]         = value;
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
  out_keys.Clear();
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
    const auto count   = m_Value.Get<xiiVariantArray>().GetCount();
    xiiUInt32  uiIndex = count;
    // -1 is the established append sentinel. Other negatives, fractions and narrowing overflow
    // must not wrap or truncate into valid positions.
    if (index.ConvertTo<double>() != -1.0 && (!TryGetArrayIndex(index, uiIndex) || uiIndex > count))
      return xiiStatus(xiiFmt("InsertValue: index '{0}' for property '{1}' is out of bounds.", index, m_sProperty));
    m_Value.GetWritable<xiiVariantArray>().InsertAt(uiIndex, value);
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
    xiiUInt32 uiIndex;
    if (!TryGetArrayIndex(index, uiIndex) || uiIndex >= m_Value.Get<xiiVariantArray>().GetCount())
      return xiiStatus(xiiFmt("RemoveValue: index '{0}' for property '{1}' is out of bounds.", index, m_sProperty));
    m_Value.GetWritable<xiiVariantArray>().RemoveAtAndCopy(uiIndex);
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
    const auto count = m_Value.Get<xiiVariantArray>().GetCount();
    xiiUInt32  uiOldIndex, uiNewIndex;
    if (TryGetArrayIndex(oldIndex, uiOldIndex) && TryGetArrayIndex(newIndex, uiNewIndex) && uiOldIndex < count && uiNewIndex <= count)
    {
      xiiVariantArray& values = m_Value.GetWritable<xiiVariantArray>();
      xiiVariant       value  = values[uiOldIndex];
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
      return xiiStatus(xiiFmt("MoveValue: index '{0}' or '{1}' for property '{2}' is out of bounds.", oldIndex, newIndex, m_sProperty));
    }
  }
  else if (m_Value.IsA<xiiVariantDictionary>() && oldIndex.IsA<xiiString>() && newIndex.IsA<xiiString>())
  {
    xiiVariantDictionary& values    = m_Value.GetWritable<xiiVariantDictionary>();
    const xiiString&      sOldIndex = oldIndex.Get<xiiString>();
    const xiiString&      sNewIndex = newIndex.Get<xiiString>();

    if (!values.Contains(sOldIndex))
      return xiiStatus(xiiFmt("MoveValue: old index '{0}' for property '{1}' does not exist.", sOldIndex, m_sProperty));
    else if (values.Contains(sNewIndex))
      return xiiStatus(xiiFmt("MoveValue: new index '{0}' for property '{1}' already exists.", sNewIndex, m_sProperty));

    values.Insert(sNewIndex, values[sOldIndex]);
    values.Remove(sOldIndex);
    return XII_SUCCESS;
  }
  return xiiStatus(xiiFmt("MoveValue: Property '{0}' is not a container or index '{1}' or '{2}' is invalid.", m_sProperty, oldIndex, newIndex));
}
