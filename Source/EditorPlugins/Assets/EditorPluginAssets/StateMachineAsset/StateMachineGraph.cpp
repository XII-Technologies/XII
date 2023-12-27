#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <GameEngine/StateMachine/StateMachine.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, StateMachine)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiQtNodeScene::GetPinFactory().RegisterCreator(xiiGetStaticRTTI<xiiStateMachinePin>(), [](const xiiRTTI* pRtti)->xiiQtPin* { return new xiiQtStateMachinePin(); });
    xiiQtNodeScene::GetConnectionFactory().RegisterCreator(xiiGetStaticRTTI<xiiStateMachineConnection>(), [](const xiiRTTI* pRtti)->xiiQtConnection* { return new xiiQtStateMachineConnection(); });    
    xiiQtNodeScene::GetNodeFactory().RegisterCreator(xiiGetStaticRTTI<xiiStateMachineNodeBase>(), [](const xiiRTTI* pRtti)->xiiQtNode* { return new xiiQtStateMachineNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiQtNodeScene::GetPinFactory().UnregisterCreator(xiiGetStaticRTTI<xiiStateMachinePin>());
    xiiQtNodeScene::GetConnectionFactory().UnregisterCreator(xiiGetStaticRTTI<xiiStateMachineConnection>());
    xiiQtNodeScene::GetNodeFactory().UnregisterCreator(xiiGetStaticRTTI<xiiStateMachineNodeBase>());
  }

XII_END_SUBSYSTEM_DECLARATION;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachinePin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachinePin::xiiStateMachinePin(Type type, const xiiDocumentObject* pObject) :
  xiiPin(type, type == Type::Input ? "Enter" : "Exit", xiiColor::Grey, pObject)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineConnection, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_pType)->AddFlags(xiiPropertyFlags::PointerOwner)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineNodeBase, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineNode, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute(xiiStringView("State"))), // wrap in xiiStringView to prevent a memory leak report
    XII_MEMBER_PROPERTY("Type", m_pType)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineNodeAny, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

xiiStateMachineNodeManager::xiiStateMachineNodeManager()
{
  m_ObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiStateMachineNodeManager::ObjectHandler, this));
}

xiiStateMachineNodeManager::~xiiStateMachineNodeManager()
{
  m_ObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiStateMachineNodeManager::ObjectHandler, this));
}

void xiiStateMachineNodeManager::SetInitialState(const xiiDocumentObject* pObject)
{
  if (m_pInitialStateObject == pObject)
    return;

  XII_ASSERT_DEV(IsAnyState(pObject) == false, "'Any State' can't be initial state");

  auto BroadcastEvent = [this](const xiiDocumentObject* pObject) {
    if (pObject != nullptr)
    {
      xiiDocumentObjectPropertyEvent e;
      e.m_EventType = xiiDocumentObjectPropertyEvent::Type::PropertySet;
      e.m_pObject   = pObject;

      m_PropertyEvents.Broadcast(e);
    }
  };

  const xiiDocumentObject* pOldInitialStateObject = m_pInitialStateObject;
  m_pInitialStateObject                           = pObject;

  // Broadcast after the initial state object has been changed since the qt node will query it from the manager
  BroadcastEvent(pOldInitialStateObject);
  BroadcastEvent(m_pInitialStateObject);
}

bool xiiStateMachineNodeManager::IsAnyState(const xiiDocumentObject* pObject) const
{
  if (pObject != nullptr)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    return pType->IsDerivedFrom<xiiStateMachineNodeAny>();
  }
  return false;
}

bool xiiStateMachineNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  if (pObject != nullptr)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    return pType->IsDerivedFrom<xiiStateMachineNodeBase>();
  }
  return false;
}

xiiStatus xiiStateMachineNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNtoN;
  return xiiStatus(XII_SUCCESS);
}

void xiiStateMachineNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node)
{
  if (IsNode(pObject) == false)
    return;

  if (IsAnyState(pObject) == false)
  {
    auto pPin = XII_DEFAULT_NEW(xiiStateMachinePin, xiiPin::Type::Input, pObject);
    node.m_Inputs.PushBack(pPin);
  }

  {
    auto pPin = XII_DEFAULT_NEW(xiiStateMachinePin, xiiPin::Type::Output, pObject);
    node.m_Outputs.PushBack(pPin);
  }
}

void xiiStateMachineNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const
{
  Types.PushBack(xiiGetStaticRTTI<xiiStateMachineNode>());
  Types.PushBack(xiiGetStaticRTTI<xiiStateMachineNodeAny>());
}

const xiiRTTI* xiiStateMachineNodeManager::GetConnectionType() const
{
  return xiiGetStaticRTTI<xiiStateMachineConnection>();
}

void xiiStateMachineNodeManager::ObjectHandler(const xiiDocumentObjectEvent& e)
{
  if (e.m_EventType == xiiDocumentObjectEvent::Type::AfterObjectCreated && IsNode(e.m_pObject))
  {
    if (m_pInitialStateObject == nullptr && IsAnyState(e.m_pObject) == false)
    {
      SetInitialState(e.m_pObject);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachine_SetInitialStateCommand, 1, xiiRTTIDefaultAllocator<xiiStateMachine_SetInitialStateCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("NewInitialStateObject", m_NewInitialStateObject),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachine_SetInitialStateCommand::xiiStateMachine_SetInitialStateCommand() = default;

xiiStatus xiiStateMachine_SetInitialStateCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();
  auto         pManager  = static_cast<xiiStateMachineNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pNewInitialStateObject = pManager->GetObject(m_NewInitialStateObject);
    m_pOldInitialStateObject = pManager->GetInitialState();
  }

  pManager->SetInitialState(m_pNewInitialStateObject);
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiStateMachine_SetInitialStateCommand::UndoInternal(bool bFireEvents)
{
  xiiDocument* pDocument = GetDocument();
  auto         pManager  = static_cast<xiiStateMachineNodeManager*>(pDocument->GetObjectManager());

  pManager->SetInitialState(m_pOldInitialStateObject);
  return xiiStatus(XII_SUCCESS);
}
