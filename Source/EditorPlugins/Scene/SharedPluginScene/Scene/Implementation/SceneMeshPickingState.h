/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>
#include <SharedPluginScene/Scene/Implementation/ScenePickingReadback.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>

struct xiiResourceEvent;

/// Internal mutable state, never shared by resource definitions or views. Native owner metadata
/// is captured with each bounded image, so recycling local GPU IDs cannot remap old pixels.
class xiiSceneMeshPickingState
{
public:
  xiiSceneMeshPickingState();
  ~xiiSceneMeshPickingState();

  struct Owner
  {
    xiiSceneMeshSource    m_Source;
    xiiTransform          m_Transform;
    xiiMeshResourceHandle m_hMesh;
    xiiUInt64             m_uiConfiguration = 0;
  };

  static xiiUInt64 ConfigurationSignature(const xiiMeshComponentBase& component);
  void             Invalidate();
  xiiResult        Poll();
  xiiResult        ResizeVisibility(xiiUInt32 instances, xiiUInt64 meshletCount);
  xiiResult        Capture(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle surface);
  xiiResult        Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, xiiSceneMeshPickingPass::PickingResult& out_hit);
  xiiResult        Marquee(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiGameObjectHandle>& out_objects);

  xiiSceneDatabaseContext         m_Scene;
  xiiGpuVisibilityContextHandle   m_hVisibility;
  xiiDynamicArray<xiiRenderData*> m_Packets;
  xiiHashSet<xiiGameObjectHandle> m_Selected;
  xiiUInt64                       m_uiFingerprint = 0;

private:
  void OnResourceEvent(const xiiResourceEvent& event);
  bool Resolve(const xiiView& view, xiiUInt32 id, const Owner*& out_owner) const;
  bool Pixel(const xiiView& view, xiiUInt32 x, xiiUInt32 y, xiiUInt32& out_index) const;

  xiiScenePickingReadback        m_Readback;
  xiiHashTable<xiiUInt32, Owner> m_SlotOwners[xiiScenePickingReadback::SlotCount];
  xiiHashTable<xiiUInt32, Owner> m_Owners;
  xiiAtomicInteger32             m_ResourceEpoch      = 0;
  xiiInt32                       m_iObservedEpoch     = 0;
  xiiUInt32                      m_uiInstanceCapacity = 0;
  xiiUInt32                      m_uiMeshletCapacity  = 0;
};
