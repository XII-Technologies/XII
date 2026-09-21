/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Types/Uuid.h>

/// Describes the initial state of a game object.
struct XII_CORE_DLL xiiGameObjectDescription
{
  XII_DECLARE_POD_TYPE();

  bool      m_bActiveFlag = true;  ///< Whether the object should have the 'active flag' set. See xiiGameObject::SetActiveFlag().
  bool      m_bDynamic    = false; ///< Whether the object should start out as 'dynamic'. See xiiGameObject::MakeDynamic().
  xiiUInt16 m_uiTeamID    = 0;     ///< See xiiGameObject::GetTeamID().

  xiiHashedString     m_sName;   ///< See xiiGameObject::SetName().
  xiiGameObjectHandle m_hParent; ///< An optional parent object to attach this object to as a child.

  xiiVec3   m_LocalPosition       = xiiVec3::MakeZero();     ///< The local position relative to the parent (or the world)
  xiiQuat   m_LocalRotation       = xiiQuat::MakeIdentity(); ///< The local rotation relative to the parent (or the world)
  xiiVec3   m_LocalScaling        = xiiVec3(1);              ///< The local scaling relative to the parent (or the world)
  float     m_LocalUniformScaling = 1.0f;                    ///< An additional local uniform scaling relative to the parent (or the world)
  xiiTagSet m_Tags;                                          ///< See xiiGameObject::GetTags()
  xiiUInt32 m_uiStableRandomSeed = 0xFFFFFFFF;               ///< 0 means the game object gets a random value assigned, 0xFFFFFFFF means that if the object has a parent, the value will be derived deterministically from that one's seed, otherwise it gets a random value, any other value will be used directly
};
