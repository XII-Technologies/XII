/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct xiiStatus;

/// Helper class to modify an xiiVariant as if it was a container.
/// GetValue and SetValue are valid for all variant types.
/// The remaining accessor functions require a VariantArray or VariantDictionary type.
/// Array indices must be finite, nonnegative integers within the container's bounds.
/// InsertValue additionally accepts -1 to append; insertion and movement accept the end position.
/// Failed operations preserve the stored value. GetKeys replaces its output, including on failure.
class XII_TOOLSFOUNDATION_DLL xiiVariantStorageAccessor
{
public:
  xiiVariantStorageAccessor(xiiStringView sProperty, xiiVariant& value);
  xiiVariantStorageAccessor(xiiStringView sProperty, const xiiVariant& value);

  xiiVariant GetValue(xiiVariant index = xiiVariant(), xiiStatus* pRes = nullptr) const;
  xiiStatus  SetValue(const xiiVariant& value, xiiVariant index = xiiVariant());

  xiiInt32  GetCount() const;
  xiiStatus GetKeys(xiiDynamicArray<xiiVariant>& out_keys) const;
  xiiStatus InsertValue(const xiiVariant& index, const xiiVariant& value);
  xiiStatus RemoveValue(const xiiVariant& index);
  xiiStatus MoveValue(const xiiVariant& oldIndex, const xiiVariant& newIndex);

private:
  xiiStringView m_sProperty;
  xiiVariant&   m_Value;
};
