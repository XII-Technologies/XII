#pragma once

#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/Variant.h>

class xiiDocumentObject;

struct XII_GUIFOUNDATION_DLL xiiPropertySelection
{
  const xiiDocumentObject* m_pObject;
  xiiVariant               m_Index;

  bool operator==(const xiiPropertySelection& rhs) const { return m_pObject == rhs.m_pObject && m_Index == rhs.m_Index; }

  bool operator<(const xiiPropertySelection& rhs) const
  {
    // Qt6 requires the less than operator but never calls it, so we use this dummy for now.
    XII_ASSERT_NOT_IMPLEMENTED;
    return false;
  }
};

struct XII_GUIFOUNDATION_DLL xiiPropertyClipboard
{
  xiiString  m_Type;
  xiiVariant m_Value;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GUIFOUNDATION_DLL, xiiPropertyClipboard)
