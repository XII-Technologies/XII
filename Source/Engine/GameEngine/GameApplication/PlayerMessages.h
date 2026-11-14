/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Strings/String.h>

/// Private per-instance pipe; messages are pumped on the application/editor thread.
class XII_GAMEENGINE_DLL xiiPlayerStatusMessage : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlayerStatusMessage, xiiProcessMessage);

public:
  xiiString m_sInstance;
  xiiString m_sStage;
  xiiString m_sDetail;
  xiiUInt64 m_uiSceneRevision = 0;
  float     m_fProgress       = -1.0F; ///< Negative is indeterminate (IO/initialization).
};

class XII_GAMEENGINE_DLL xiiPlayerStopMessage : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlayerStopMessage, xiiProcessMessage);
};

/// A runtime transition request for a single owned instance. Cancellation preserves its active world.
class XII_GAMEENGINE_DLL xiiPlayerSceneMessage : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlayerSceneMessage, xiiProcessMessage);

public:
  xiiString m_sScene;
  xiiString m_sPreloadCollection;
  bool      m_bCancel = false;
};
