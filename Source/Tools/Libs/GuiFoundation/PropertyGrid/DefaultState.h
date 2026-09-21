/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Basics.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

class xiiDefaultStateProvider;
class xiiObjectAccessorBase;
class xiiDocumentObject;
class xiiAbstractProperty;

/// Registry for all xiiDefaultStateProvider factory functions.
class XII_GUIFOUNDATION_DLL xiiDefaultState
{
public:
  /// The functor interface for the xiiDefaultStateProvider factory function
  ///
  /// The return value is a sharedPtr as each implementation can decide whether to provide the same instance for all objects or whether a custom instance should be created for each object to allow for state caching (e.g. prefab root information). Returning nullptr is also valid for objects / containers for which the factory has no use (e.g. prefab default state provider on an object that does not belong to a prefab).
  /// The function is called for xiiDefaultObjectState usage with the pProp field left blank.
  /// For xiiDefaultContainerState usage pProp will point to the container property.
  using CreateStateProviderFunc = xiiSharedPtr<xiiDefaultStateProvider> (*)(xiiObjectAccessorBase*, const xiiDocumentObject*, const xiiAbstractProperty*);

  /// Registers a xiiDefaultStateProvider factory method. It is safe to register / unregister factories at any time.
  static void RegisterDefaultStateProvider(CreateStateProviderFunc func);
  /// Unregisters a xiiDefaultStateProvider factory method.
  static void UnregisterDefaultStateProvider(CreateStateProviderFunc func);

private:
  friend class xiiDefaultObjectState;
  friend class xiiDefaultContainerState;
  static xiiDynamicArray<CreateStateProviderFunc> s_Factories;
};

/// Object used to query and revert to the default state of all properties of an object.
///
/// This class should not be persisted in memory and just used on the stack to query all property states and then destroyed. It should also not be used across hierarchical changes of any kind (deleting objects etc).
class XII_GUIFOUNDATION_DLL xiiDefaultObjectState
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDefaultObjectState);

public:
  /// Constructor. Will collect the appropriate xiiDefaultStateProviders to query the states.
  /// \param pType The common base type of the selection.
  /// \param pAccessor Used to revert properties and query their current value.
  /// \param selection For which objects the default state should be queried. The xiiPropertySelection::m_Index should be invalid.
  xiiDefaultObjectState(const xiiRTTI* pType, xiiObjectAccessorBase* pAccessor, const xiiArrayPtr<xiiPropertySelection> selection);

  /// Returns the color of the top-most xiiDefaultStateProvider of the first element of the selection.
  xiiColorGammaUB GetBackgroundColor() const;
  /// Returns the name of the top-most xiiDefaultStateProvider of the first element of the selection.
  xiiString GetStateProviderName() const;

  bool       IsDefaultValue(xiiStringView sProperty) const;
  bool       IsDefaultValue(const xiiAbstractProperty* pProp) const;
  xiiStatus  RevertProperty(xiiStringView sProperty);
  xiiStatus  RevertProperty(const xiiAbstractProperty* pProp);
  xiiStatus  RevertObject();
  xiiVariant GetDefaultValue(xiiStringView sProperty, xiiUInt32 uiSelectionIndex = 0) const;
  xiiVariant GetDefaultValue(const xiiAbstractProperty* pProp, xiiUInt32 uiSelectionIndex = 0) const;

private:
  const xiiRTTI*                                                              m_pType     = nullptr;
  xiiObjectAccessorBase*                                                      m_pAccessor = nullptr;
  xiiArrayPtr<xiiPropertySelection>                                           m_Selection;
  xiiHybridArray<xiiHybridArray<xiiSharedPtr<xiiDefaultStateProvider>, 4>, 1> m_Providers;
};

/// Object used to query and revert to the default state of all elements of a container of an object.
///
/// This class should not be persisted in memory and just used on the stack to query all element states and then destroyed. It should also not be used across hierarchical changes of any kind (deleting objects etc).
class XII_GUIFOUNDATION_DLL xiiDefaultContainerState
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDefaultContainerState);

public:
  /// Constructor. Will collect the appropriate xiiDefaultStateProviders to query the states.
  /// \param pType The common base type of the selection.
  /// \param pAccessor Used to revert properties and query their current value.
  /// \param selection For which objects the default state should be queried. If xiiPropertySelection::m_Index is set, IsDefaultElement and RevertElement will query the value under that index if the passed in index is invalid.
  /// \param sProperty The name of the container for which default states should be queried.
  xiiDefaultContainerState(const xiiRTTI* pType, xiiObjectAccessorBase* pAccessor, const xiiArrayPtr<xiiPropertySelection> selection, xiiStringView sProperty);

  /// Returns the color of the top-most xiiDefaultStateProvider of the first element of the selection.
  /// \sa xiiDefaultStateProvider::GetBackgroundColor
  xiiColorGammaUB GetBackgroundColor() const;
  /// Returns the name of the top-most xiiDefaultStateProvider of the first element of the selection.
  /// \sa xiiDefaultStateProvider::GetStateProviderName
  xiiString GetStateProviderName() const;

  bool       IsDefaultElement(xiiVariant index) const;
  bool       IsDefaultContainer() const;
  xiiStatus  RevertElement(xiiVariant index);
  xiiStatus  RevertContainer();
  xiiVariant GetDefaultElement(xiiVariant index, xiiUInt32 uiSelectionIndex = 0) const;
  xiiVariant GetDefaultContainer(xiiUInt32 uiSelectionIndex = 0) const;

private:
  const xiiRTTI*                                                              m_pType     = nullptr;
  xiiObjectAccessorBase*                                                      m_pAccessor = nullptr;
  const xiiAbstractProperty*                                                  m_pProp     = nullptr;
  xiiArrayPtr<xiiPropertySelection>                                           m_Selection;
  xiiHybridArray<xiiHybridArray<xiiSharedPtr<xiiDefaultStateProvider>, 4>, 1> m_Providers;
};

/// Interface for querying and restoring the default state of objects and containers.
///
/// The high level functions IsDefaultValue, RevertProperty, RevertObjectContainer don't need to be overwritten in most cases. Instead, just implementing the pure virtual methods is enough.
class XII_GUIFOUNDATION_DLL xiiDefaultStateProvider : public xiiRefCounted
{
public:
  /// Parent hierarchy of state providers.
  ///
  /// xiiDefaultContainerState and xiiDefaultObjectState will build a hierarchy of parent default state providers depending on the root depth of all available providers (this is like virtual function overrides but with dynamic parent classes). If a provider can't handle a request, it should forward it to the first element in the superPtr array and pass in superPtr.GetSubArray(1) to that function call. Note that generally you don't need to check for validity of the ptr as the xiiAttributeDefaultStateProvider has root depth of -1 and will thus always be the last one in line.
  using SuperArray = const xiiArrayPtr<const xiiSharedPtr<xiiDefaultStateProvider>>;

  /// Returns the root depth of this provider instance.
  ///
  /// This is through how many properties and objects we needed to pass through from the object and property passed into the factory method to find the root object / property that this provider represents.
  /// For example if we have this object hierarchy:
  /// A
  /// |-children- B
  ///             |-elements- C
  ///
  /// If A is a prefab and the factory method was called for C (with no property) then we need to walk up the hierarchy via elements container, the B object, the children container and then finally A. Thus, we need 4 hops to get the the prefab root which means the root depth for this provider instance is 4.
  virtual xiiInt32 GetRootDepth() const = 0;

  /// Returns a color to be used in the property grid. Only the hue of the color is used. If alpha is 0, the color is ignored and no tinting of the property grid takes place.
  virtual xiiColorGammaUB GetBackgroundColor() const = 0;

  /// Returns the name of this state provider. Can be used to check what the outer most provider is for GUI purposes.
  virtual xiiString GetStateProviderName() const = 0;

  /// Returns the default value of an object's property at a given index.
  /// \param superPtr Parent hierarchy of inner providers that should be called of this instance cannot handle the request. See SuperArray definition for details.
  /// \param pAccessor Accessor to be used for querying object values if necessary. Always valid.
  /// \param pObject The object for which the default value should be queried. Always valid.
  /// \param pProp The property for which the default value should be queried. Always valid.
  /// \param index For containers: If the index is valid, the container element's default value is requested. If not, the entire container (either array or dictionary) is requested.
  /// \return The default value. xiiReflectionUtils::GetDefaultValue is a good example what is expected to be returned.
  /// \sa xiiReflectionUtils::GetDefaultValue, xiiDefaultStateProvider::DoesVariantMatchProperty
  virtual xiiVariant GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) = 0;

  /// Queries an array of diff operations that can be executed to revert the object container.
  /// \param superPtr superPtr Parent hierarchy of inner providers that should be called of this instance cannot handle the request. See SuperArray definition for details.
  /// \param pAccessor pAccessor Accessor to be used for querying object values if necessary. Always valid.
  /// \param pObject pObject The object which is to be reverted. Always valid.
  /// \param pProp pProp The container property which is to be reverted. Always valid.
  /// \param out_diff An array of diff operations that should be executed via xiiDocumentObjectConverterReader::ApplyDiffToObject to revert the object / container to its default state.
  /// \return If failure is returned, the operation failed and the undo transaction should be canceled.
  /// \sa xiiDocumentObjectConverterReader::ApplyDiffToObject
  virtual xiiStatus CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff) = 0;

public:
  virtual bool      IsDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant());
  virtual xiiStatus RevertProperty(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant());
  virtual xiiStatus RevertObjectContainer(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  /// A sanity check function that verifies that a given variant's value matches that expected of the property at the given index. If index is invalid and the property a container, the value must be an array or dictionary of the property's type.
  static bool DoesVariantMatchProperty(const xiiVariant& value, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant());
};
