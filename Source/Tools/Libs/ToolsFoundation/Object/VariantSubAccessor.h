/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class xiiDocumentObject;

/// Accessor for a sub-tree on an xiiVariant property.
/// The tools foundation code uses an xiiDocumentObject, one of its xiiAbstractProperty and an optional xiiVariant index to reference to properties. Any deeper hierarchies must be built from additional objects. This principle prevents the GUI to reference anything inside an xiiVariant that stores an VariantArray or VariantDictionary as xiiVariant is a pure value type and cannot store additional objects on the tool side. To work around this, this class creates a view one level deeper into an xiiVariant. This is done by calling SetSubItems which for each object in the map moves the view into the sub-tree referenced by the given value of the map.
class XII_TOOLSFOUNDATION_DLL xiiVariantSubAccessor : public xiiObjectProxyAccessor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVariantSubAccessor, xiiObjectProxyAccessor);

public:
  /// Constructor
  /// \param pSource The original accessor that is going to be proxied. By chaining this class an xiiVariant can be explored deeper and deeper.
  /// \param pProp The xiiVariant property that is going to be proxied. Only this property is allowed to be accessed by the accessor functions.
  xiiVariantSubAccessor(xiiObjectAccessorBase* pSource, const xiiAbstractProperty* pProp);
  /// Sets the sub-tree indices for the selected objects.
  /// \param subItemMap Object to index map. Note that as this is in the ToolsFoundation it cannot use the xiiPropertySelection class.
  void SetSubItems(const xiiMap<const xiiDocumentObject*, xiiVariant>& subItemMap);
  /// Returns the property this accessor wraps.
  const xiiAbstractProperty* GetRootProperty() const { return m_pProp; }
  /// How many level deep the view is inside the property.
  xiiInt32 GetDepth() const;
  /// Builds a path up the hierarchy of wrapped xiiVariantSubAccessor objects to determine the path to the current sub-tree of the xiiVariant.
  /// \param pObject The object for which the path should be computed
  /// \param out_path An array of indices that has to be followed from the root of the xiiVariant to each the current sub-tree view.
  /// \return Returns XII_FAILURE if pObject is not known.
  xiiResult GetPath(const xiiDocumentObject* pObject, xiiDynamicArray<xiiVariant>& out_path) const;

  virtual xiiStatus GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex) override;
  virtual xiiStatus GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount) override;
  virtual xiiStatus GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys) override;
  virtual xiiStatus GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values) override;

private:
  xiiStatus GetSubValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value);
  xiiStatus SetSubValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiDelegate<xiiStatus(xiiVariant& subValue)>& func);

private:
  const xiiAbstractProperty*                   m_pProp = nullptr;
  xiiMap<const xiiDocumentObject*, xiiVariant> m_SubItemMap;
};
