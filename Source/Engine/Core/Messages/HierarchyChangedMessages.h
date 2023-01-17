#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct XII_CORE_DLL xiiMsgParentChanged : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgParentChanged, xiiMessage);

  enum class Type
  {
    ParentLinked,
    ParentUnlinked,
  };

  Type                m_Type;
  xiiGameObjectHandle m_hParent; // Previous or new parent, depending on m_Type
};

struct XII_CORE_DLL xiiMsgChildrenChanged : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgChildrenChanged, xiiMessage);

  enum class Type
  {
    ChildAdded,
    ChildRemoved
  };

  Type                m_Type;
  xiiGameObjectHandle m_hParent;
  xiiGameObjectHandle m_hChild;
};

struct XII_CORE_DLL xiiMsgComponentsChanged : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgComponentsChanged, xiiMessage);

  enum class Type
  {
    ComponentAdded,
    ComponentRemoved
  };

  Type                m_Type;
  xiiGameObjectHandle m_hOwner;
  xiiComponentHandle  m_hComponent;
};
