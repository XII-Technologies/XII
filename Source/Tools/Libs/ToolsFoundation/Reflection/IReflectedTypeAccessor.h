/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

class xiiDocumentObject;
struct xiiStatus;

/// Provides access to the properties of a xiiRTTI compatible data storage.
class XII_TOOLSFOUNDATION_DLL xiiIReflectedTypeAccessor
{
public:
  /// Constructor for the xiiIReflectedTypeAccessor.
  ///
  /// It is a valid implementation to pass an invalid handle. Note that in this case there is no way to determine
  /// what is actually stored inside. However, it can be useful to use e.g. the xiiReflectedTypeDirectAccessor
  /// to set properties on the engine runtime side without having the xiiPhantomRttiManager initialized.
  xiiIReflectedTypeAccessor(const xiiRTTI* pRtti, xiiDocumentObject* pOwner) :
    m_pRtti(pRtti), m_pOwner(pOwner)
  {
  } // [tested]

  /// Returns the xiiRTTI* of the wrapped instance type.
  const xiiRTTI* GetType() const { return m_pRtti; } // [tested]

  /// Returns the value of the property defined by its path. Return value is invalid iff the path was invalid.
  virtual const xiiVariant GetValue(xiiStringView sProperty, xiiVariant index = xiiVariant(), xiiStatus* pRes = nullptr) const = 0;

  /// Sets a property defined by its path to the given value. Returns whether the operation was successful.
  virtual bool SetValue(xiiStringView sProperty, const xiiVariant& value, xiiVariant index = xiiVariant()) = 0;

  virtual xiiInt32 GetCount(xiiStringView sProperty) const                                       = 0;
  virtual bool     GetKeys(xiiStringView sProperty, xiiDynamicArray<xiiVariant>& out_keys) const = 0;

  virtual bool InsertValue(xiiStringView sProperty, xiiVariant index, const xiiVariant& value) = 0;
  virtual bool RemoveValue(xiiStringView sProperty, xiiVariant index)                          = 0;
  virtual bool MoveValue(xiiStringView sProperty, xiiVariant oldIndex, xiiVariant newIndex)    = 0;

  virtual xiiVariant GetPropertyChildIndex(xiiStringView sProperty, const xiiVariant& value) const = 0;

  const xiiDocumentObject* GetOwner() const { return m_pOwner; }

  bool GetValues(xiiStringView sProperty, xiiDynamicArray<xiiVariant>& out_values) const;


private:
  friend class xiiDocumentObjectManager;
  friend class xiiDocumentObject;

  const xiiRTTI*     m_pRtti;
  xiiDocumentObject* m_pOwner;
};
