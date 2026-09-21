/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>

class xiiPrefabReferenceComponent;

class XII_CORE_DLL xiiPrefabReferenceComponentManager : public xiiComponentManager<xiiPrefabReferenceComponent, xiiBlockStorageType::Compact>
{
public:
  xiiPrefabReferenceComponentManager(xiiWorld* pWorld);
  ~xiiPrefabReferenceComponentManager();

  virtual void Initialize() override;

  void Update(const xiiWorldModule::UpdateContext& context);
  void AddToUpdateList(xiiPrefabReferenceComponent* pComponent);

private:
  void ResourceEventHandler(const xiiResourceEvent& e);

  xiiDeque<xiiComponentHandle> m_ComponentsToUpdate;
};

/// The central component to instantiate prefabs.
///
/// This component instantiates a prefab and attaches the instantiated objects as children to this object.
/// The component is able to remove and recreate instantiated objects, which is needed at editing time.
/// Whenever the prefab resource changes, this component re-creates the instance.
///
/// It also holds prefab parameters, which are passed through during instantiation.
/// For that it also implements remapping of game object references, so that they can be passed into prefabs during instantiation.
class XII_CORE_DLL xiiPrefabReferenceComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPrefabReferenceComponent, xiiComponent, xiiPrefabReferenceComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiPrefabReferenceComponent

public:
  xiiPrefabReferenceComponent();
  ~xiiPrefabReferenceComponent();

  void                                             SetPrefab(const xiiPrefabResourceHandle& hPrefab); // [ property ]
  XII_ALWAYS_INLINE const xiiPrefabResourceHandle& GetPrefab() const { return m_hPrefab; }            // [ property ]

  void SetShowShapeIcons(bool bShow); // [ property ]
  bool GetShowShapeIcons() const;     // [ property ]

  const xiiRangeView<xiiStringView, xiiUInt32> GetParameters() const;                                         // [ property ] (exposed parameter)
  void                                         SetParameter(xiiStringView sKey, const xiiVariant& value);     // [ property ] (exposed parameter)
  void                                         RemoveParameter(xiiStringView sKey);                           // [ property ] (exposed parameter)
  bool                                         GetParameter(xiiStringView sKey, xiiVariant& out_value) const; // [ property ] (exposed parameter)

  static void SerializePrefabParameters(const xiiWorld& world, xiiWorldWriter& ref_stream, xiiArrayMap<xiiHashedString, xiiVariant> parameters);
  static void DeserializePrefabParameters(xiiArrayMap<xiiHashedString, xiiVariant>& out_parameters, xiiWorldReader& ref_stream);

private:
  void InstantiatePrefab();
  void ClearPreviousInstances();

  xiiPrefabResourceHandle                  m_hPrefab;
  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;
};
