/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>

/// A xiiIReflectedTypeAccessor implementation that also stores the actual data that is defined in the passed xiiRTTI.
///
/// This class is used to store data on the tool side for classes that are not known to the tool but exist outside of it
/// like engine components. As this is basically a complex value map the used type can be hot-reloaded. For this, the
/// xiiRTTI just needs to be updated with its new definition in the xiiPhantomRttiManager and all xiiReflectedTypeStorageAccessor
/// will be automatically rearranged to match the new class layout.
class XII_TOOLSFOUNDATION_DLL xiiReflectedTypeStorageAccessor : public xiiIReflectedTypeAccessor
{
  friend class xiiReflectedTypeStorageManager;

public:
  xiiReflectedTypeStorageAccessor(const xiiRTTI* pReflectedType, xiiDocumentObject* pOwner); // [tested]
  ~xiiReflectedTypeStorageAccessor();

  const xiiVariant GetValue(xiiStringView sProperty, xiiVariant index = xiiVariant(), xiiStatus* pRes = nullptr) const override; // [tested]
  bool             SetValue(xiiStringView sProperty, const xiiVariant& value, xiiVariant index = xiiVariant()) override;         // [tested]

  xiiInt32 GetCount(xiiStringView sProperty) const override;
  bool     GetKeys(xiiStringView sProperty, xiiDynamicArray<xiiVariant>& out_keys) const override;

  bool InsertValue(xiiStringView sProperty, xiiVariant index, const xiiVariant& value) override;
  bool RemoveValue(xiiStringView sProperty, xiiVariant index) override;
  bool MoveValue(xiiStringView sProperty, xiiVariant oldIndex, xiiVariant newIndex) override;

  xiiVariant GetPropertyChildIndex(xiiStringView sProperty, const xiiVariant& value) const override;

private:
  xiiDynamicArray<xiiVariant>                                        m_Data;
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping* m_pMapping;
};
