/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class xiiIReflectedTypeAccessor;
class xiiDocumentObject;
class xiiAbstractObjectGraph;

/// Helper functions for handling reflection related operations.
///
/// Also check out xiiToolsSerializationUtils for related functionality.
class XII_TOOLSFOUNDATION_DLL xiiToolsReflectionUtils
{
public:
  /// Returns the type under which the property is stored on the editor side.
  static xiiVariantType::Enum GetStorageType(const xiiAbstractProperty* pProperty);

  /// Returns the default value for the entire property as it is stored on the editor side.
  static xiiVariant GetStorageDefault(const xiiAbstractProperty* pProperty);

  static bool GetFloatFromVariant(const xiiVariant& val, double& out_fValue);
  static bool GetVariantFromFloat(double fValue, xiiVariantType::Enum type, xiiVariant& out_val);

  /// Creates a ReflectedTypeDescriptor from a xiiRTTI instance that can be serialized and registered at the xiiPhantomRttiManager.
  static void GetReflectedTypeDescriptorFromRtti(const xiiRTTI* pRtti, xiiReflectedTypeDescriptor& out_desc); // [tested]
  static void GetMinimalReflectedTypeDescriptorFromRtti(const xiiRTTI* pRtti, xiiReflectedTypeDescriptor& out_desc);

  static void GatherObjectTypes(const xiiDocumentObject* pObject, xiiSet<const xiiRTTI*>& inout_types);

  static bool DependencySortTypeDescriptorArray(xiiDynamicArray<xiiReflectedTypeDescriptor*>& ref_descriptors);
};
