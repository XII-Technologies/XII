#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct XII_CORE_DLL xiiMsgCollision : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgCollision, xiiMessage);

  xiiGameObjectHandle m_hObjectA;
  xiiGameObjectHandle m_hObjectB;

  xiiComponentHandle m_hComponentA;
  xiiComponentHandle m_hComponentB;

  xiiVec3Real m_vPosition; ///< The collision position in world space.
  xiiVec3Real m_vNormal;   ///< The collision normal on the surface of object B.
  xiiVec3Real m_vImpulse;  ///< The collision impulse applied from object A to object B.
};
