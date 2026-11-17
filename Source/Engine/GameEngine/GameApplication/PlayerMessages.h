/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

/// Managed Player status on the application/editor thread.
class XII_GAMEENGINE_DLL xiiPlayerStatusMessage : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlayerStatusMessage, xiiReflectedClass);

public:
  xiiString m_sInstance;
  xiiString m_sStage;
  xiiString m_sDetail;
  xiiUInt64 m_uiSceneRevision = 0;
  float     m_fProgress       = -1.0F; ///< Negative is indeterminate (IO/initialization).
};

class XII_GAMEENGINE_DLL xiiPlayerStopMessage : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlayerStopMessage, xiiReflectedClass);

public:
  xiiString m_sInstance;
};

/// A runtime transition request for a single owned instance. Cancellation preserves its active world.
class XII_GAMEENGINE_DLL xiiPlayerSceneMessage : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlayerSceneMessage, xiiReflectedClass);

public:
  xiiString m_sInstance;
  xiiString m_sScene;
  xiiString m_sPreloadCollection;
  bool      m_bCancel = false;
};
