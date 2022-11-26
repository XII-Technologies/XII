#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/ArrayPtr.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct XII_PARTICLEPLUGIN_DLL xiiParticleEvent
{
  XII_DECLARE_POD_TYPE();

  xiiTempHashedString m_EventType;
  xiiVec3             m_vPosition;
  xiiVec3             m_vDirection;
  xiiVec3             m_vNormal;
};

typedef xiiArrayPtr<xiiParticleEvent> xiiParticleEventQueue;
