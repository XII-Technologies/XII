/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Messages/EventMessage.h>
#include <GameEngine/GameEngineDLL.h>

struct XII_GAMEENGINE_DLL xiiMsgDamage : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgDamage, xiiEventMessage);

  double    m_fDamage = 0;
  xiiString m_sHitObjectName; ///< The actual game object that was hit (may be a child of the object to which the message is sent)

  xiiVec3 m_vGlobalPosition;  ///< The global position at which the damage was applied. Set to zero, if unused.
  xiiVec3 m_vImpactDirection; ///< The direction into which the damage was applied (e.g. direction of a projectile). May be zero.
};
