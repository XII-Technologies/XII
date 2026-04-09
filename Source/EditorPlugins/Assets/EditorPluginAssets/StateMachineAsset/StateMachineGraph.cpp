#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <SharedPluginAssets/StateMachineAsset/StateMachineGraphTypes.h>

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

constexpr const char* s_szIsInitialState = "IsInitialState";

xiiStateMachineNodeManager::xiiStateMachineNodeManager()
{
  m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiStateMachineNodeManager::StructureEventHandler, this));
}

xiiStateMachineNodeManager::~xiiStateMachineNodeManager()
{
  m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiStateMachineNodeManager::StructureEventHandler, this));
}

bool xiiStateMachineNodeManager::IsInitialState(const xiiDocumentObject* pObject) const
{
  xiiVariant val = pObject->GetTypeAccessor().GetValue(s_szIsInitialState);
  if (val.IsValid())
  {
    return val.Get<bool>() == true;
  }

  return false;
}

const xiiDocumentObject* xiiStateMachineNodeManager::GetInitialState() const
{
  for (auto pObject : GetRootObject()->GetChildren())
  {
    if (IsNode(pObject) && IsInitialState(pObject))
    {
      return pObject;
    }
  }

  return nullptr;
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
  return XII_SUCCESS;
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

void xiiStateMachineNodeManager::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  if (IsNode(e.m_pObject) == false || IsAnyState(e.m_pObject))
    return;

  auto pCommandHistory = GetDocument()->GetCommandHistory();
  if (pCommandHistory == nullptr || pCommandHistory->IsInTransaction() == false)
    return;

  if (e.m_EventType == xiiDocumentObjectStructureEvent::Type::AfterObjectAdded &&
      e.m_pObject->GetTypeAccessor().GetValue(s_szIsInitialState) == false &&
      GetInitialState() == nullptr)
  {
    xiiSetObjectPropertyCommand propCmd;
    propCmd.m_Object    = e.m_pObject->GetGuid();
    propCmd.m_sProperty = s_szIsInitialState;
    propCmd.m_NewValue  = xiiVariant(true);

    XII_VERIFY(pCommandHistory->AddCommand(propCmd).Succeeded(), "");
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
    if (m_NewInitialStateObject.IsValid())
      m_pNewInitialStateObject = pManager->GetObject(m_NewInitialStateObject);

    if (auto pOldInitialStateObject = pManager->GetInitialState())
      m_pOldInitialStateObject = pManager->GetObject(pOldInitialStateObject->GetGuid());
  }

  if (m_pNewInitialStateObject)
    XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pNewInitialStateObject, s_szIsInitialState, xiiVariant(true)));

  if (m_pOldInitialStateObject)
    XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pOldInitialStateObject, s_szIsInitialState, xiiVariant(false)));

  return XII_SUCCESS;
}

xiiStatus xiiStateMachine_SetInitialStateCommand::UndoInternal(bool bFireEvents)
{
  xiiDocument* pDocument = GetDocument();
  auto         pManager  = static_cast<xiiStateMachineNodeManager*>(pDocument->GetObjectManager());

  if (m_pNewInitialStateObject)
    XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pNewInitialStateObject, s_szIsInitialState, xiiVariant(false)));

  if (m_pOldInitialStateObject)
    XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pOldInitialStateObject, s_szIsInitialState, xiiVariant(true)));

  return XII_SUCCESS;
}
