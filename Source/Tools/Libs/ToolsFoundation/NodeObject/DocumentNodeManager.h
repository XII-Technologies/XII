/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiPin;
class xiiConnection;

struct XII_TOOLSFOUNDATION_DLL xiiDocumentNodeManagerEvent
{
  enum class Type
  {
    NodeMoved,
    AfterPinsConnected,
    BeforePinsDisonnected,
    BeforePinsChanged,
    AfterPinsChanged,
    BeforeNodeAdded,
    AfterNodeAdded,
    BeforeNodeRemoved,
    AfterNodeRemoved,
  };

  xiiDocumentNodeManagerEvent(Type eventType, const xiiDocumentObject* pObject = nullptr) :
    m_EventType(eventType), m_pObject(pObject)
  {
  }

  Type                     m_EventType;
  const xiiDocumentObject* m_pObject;
};

class xiiConnection
{
public:
  const xiiPin&            GetSourcePin() const { return m_SourcePin; }
  const xiiPin&            GetTargetPin() const { return m_TargetPin; }
  const xiiDocumentObject* GetParent() const { return m_pParent; }

private:
  friend class xiiDocumentNodeManager;

  xiiConnection(const xiiPin& sourcePin, const xiiPin& targetPin, const xiiDocumentObject* pParent) :
    m_SourcePin(sourcePin), m_TargetPin(targetPin), m_pParent(pParent)
  {
  }

  const xiiPin&            m_SourcePin;
  const xiiPin&            m_TargetPin;
  const xiiDocumentObject* m_pParent = nullptr;
};

class XII_TOOLSFOUNDATION_DLL xiiPin : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPin, xiiReflectedClass);

public:
  enum class Type
  {
    Input,
    Output
  };

  enum class Shape
  {
    Circle,
    Rect,
    RoundRect,
    Arrow,
    Default = Circle
  };

  xiiPin(Type type, xiiStringView sName, const xiiColorGammaUB& color, const xiiDocumentObject* pObject) :
    m_Type(type), m_Color(color), m_sName(sName), m_pParent(pObject)
  {
  }

  Shape m_Shape = Shape::Default;

  Type                     GetType() const { return m_Type; }
  xiiStringView            GetName() const { return m_sName; }
  const xiiColorGammaUB&   GetColor() const { return m_Color; }
  const xiiDocumentObject* GetParent() const { return m_pParent; }

private:
  friend class xiiDocumentNodeManager;

  Type                     m_Type;
  xiiColorGammaUB          m_Color;
  xiiString                m_sName;
  const xiiDocumentObject* m_pParent = nullptr;
};

//////////////////////////////////////////////////////////////////////////

struct xiiNodePropertyValue
{
  xiiHashedString m_sPropertyName;
  xiiVariant      m_Value;
};

/// Describes a template that will be used to create new nodes. In most cases this only contains the type
/// but it can also contain properties that are pre-filled when the node is created.
///
/// For example in visual script this allows us to have one generic node type for setting reflected properties
/// but we can expose all relevant reflected properties in the node creation menu so the user does not need to fill out the property name manually.
struct xiiNodeCreationTemplate
{
  const xiiRTTI*                          m_pType = nullptr;
  xiiStringView                           m_sTypeName;
  xiiHashedString                         m_sCategory;
  xiiArrayPtr<const xiiNodePropertyValue> m_PropertyValues;
};

//////////////////////////////////////////////////////////////////////////

/// Base class for all node connections. Derive from this class and overwrite xiiDocumentNodeManager::GetConnectionType
/// if you need custom properties for connections.
class XII_TOOLSFOUNDATION_DLL xiiDocumentObject_ConnectionBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentObject_ConnectionBase, xiiReflectedClass);

public:
  xiiUuid   m_Source;
  xiiUuid   m_Target;
  xiiString m_SourcePin;
  xiiString m_TargetPin;
};

//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiDocumentNodeManager : public xiiDocumentObjectManager
{
public:
  xiiEvent<const xiiDocumentNodeManagerEvent&> m_NodeEvents;

  xiiDocumentNodeManager();
  virtual ~xiiDocumentNodeManager();

  /// For node documents this function is called instead of GetCreateableTypes to get a list for the node creation menu.
  ///
  /// \see xiiNodeCreationTemplate
  virtual void GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const;

  virtual const xiiRTTI* GetConnectionType() const;

  xiiVec2              GetNodePos(const xiiDocumentObject* pObject) const;
  const xiiConnection& GetConnection(const xiiDocumentObject* pObject) const;
  const xiiConnection* GetConnectionIfExists(const xiiDocumentObject* pObject) const;

  const xiiPin*                                 GetInputPinByName(const xiiDocumentObject* pObject, xiiStringView sName) const;
  const xiiPin*                                 GetOutputPinByName(const xiiDocumentObject* pObject, xiiStringView sName) const;
  xiiArrayPtr<const xiiUniquePtr<const xiiPin>> GetInputPins(const xiiDocumentObject* pObject) const;
  xiiArrayPtr<const xiiUniquePtr<const xiiPin>> GetOutputPins(const xiiDocumentObject* pObject) const;

  enum class CanConnectResult
  {
    ConnectNever, ///< Pins can't be connected
    Connect1to1,  ///< Output pin can have 1 outgoing connection, Input pin can have 1 incoming connection
    Connect1toN,  ///< Output pin can have 1 outgoing connection, Input pin can have N incoming connections
    ConnectNto1,  ///< Output pin can have N outgoing connections, Input pin can have 1 incoming connection
    ConnectNtoN,  ///< Output pin can have N outgoing connections, Input pin can have N incoming connections
  };

  bool IsNode(const xiiDocumentObject* pObject) const;
  bool IsConnection(const xiiDocumentObject* pObject) const;
  bool IsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const;

  xiiArrayPtr<const xiiConnection* const> GetConnections(const xiiPin& pin) const;
  bool                                    HasConnections(const xiiPin& pin) const;
  bool                                    IsConnected(const xiiPin& source, const xiiPin& target) const;

  xiiStatus CanConnect(const xiiRTTI* pObjectType, const xiiPin& source, const xiiPin& target, CanConnectResult& ref_result) const;
  xiiStatus CanDisconnect(const xiiConnection* pConnection) const;
  xiiStatus CanDisconnect(const xiiDocumentObject* pObject) const;
  xiiStatus CanMoveNode(const xiiDocumentObject* pObject, const xiiVec2& vPos) const;

  void Connect(const xiiDocumentObject* pObject, const xiiPin& source, const xiiPin& target);
  void Disconnect(const xiiDocumentObject* pObject);
  void MoveNode(const xiiDocumentObject* pObject, const xiiVec2& vPos);

  void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& ref_graph) const;
  void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable);

  void GetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const;
  bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph) const;
  bool PasteObjects(const xiiArrayPtr<xiiDocument::PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, const xiiVec2& vPickedPosition, bool bAllowPickedPosition);

protected:
  /// Tests whether pTarget can be reached from pSource by following the pin connections
  bool CanReachNode(const xiiDocumentObject* pSource, const xiiDocumentObject* pTarget, xiiSet<const xiiDocumentObject*>& Visited) const;

  /// Returns true if adding a connection between the two pins would create a circular graph
  bool WouldConnectionCreateCircle(const xiiPin& source, const xiiPin& target) const;

  xiiResult ResolveConnection(const xiiUuid& sourceObject, const xiiUuid& targetObject, xiiStringView sourcePin, xiiStringView targetPin, const xiiPin*& out_pSourcePin, const xiiPin*& out_pTargetPin) const;

  virtual void GetDynamicPinNames(const xiiDocumentObject* pObject, xiiStringView sPropertyName, xiiStringView sPinName, xiiDynamicArray<xiiString>& out_Names) const;
  virtual bool TryRecreatePins(const xiiDocumentObject* pObject);

  struct NodeInternal
  {
    xiiVec2                                 m_vPos = xiiVec2::MakeZero();
    xiiHybridArray<xiiUniquePtr<xiiPin>, 6> m_Inputs;
    xiiHybridArray<xiiUniquePtr<xiiPin>, 6> m_Outputs;
  };

private:
  virtual bool      InternalIsNode(const xiiDocumentObject* pObject) const;
  virtual bool      InternalIsConnection(const xiiDocumentObject* pObject) const;
  virtual bool      InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const { return false; }
  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const;
  virtual xiiStatus InternalCanDisconnect(const xiiPin& source, const xiiPin& target) const { return XII_SUCCESS; }
  virtual xiiStatus InternalCanMoveNode(const xiiDocumentObject* pObject, const xiiVec2& vPos) const { return XII_SUCCESS; }
  virtual void      InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node) = 0;

  void ObjectHandler(const xiiDocumentObjectEvent& e);
  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void PropertyEventsHandler(const xiiDocumentObjectPropertyEvent& e);

  void HandlePotentialDynamicPinPropertyChanged(const xiiDocumentObject* pObject, xiiStringView sPropertyName);

private:
  xiiHashTable<xiiUuid, NodeInternal>                            m_ObjectToNode;
  xiiHashTable<xiiUuid, xiiUniquePtr<xiiConnection>>             m_ObjectToConnection;
  xiiMap<const xiiPin*, xiiHybridArray<const xiiConnection*, 6>> m_Connections;
};
