/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiStateMachineState;
class xiiStateMachineTransition;

/// A connection that represents a state machine transition. Since we can't chose different connection
/// types in the Editor we allow the user to switch the type in the properties.
class XII_SHAREDPLUGINASSETS_DLL xiiStateMachineConnection : public xiiDocumentObject_ConnectionBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineConnection, xiiDocumentObject_ConnectionBase);

public:
  xiiStateMachineTransition* m_pType = nullptr;
};

/// Base class for nodes in the state machine graph
class XII_SHAREDPLUGINASSETS_DLL xiiStateMachineNodeBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineNodeBase, xiiReflectedClass);
};

/// A node that represents a state machine state. We don't use xiiStateMachineState directly to allow
/// the user to switch the type in the properties similar to what we do with transitions.
class XII_SHAREDPLUGINASSETS_DLL xiiStateMachineNode : public xiiStateMachineNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineNode, xiiStateMachineNodeBase);

public:
  xiiString             m_sName;
  xiiStateMachineState* m_pType           = nullptr;
  bool                  m_bIsInitialState = false;
};

/// A node that represents "any" state machine state. This can be used if a transition with the same conditions
/// is possible from any other state in the state machine. Instead of creating many connections with the same properties
/// an "any" state can be used to make the graph much easier to read and to maintain.
///
/// Note that there is no "any" state at runtime but rather only the transition is stored.
class XII_SHAREDPLUGINASSETS_DLL xiiStateMachineNodeAny : public xiiStateMachineNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineNodeAny, xiiStateMachineNodeBase);
};
