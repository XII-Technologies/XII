#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Messages/EventMessage.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>

class xiiPhysicsWorldModuleInterface;

struct XII_GAMEENGINE_DLL xiiMsgSensorDetectedObjectsChanged : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSensorDetectedObjectsChanged, xiiEventMessage);

  xiiArrayPtr<xiiGameObjectHandle> m_DetectedObjects;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for sensor components that can be used for AI perception like vision or hearing.
///
/// Derived component classes implemented different shapes like sphere cylinder or cone.
/// All sensors do a query with the specified spatial category in the world's spatial system first, therefore it is necessary to have objects
/// with matching spatial category for the sensors to detect them. This can be achieved with components like e.g. xiiMarkerComponent.
/// Visibility tests via raycasts are done afterwards by default but can be disabled.
/// The components store an array of all their currently detected objects and send a xiiMsgSensorDetectedObjectsChanged message if this array changes.
class XII_GAMEENGINE_DLL xiiSensorComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiSensorComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSensorComponent

public:
  xiiSensorComponent();
  ~xiiSensorComponent();

  virtual void GetObjectsInSensorVolume(xiiDynamicArray<xiiGameObject*>& out_objects) const = 0;
  virtual void DebugDrawSensorShape() const                                                 = 0;

  void        SetSpatialCategory(const char* szCategory); // [ property ]
  const char* GetSpatialCategory() const;                 // [ property ]

  bool     m_bTestVisibility  = true; // [ property ]
  xiiUInt8 m_uiCollisionLayer = 0;    // [ property ]

  void                          SetUpdateRate(const xiiEnum<xiiUpdateRate>& updateRate); // [ property ]
  const xiiEnum<xiiUpdateRate>& GetUpdateRate() const;                                   // [ property ]

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  void            SetColor(xiiColorGammaUB color); // [ property ]
  xiiColorGammaUB GetColor() const;                // [ property ]

  /// \brief Returns the list of objects that this sensor has detected during its last update
  xiiArrayPtr<const xiiGameObjectHandle> GetLastDetectedObjects() const { return m_LastDetectedObjects; }

  /// \brief Updates the sensor state right now.
  ///
  /// If the update rate isn't set to 'Never', this is periodically done automatically.
  /// Otherwise, it has to be called manually to update the state on demand.
  ///
  /// Afterwards out_objectsInSensorVolume will contain all objects that were found inside the volume.
  /// ref_detectedObjects needs to be provided as a temp array, but will not contain a usable result afterwards,
  /// call GetLastDetectedObjects() instead.
  ///
  /// If bPostChangeMsg is true, xiiMsgSensorDetectedObjectsChanged is posted in case there is a change.
  /// Physical visibility checks are skipped in case pPhysicsWorldModule is null.
  ///
  /// Returns true, if there was a change in detected objects, false if the same objects were detected as last time.
  bool RunSensorCheck(xiiPhysicsWorldModuleInterface* pPhysicsWorldModule, xiiDynamicArray<xiiGameObject*>& out_objectsInSensorVolume, xiiDynamicArray<xiiGameObjectHandle>& ref_detectedObjects, bool bPostChangeMsg) const;

protected:
  void UpdateSpatialCategory();
  void UpdateScheduling();
  void UpdateDebugInfo();

  xiiEnum<xiiUpdateRate> m_UpdateRate;
  bool                   m_bShowDebugInfo = false;
  xiiColorGammaUB        m_Color          = xiiColorScheme::LightUI(xiiColorScheme::Orange);

  xiiHashedString          m_sSpatialCategory;
  xiiSpatialData::Category m_SpatialCategory = xiiInvalidSpatialDataCategory;

  friend class xiiSensorWorldModule;
  mutable xiiDynamicArray<xiiGameObjectHandle> m_LastDetectedObjects;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  mutable xiiDynamicArray<xiiVec3> m_LastOccludedObjectPositions;
#endif
};

//////////////////////////////////////////////////////////////////////////

using xiiSensorSphereComponentManager = xiiComponentManager<class xiiSensorSphereComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiSensorSphereComponent : public xiiSensorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSensorSphereComponent, xiiSensorComponent, xiiSensorSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSensorComponent

  virtual void GetObjectsInSensorVolume(xiiDynamicArray<xiiGameObject*>& out_objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSensorSphereComponent

public:
  xiiSensorSphereComponent();
  ~xiiSensorSphereComponent();

  float m_fRadius = 10.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using xiiSensorCylinderComponentManager = xiiComponentManager<class xiiSensorCylinderComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiSensorCylinderComponent : public xiiSensorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSensorCylinderComponent, xiiSensorComponent, xiiSensorCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSensorComponent

  virtual void GetObjectsInSensorVolume(xiiDynamicArray<xiiGameObject*>& out_objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSensorCylinderComponent

public:
  xiiSensorCylinderComponent();
  ~xiiSensorCylinderComponent();

  float m_fRadius = 10.0f; // [ property ]
  float m_fHeight = 10.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using xiiSensorConeComponentManager = xiiComponentManager<class xiiSensorConeComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiSensorConeComponent : public xiiSensorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSensorConeComponent, xiiSensorComponent, xiiSensorConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSensorComponent

  virtual void GetObjectsInSensorVolume(xiiDynamicArray<xiiGameObject*>& out_objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSensorConeComponent

public:
  xiiSensorConeComponent();
  ~xiiSensorConeComponent();

  float    m_fNearDistance = 0.0f;                            // [ property ]
  float    m_fFarDistance  = 10.0f;                           // [ property ]
  xiiAngle m_Angle         = xiiAngle::MakeFromDegree(90.0f); // [ property ]
};

//////////////////////////////////////////////////////////////////////////

class xiiSensorWorldModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiSensorWorldModule, xiiWorldModule);

public:
  xiiSensorWorldModule(xiiWorld* pWorld);

  virtual void Initialize() override;

  void AddComponentToSchedule(xiiSensorComponent* pComponent, xiiUpdateRate::Enum updateRate);
  void RemoveComponentToSchedule(xiiSensorComponent* pComponent);

  void AddComponentForDebugRendering(xiiSensorComponent* pComponent);
  void RemoveComponentForDebugRendering(xiiSensorComponent* pComponent);

private:
  void UpdateSensors(const xiiWorldModule::UpdateContext& context);
  void DebugDrawSensors(const xiiWorldModule::UpdateContext& context);

  xiiIntervalScheduler<xiiComponentHandle> m_Scheduler;
  xiiPhysicsWorldModuleInterface*          m_pPhysicsWorldModule = nullptr;

  xiiDynamicArray<xiiGameObject*>      m_ObjectsInSensorVolume;
  xiiDynamicArray<xiiGameObjectHandle> m_DetectedObjects;

  xiiDynamicArray<xiiComponentHandle> m_DebugComponents;
};
