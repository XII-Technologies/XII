#include <Core/CorePCH.h>

#include <Core/Messages/ApplyOnlyToMessage.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgOnlyApplyToObject);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgOnlyApplyToObject, 1, xiiRTTIDefaultAllocator<xiiMsgOnlyApplyToObject>)
{
  ///\todo enable this once we have object reference properties
  /*XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Object", m_hObject),
  }
  XII_END_PROPERTIES;*/
  XII_BEGIN_ATTRIBUTES
  {
    new xiiAutoGenVisScriptMsgSender
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


XII_STATICLINK_FILE(Core, Core_Messages_Implementation_ApplyOnlyToMessage);
