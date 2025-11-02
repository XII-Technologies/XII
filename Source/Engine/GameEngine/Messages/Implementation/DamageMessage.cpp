#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgDamage);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgDamage, 1, xiiRTTIDefaultAllocator<xiiMsgDamage>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Damage", m_fDamage),
    XII_MEMBER_PROPERTY("HitObjectName", m_sHitObjectName),
    XII_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    XII_MEMBER_PROPERTY("ImpactDirection", m_vImpactDirection),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_DamageMessage);
