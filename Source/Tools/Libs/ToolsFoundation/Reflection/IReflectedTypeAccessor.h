#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

class xiiDocumentObject;
struct xiiStatus;

/// \brief Provides access to the properties of a xiiRTTI compatible data storage.
class XII_TOOLSFOUNDATION_DLL xiiIReflectedTypeAccessor
{
public:
  /// \brief Constructor for the xiiIReflectedTypeAccessor.
  ///
  /// It is a valid implementation to pass an invalid handle. Note that in this case there is no way to determine
  /// what is actually stored inside. However, it can be useful to use e.g. the xiiReflectedTypeDirectAccessor
  /// to set properties on the engine runtime side without having the xiiPhantomRttiManager initialized.
  xiiIReflectedTypeAccessor(const xiiRTTI* pRtti, xiiDocumentObject* pOwner) :
    m_pRtti(pRtti), m_pOwner(pOwner)
  {
  } // [tested]

  /// \brief Returns the xiiRTTI* of the wrapped instance type.
  const xiiRTTI* GetType() const { return m_pRtti; } // [tested]

  /// \brief Returns the value of the property defined by its path. Return value is invalid iff the path was invalid.
  virtual const xiiVariant GetValue(const char* szProperty, xiiVariant index = xiiVariant(), xiiStatus* pRes = nullptr) const = 0;

  /// \brief Sets a property defined by its path to the given value. Returns whether the operation was successful.
  virtual bool SetValue(const char* szProperty, const xiiVariant& value, xiiVariant index = xiiVariant()) = 0;

  virtual xiiInt32 GetCount(const char* szProperty) const                                       = 0;
  virtual bool     GetKeys(const char* szProperty, xiiDynamicArray<xiiVariant>& out_keys) const = 0;

  virtual bool InsertValue(const char* szProperty, xiiVariant index, const xiiVariant& value) = 0;
  virtual bool RemoveValue(const char* szProperty, xiiVariant index)                          = 0;
  virtual bool MoveValue(const char* szProperty, xiiVariant oldIndex, xiiVariant newIndex)    = 0;

  virtual xiiVariant GetPropertyChildIndex(const char* szProperty, const xiiVariant& value) const = 0;

  const xiiDocumentObject* GetOwner() const { return m_pOwner; }

  bool GetValues(const char* szProperty, xiiDynamicArray<xiiVariant>& out_values) const;


private:
  friend class xiiDocumentObjectManager;
  friend class xiiDocumentObject;

  const xiiRTTI*     m_pRtti;
  xiiDocumentObject* m_pOwner;
};
