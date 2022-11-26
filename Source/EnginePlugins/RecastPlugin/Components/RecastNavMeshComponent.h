#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/RecastPluginDLL.h>

class xiiRecastWorldModule;
class xiiAbstractObjectNode;

using xiiRecastNavMeshResourceHandle = xiiTypedResourceHandle<class xiiRecastNavMeshResource>;

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for all Recast components
class XII_RECASTPLUGIN_DLL xiiRcComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiRcComponent, xiiComponent);

public:
  xiiRcComponent();
  ~xiiRcComponent();
};

//////////////////////////////////////////////////////////////////////////

class XII_RECASTPLUGIN_DLL xiiRcNavMeshComponentManager : public xiiComponentManager<class xiiRcNavMeshComponent, xiiBlockStorageType::Compact>
{
  typedef xiiComponentManager<class xiiRcNavMeshComponent, xiiBlockStorageType::Compact> SUPER;

public:
  xiiRcNavMeshComponentManager(xiiWorld* pWorld);
  ~xiiRcNavMeshComponentManager();

  virtual void Initialize() override;

  xiiRecastWorldModule* GetRecastWorldModule() const { return m_pWorldModule; }

  void Update(const xiiWorldModule::UpdateContext& context);

private:
  xiiRecastWorldModule* m_pWorldModule;
};

//////////////////////////////////////////////////////////////////////////

class XII_RECASTPLUGIN_DLL xiiRcNavMeshComponent : public xiiRcComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRcNavMeshComponent, xiiRcComponent, xiiRcNavMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  //  xiiRcNavMeshComponent

public:
  xiiRcNavMeshComponent();
  ~xiiRcNavMeshComponent();

  bool m_bShowNavMesh = false; // [ property ]

  xiiRecastConfig m_NavMeshConfig; // [ property ]

protected:
  void Update();
  void VisualizeNavMesh();
  void VisualizePointsOfInterest();

  xiiRecastNavMeshResourceHandle m_hNavMesh;


  //////////////////////////////////////////////////////////////////////////
  // Editor

protected:
  void OnObjectCreated(const xiiAbstractObjectNode& node);
};
