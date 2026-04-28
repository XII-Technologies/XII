/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPhysicsWorldModuleInterface, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiPhysicsShapeType, 1)
  XII_BITFLAGS_CONSTANT(xiiPhysicsShapeType::Static),
  XII_BITFLAGS_CONSTANT(xiiPhysicsShapeType::Dynamic),
  XII_BITFLAGS_CONSTANT(xiiPhysicsShapeType::Query),
  XII_BITFLAGS_CONSTANT(xiiPhysicsShapeType::Trigger),
  XII_BITFLAGS_CONSTANT(xiiPhysicsShapeType::Character),
  XII_BITFLAGS_CONSTANT(xiiPhysicsShapeType::Ragdoll),
  XII_BITFLAGS_CONSTANT(xiiPhysicsShapeType::Rope),
  XII_BITFLAGS_CONSTANT(xiiPhysicsShapeType::Cloth),
  XII_BITFLAGS_CONSTANT(xiiPhysicsShapeType::Debris),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgPhysicsAddImpulse);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgPhysicsAddImpulse, 1, xiiRTTIDefaultAllocator<xiiMsgPhysicsAddImpulse>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    XII_MEMBER_PROPERTY("Impulse", m_vImpulse),
    XII_MEMBER_PROPERTY("ObjectFilterID", m_uiObjectFilterID),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgPhysicCharacterContact);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgPhysicCharacterContact, 1, xiiRTTIDefaultAllocator<xiiMsgPhysicCharacterContact>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Character", m_hCharacter),
    XII_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    XII_MEMBER_PROPERTY("Normal", m_vNormal),
    XII_MEMBER_PROPERTY("CharacterVelocity", m_vCharacterVelocity),
    XII_MEMBER_PROPERTY("Impact", m_fImpact),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgPhysicContact);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgPhysicContact, 1, xiiRTTIDefaultAllocator<xiiMsgPhysicContact>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    XII_MEMBER_PROPERTY("Normal", m_vNormal),
    XII_MEMBER_PROPERTY("ImpactSqr", m_fImpactSqr),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgPhysicsJointBroke);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgPhysicsJointBroke, 1, xiiRTTIDefaultAllocator<xiiMsgPhysicsJointBroke>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("JointObject", m_hJointObject)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgObjectGrabbed);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgObjectGrabbed, 1, xiiRTTIDefaultAllocator<xiiMsgObjectGrabbed>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GrabbedBy", m_hGrabbedBy),
    XII_MEMBER_PROPERTY("GotGrabbed", m_bGotGrabbed),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgReleaseObjectGrab);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgReleaseObjectGrab, 1, xiiRTTIDefaultAllocator<xiiMsgReleaseObjectGrab>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GrabbedObjectToRelease", m_hGrabbedObjectToRelease),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgBuildStaticMesh);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgBuildStaticMesh, 1, xiiRTTIDefaultAllocator<xiiMsgBuildStaticMesh>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(Core, Core_Interfaces_Implementation_PhysicsWorldModule);
