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

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgPhysicsAddForce);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgPhysicsAddForce, 1, xiiRTTIDefaultAllocator<xiiMsgPhysicsAddForce>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    XII_MEMBER_PROPERTY("Force", m_vForce),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgPhysicsJointBroke);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgPhysicsJointBroke, 1, xiiRTTIDefaultAllocator<xiiMsgPhysicsJointBroke>)
//{
  //XII_BEGIN_PROPERTIES
  //{
  //  XII_MEMBER_PROPERTY("JointObject", m_hJointObject)
  //}
  //XII_END_PROPERTIES;
//}
XII_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgBuildStaticMesh);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgBuildStaticMesh, 1, xiiRTTIDefaultAllocator<xiiMsgBuildStaticMesh>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


XII_STATICLINK_FILE(Core, Core_Interfaces_PhysicsWorldModule);
