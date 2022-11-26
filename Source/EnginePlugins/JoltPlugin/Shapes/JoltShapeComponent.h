#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Component.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct xiiMsgExtractGeometry;
struct xiiMsgUpdateLocalBounds;
class xiiJoltUserData;
class xiiJoltMaterial;

namespace JPH
{
  class Shape;
}

struct xiiJoltSubShape
{
  JPH::Shape*  m_pShape    = nullptr;
  xiiTransform m_Transform = xiiTransform::IdentityTransform();
};

class XII_JOLTPLUGIN_DLL xiiJoltShapeComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiJoltShapeComponent, xiiComponent);


  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

protected:
  virtual void Initialize() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltShapeComponent

public:
  xiiJoltShapeComponent();
  ~xiiJoltShapeComponent();

  virtual void ExtractGeometry(xiiMsgExtractGeometry& msg) const {}

protected:
  friend class xiiJoltActorComponent;
  virtual void CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial) = 0;

  const xiiJoltUserData* GetUserData();
  xiiUInt32              GetUserDataIndex();

  xiiUInt32 m_uiUserDataIndex = xiiInvalidIndex;
};
