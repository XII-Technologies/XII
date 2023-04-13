#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>

//////////////////////////////////////////////////////////////////////////

class XII_JOLTPLUGIN_DLL xiiJoltQueryShapeActorComponentManager : public xiiComponentManager<class xiiJoltQueryShapeActorComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiJoltQueryShapeActorComponentManager(xiiWorld* pWorld);
  ~xiiJoltQueryShapeActorComponentManager();

private:
  friend class xiiJoltWorldModule;
  friend class xiiJoltQueryShapeActorComponent;

  void UpdateMovingQueryShapes();

  xiiDynamicArray<xiiJoltQueryShapeActorComponent*> m_MovingQueryShapes;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A physics actor that can be moved procedurally (like a kinematic actor) but that doesn't affect rigid bodies.
///
/// It passes right through dynamic actors. However, you can detect it via raycasts or shape casts.
/// This is useful to represent detail shapes (like the collision shapes of animated meshes) that should be pickable,
/// but that shouldn't interact with the world otherwise.
/// They are more lightweight at runtime than full kinematic dynamic actors.
class XII_JOLTPLUGIN_DLL xiiJoltQueryShapeActorComponent : public xiiJoltActorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltQueryShapeActorComponent, xiiJoltActorComponent, xiiJoltQueryShapeActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltQueryShapeActorComponent
public:
  xiiJoltQueryShapeActorComponent();
  ~xiiJoltQueryShapeActorComponent();

  void        SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;             // [ property ]

  xiiSurfaceResourceHandle m_hSurface; // [ property ]

protected:
  const xiiJoltMaterial* GetJoltMaterial() const;
};
