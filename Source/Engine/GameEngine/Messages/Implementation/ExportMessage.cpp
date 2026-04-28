/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Messages/ExportMessage.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgExport);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgExport, 1, xiiRTTIDefaultAllocator<xiiMsgExport>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DocumentType", m_sDocumentType),
    XII_MEMBER_PROPERTY("DocumentGuid", m_sDocumentGuid),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_ExportMessage);
