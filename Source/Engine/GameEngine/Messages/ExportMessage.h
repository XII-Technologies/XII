/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Communication/Message.h>

/// Message that is sent to all game objects when a scene or prefab is being exported.
/// This message can be handled in scripts or custom components to e.g. remove editor only objects/components or save custom data.
struct XII_GAMEENGINE_DLL xiiMsgExport : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExport, xiiMessage);

  xiiString m_sDocumentType; ///< The type of document that is being exported, e.g. "Prefab", "Scene", etc.
  xiiString m_sDocumentGuid; ///< The GUID (as string) of the document that is being exported.
};
