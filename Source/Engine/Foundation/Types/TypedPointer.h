/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/TypeTraits.h>

class xiiRTTI;

/// A typed raw pointer.
///
/// Common use case is the storage of object pointers inside a xiiVariant.
/// Has the same lifetime concerns that any other raw pointer.
/// \sa xiiVariant
struct xiiTypedPointer
{
  XII_DECLARE_POD_TYPE();
  void*          m_pObject = nullptr;
  const xiiRTTI* m_pType   = nullptr;

  xiiTypedPointer() = default;
  xiiTypedPointer(void* pObject, const xiiRTTI* pType) :
    m_pObject(pObject), m_pType(pType)
  {
  }

  bool operator==(const xiiTypedPointer& rhs) const
  {
    return m_pObject == rhs.m_pObject;
  }
};
