/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/RefCounted.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

/// Describes the current meta state of a property for display purposes in the property grid
struct xiiPropertyUiState
{
  enum Visibility
  {
    Default,   ///< Displayed normally, for editing (unless the property is read-only)
    Invisible, ///< Hides the property entirely
    Disabled,  ///< The property is shown but disabled, when multiple objects are selected and in one the property is invisible, in the other it is
               ///< disabled, the disabled state takes precedence
  };

  xiiPropertyUiState()
  {
    m_Visibility = Visibility::Default;
  }

  Visibility m_Visibility;
  xiiString  m_sNewLabelText;
};

/// Event that is broadcast whenever information about how to present properties is required
struct xiiPropertyMetaStateEvent
{
  /// The object for which the information is queried
  const xiiDocumentObject* m_pObject = nullptr;

  /// The map into which event handlers should write their information about the state of each property.
  /// The string is the property name that identifies the property in m_pObject.
  xiiMap<xiiString, xiiPropertyUiState>* m_pPropertyStates = nullptr;
};

/// Event that is broadcast whenever information about how to present elements in a container is required
struct xiiContainerElementMetaStateEvent
{
  /// The object for which the information is queried
  const xiiDocumentObject* m_pObject = nullptr;
  /// The Container property
  xiiStringView m_sProperty;
  /// The map into which event handlers should write their information about the state of each container element.
  /// The xiiVariant should be the key of the container element, either xiiUInt32 for arrays and sets or xiiString for maps.
  xiiHashTable<xiiVariant, xiiPropertyUiState>* m_pContainerElementStates = nullptr;
};

/// This class allows to query additional information about how to present properties in the property grid
///
/// The property grid calls GetTypePropertiesState() and GetContainerElementsState() with the current selection of xiiDocumentObject's.
/// This triggers the xiiPropertyMetaStateEvent to be broadcast, which allows for other code to determine additional
/// information for the properties and write it into the event data.
class XII_GUIFOUNDATION_DLL xiiPropertyMetaState
{
  XII_DECLARE_SINGLETON(xiiPropertyMetaState);

public:
  xiiPropertyMetaState();

  /// Queries the property meta state for a single xiiDocumentObject
  void GetTypePropertiesState(const xiiDocumentObject* pObject, xiiMap<xiiString, xiiPropertyUiState>& out_propertyStates);

  /// Queries the property meta state for a multi selection of xiiDocumentObject's
  ///
  /// This will query the information for every single selected object and then merge the result into one.
  void GetTypePropertiesState(const xiiHybridArray<xiiPropertySelection, 8>& items, xiiMap<xiiString, xiiPropertyUiState>& out_propertyStates);

  /// Queries the meta state for the elements of a single container property on one xiiDocumentObject.
  void GetContainerElementsState(const xiiDocumentObject* pObject, xiiStringView sProperty, xiiHashTable<xiiVariant, xiiPropertyUiState>& out_propertyStates);

  /// Queries the meta state for the elements of a single container property on a multi selection of xiiDocumentObjects.
  ///
  /// This will query the information for every single selected object and then merge the result into one.
  void GetContainerElementsState(const xiiHybridArray<xiiPropertySelection, 8>& items, xiiStringView sProperty, xiiHashTable<xiiVariant, xiiPropertyUiState>& out_propertyStates);

  /// Attach to this event to get notified of property state queries.
  /// Add information to xiiPropertyMetaStateEvent::m_pPropertyStates to return data.
  xiiEvent<xiiPropertyMetaStateEvent&> m_Events;
  /// Attach to this event to get notified of container element state queries.
  /// Add information to xiiContainerElementMetaStateEvent::m_pContainerElementStates to return data.
  xiiEvent<xiiContainerElementMetaStateEvent&> m_ContainerEvents;

private:
  xiiMap<xiiString, xiiPropertyUiState>        m_Temp;
  xiiHashTable<xiiVariant, xiiPropertyUiState> m_Temp2;
};
