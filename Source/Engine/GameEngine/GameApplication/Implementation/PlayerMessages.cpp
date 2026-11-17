/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Network/Reflection/NetworkAttributes.h>
#include <GameEngine/GameApplication/PlayerMessages.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlayerStatusMessage, 1, xiiRTTIDefaultAllocator<xiiPlayerStatusMessage>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Instance", m_sInstance)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Stage", m_sStage)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Detail", m_sDetail)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Progress", m_fProgress)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("SceneRevision", m_uiSceneRevision)->AddAttributes(new xiiNetworkAttribute(5)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlayerStopMessage, 1, xiiRTTIDefaultAllocator<xiiPlayerStopMessage>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Instance", m_sInstance)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlayerSceneMessage, 1, xiiRTTIDefaultAllocator<xiiPlayerSceneMessage>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Instance", m_sInstance)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Scene", m_sScene)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("PreloadCollection", m_sPreloadCollection)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Cancel", m_bCancel)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_PlayerMessages);
