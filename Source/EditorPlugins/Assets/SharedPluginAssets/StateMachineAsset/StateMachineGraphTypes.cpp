#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/StateMachineAsset/StateMachineGraphTypes.h>

#include <GameEngine/StateMachine/StateMachine.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineConnection, 1, xiiRTTIDefaultAllocator<xiiStateMachineConnection>)
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
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineNode, 1, xiiRTTIDefaultAllocator<xiiStateMachineNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute(xiiStringView("State"))), // wrap in xiiStringView to prevent a memory leak report
    XII_MEMBER_PROPERTY("Type", m_pType)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_MEMBER_PROPERTY("IsInitialState", m_bIsInitialState)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineNodeAny, 1, xiiRTTIDefaultAllocator<xiiStateMachineNodeAny>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
