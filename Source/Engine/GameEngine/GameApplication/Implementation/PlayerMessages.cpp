/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameApplication/PlayerMessages.h>
#include <GameEngine/GameEnginePCH.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlayerStatusMessage, 1, xiiRTTIDefaultAllocator<xiiPlayerStatusMessage>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Instance", m_sInstance),
    XII_MEMBER_PROPERTY("Stage", m_sStage),
    XII_MEMBER_PROPERTY("Detail", m_sDetail),
    XII_MEMBER_PROPERTY("Progress", m_fProgress),
    XII_MEMBER_PROPERTY("SceneRevision", m_uiSceneRevision),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlayerStopMessage, 1, xiiRTTIDefaultAllocator<xiiPlayerStopMessage>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlayerSceneMessage, 1, xiiRTTIDefaultAllocator<xiiPlayerSceneMessage>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Scene", m_sScene),
    XII_MEMBER_PROPERTY("PreloadCollection", m_sPreloadCollection),
    XII_MEMBER_PROPERTY("Cancel", m_bCancel),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_PlayerMessages);
