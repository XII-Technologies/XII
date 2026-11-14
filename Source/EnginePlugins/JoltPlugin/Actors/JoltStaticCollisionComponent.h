/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/World/ComponentManager.h>
#include <JoltPlugin/Resources/JoltCollisionMeshResource.h>

struct xiiMsgChildrenChanged;
struct xiiMsgComponentsChanged;
struct xiiMsgParentChanged;

using xiiJoltStaticCollisionComponentManager = xiiComponentManager<class xiiJoltStaticCollisionComponent, xiiBlockStorageType::Compact>;

/// Opts a static object and its descendants into one collision group. Nested groups and explicit
/// actors own their own subtrees. Place groups on independently loaded section roots.
class XII_JOLTPLUGIN_DLL xiiJoltStaticCollisionComponent final : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltStaticCollisionComponent, xiiComponent, xiiJoltStaticCollisionComponentManager);

public:
  enum class State : xiiUInt8
  {
    Inactive,
    PendingGeometry,
    PendingResource,
    Ready,
    Unsupported,
    Failed
  };

  xiiUInt8 GetCollisionLayer() const { return m_uiCollisionLayer; }
  void     SetCollisionLayer(xiiUInt8 uiLayer);
  bool     GetReportContacts() const { return m_bReportContacts; }
  void     SetReportContacts(bool bReport);

  /// Optional pre-cooked triangle mesh in the section root's local space. When assigned it supplies
  /// this group's entire geometry. Use separate child groups for independently controlled sections.
  const xiiJoltCollisionMeshResourceHandle& GetCollisionMesh() const { return m_hCollisionMesh; }
  void                                      SetCollisionMesh(const xiiJoltCollisionMeshResourceHandle& hMesh);
  const xiiSurfaceResourceHandle&           GetSurface() const { return m_hSurface; }
  void                                      SetSurface(const xiiSurfaceResourceHandle& hSurface);

  /// Status of the requested generation. A previous body may remain queryable during replacement.
  State GetCollisionState() const { return m_State; }
  /// Recollect custom xiiMsgBuildStaticMesh providers. Call at a synchronous world boundary.
  void InvalidateGeometry();
  void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  void DeserializeComponent(xiiWorldReader& ref_stream) override;

private:
  friend class xiiJoltPhysicsWorldModule;
  void OnActivated() override;
  void OnDeactivated() override;
  void OnGeometryChanged(xiiMsgStaticMeshChanged&);
  void OnChildrenChanged(xiiMsgChildrenChanged&);
  void OnComponentsChanged(xiiMsgComponentsChanged&);
  void OnParentChanged(xiiMsgParentChanged&);

  xiiJoltCollisionMeshResourceHandle m_hCollisionMesh;
  xiiSurfaceResourceHandle           m_hSurface;
  State                              m_State            = State::Inactive;
  xiiUInt8                           m_uiCollisionLayer = 0;
  bool                               m_bReportContacts  = false;
  bool                               m_bReportedFailure = false;
};
