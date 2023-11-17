#include <Core/CorePCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgCollision);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgCollision, 1, xiiRTTIDefaultAllocator<xiiMsgCollision>)
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTriggerState, 1)
  XII_ENUM_CONSTANTS(xiiTriggerState::Activated, xiiTriggerState::Continuing, xiiTriggerState::Deactivated)
XII_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgDeleteGameObject);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgDeleteGameObject, 1, xiiRTTIDefaultAllocator<xiiMsgDeleteGameObject>)
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgComponentInternalTrigger);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgComponentInternalTrigger, 1, xiiRTTIDefaultAllocator<xiiMsgComponentInternalTrigger>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Message", m_sMessage),
    XII_MEMBER_PROPERTY("Payload", m_iPayload),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgUpdateLocalBounds);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgUpdateLocalBounds, 1, xiiRTTIDefaultAllocator<xiiMsgUpdateLocalBounds>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgSetPlaying);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgSetPlaying, 1, xiiRTTIDefaultAllocator<xiiMsgSetPlaying>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Play", m_bPlay)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgParentChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgParentChanged, 1, xiiRTTIDefaultAllocator<xiiMsgParentChanged>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgChildrenChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgChildrenChanged, 1, xiiRTTIDefaultAllocator<xiiMsgChildrenChanged>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgComponentsChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgComponentsChanged, 1, xiiRTTIDefaultAllocator<xiiMsgComponentsChanged>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgTransformChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgTransformChanged, 1, xiiRTTIDefaultAllocator<xiiMsgTransformChanged>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgSetFloatParameter);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgSetFloatParameter, 1, xiiRTTIDefaultAllocator<xiiMsgSetFloatParameter>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sParameterName),
    XII_MEMBER_PROPERTY("Value", m_fValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgSetDoubleParameter);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgSetDoubleParameter, 1, xiiRTTIDefaultAllocator<xiiMsgSetDoubleParameter>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sParameterName),
    XII_MEMBER_PROPERTY("Value", m_fValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgSetRealParameter);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgSetRealParameter, 1, xiiRTTIDefaultAllocator<xiiMsgSetRealParameter>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sParameterName),
    XII_MEMBER_PROPERTY("Value", m_fValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgGenericEvent);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgGenericEvent, 1, xiiRTTIDefaultAllocator<xiiMsgGenericEvent>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Message", m_sMessage),
    XII_MEMBER_PROPERTY("Value", m_Value)->AddAttributes(new xiiDefaultValueAttribute(0))
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgAnimationReachedEnd);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgAnimationReachedEnd, 1, xiiRTTIDefaultAllocator<xiiMsgAnimationReachedEnd>)
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgTriggerTriggered)
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgTriggerTriggered, 1, xiiRTTIDefaultAllocator<xiiMsgTriggerTriggered>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Message", m_sMessage),
    XII_ENUM_MEMBER_PROPERTY("TriggerState", xiiTriggerState, m_TriggerState),
    XII_MEMBER_PROPERTY("GameObject", m_hTriggeringObject),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

// clang-format on

XII_STATICLINK_FILE(Core, Core_Messages_Implementation_Messages);
