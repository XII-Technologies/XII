/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <GraphicsCore/Scene/SceneTypes.h>

class xiiSceneDatabaseManager;

/// Data-oriented render scene independent of xiiWorld's component storage.
///
/// Hot fields are stored in structure-of-arrays form. Stable generation checked handles are
/// translated to dense GPU records only when CommitFrame() is called. Hierarchy mutations are
/// cycle checked and dirty propagation is iterative, allowing large robotics assemblies and
/// medical volumes without recursive stack growth.
class XII_GRAPHICSCORE_DLL xiiSceneDatabase
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSceneDatabase);

public:
  ~xiiSceneDatabase();

  /// Removes every object and advances generations, clearing hierarchy links, GPU mirrors, and revisions.
  /// Capacity is retained so simulation worlds can be restarted without reallocating hot arrays.
  void Clear();
  void Reserve(xiiUInt32 uiObjectCapacity);

  [[nodiscard]] xiiSceneObjectHandle CreateObject(const xiiSceneObjectDesc& desc);
  bool                               DestroyObject(xiiSceneObjectHandle hObject);

  [[nodiscard]] bool IsAlive(xiiSceneObjectHandle hObject) const;
  /// Resolves a GPU identity from this context's records. Zero, removed identities, and stale
  /// generations return an invalid handle. IDs are context-local, like xiiSceneObjectHandle.
  [[nodiscard]] xiiSceneObjectHandle ResolveGpuObjectId(xiiUInt32 uiObjectId) const;
  [[nodiscard]] xiiUInt32            GetObjectCount() const { return m_Objects.GetCount(); }
  [[nodiscard]] xiiUInt64            GetRevision() const { return m_uiRevision; }

  bool SetParent(xiiSceneObjectHandle hObject, xiiSceneObjectHandle hParent);
  bool SetLocalTransform(xiiSceneObjectHandle hObject, const xiiMat4& localTransform);
  bool SetLocalBounds(xiiSceneObjectHandle hObject, const xiiBoundingBoxSphere& localBounds);
  bool SetGeometry(xiiSceneObjectHandle hObject, xiiUInt32 uiGeometryIndex);
  bool SetMaterial(xiiSceneObjectHandle hObject, xiiUInt32 uiMaterialIndex);
  bool SetPrimitiveRange(xiiSceneObjectHandle hObject, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiPrimitiveCount);
  /// Associates a native ushort4/byte4 palette with one instance. No resource ownership transfers.
  /// Invalid handles, non-affine matrices and stream ranges outside the stride fail atomically.
  /// Empty palettes restore bind pose; the preceding pose survives one committed frame for velocity.
  /// First assignment and resetHistory initialize previous pose to current (e.g. after mesh reload).
  /// Input must not alias internal storage. Call before CommitFrame; external synchronization is required.
  [[nodiscard]] xiiResult SetSkinningPalette(xiiSceneObjectHandle hObject, xiiArrayPtr<const xiiMat4> palette, xiiUInt32 uiVertexStride, xiiUInt32 uiBoneIndicesOffset, xiiUInt32 uiBoneWeightsOffset, bool bResetHistory = false);
  /// Sets native GPU LOD selection without changing geometry ownership. Invalid forced LOD means
  /// automatic selection. A forced LOD must be below eight; non-finite bias and stale handles fail
  /// without mutation. Bias is measured in coverage octaves, saturated to [-16, 16] on the GPU.
  /// Forced selection bypasses hysteresis and emits no geometry until the requested LOD is resident.
  /// A draw LOD restricts this range/material candidate to one authored LOD without forcing the
  /// mesh's LOD selection. Invalid admits any selected LOD. Both indices must be below eight.
  [[nodiscard]] xiiResult SetLodSelection(xiiSceneObjectHandle hObject, xiiUInt32 uiForcedLod, float fLodBias, xiiUInt32 uiDrawLod = xiiInvalidIndex);
  /// Shares LOD controls, bounds and temporal history with a representative object in this database.
  /// Useful for material/section candidates of one mesh. The representative must use the same geometry;
  /// otherwise selection falls back to self. This is a non-owning, single-hop link: invalid or self clears
  /// it, stale arguments fail without mutation, and destroying the representative restores self selection.
  /// Filtering individual sections cannot make members choose different LODs: visibility updates
  /// history before material routing, frustum and temporal occlusion tests.
  [[nodiscard]] xiiResult SetLodGroup(xiiSceneObjectHandle hObject, xiiSceneObjectHandle hRepresentative);
  bool                    SetFlags(xiiSceneObjectHandle hObject, xiiBitflags<xiiSceneObjectFlags> flags);
  bool                    SetVisibilityMask(xiiSceneObjectHandle hObject, xiiUInt32 uiVisibilityMask);

  [[nodiscard]] const xiiMat4&                   GetGlobalTransform(xiiSceneObjectHandle hObject) const;
  [[nodiscard]] const xiiBoundingBoxSphere&      GetGlobalBounds(xiiSceneObjectHandle hObject) const;
  [[nodiscard]] xiiBitflags<xiiSceneObjectFlags> GetFlags(xiiSceneObjectHandle hObject) const;
  [[nodiscard]] xiiUInt32                        GetVisibilityMask(xiiSceneObjectHandle hObject) const;

  /// Resolves all dirty transforms, compacts live objects into GPU order, and emits coalesced
  /// ranges which can be uploaded without rewriting unchanged regions.
  void CommitFrame(xiiUInt64 uiFrameIndex);

  [[nodiscard]] xiiArrayPtr<const xiiGpuSceneInstance> GetGpuInstances() const { return m_GpuInstances; }
  [[nodiscard]] xiiArrayPtr<const xiiMat4>             GetGpuSkinningMatrices() const { return m_GpuSkinningMatrices; }
  [[nodiscard]] xiiArrayPtr<const xiiSceneUploadRange> GetUploadRanges() const { return m_UploadRanges; }
  [[nodiscard]] const xiiSceneDatabaseStats&           GetStats() const { return m_Stats; }

private:
  friend class xiiSceneDatabaseManager;

  xiiSceneDatabase();

  static constexpr xiiUInt32 s_uiInvalidIndex = xiiInvalidIndex;

  [[nodiscard]] bool WouldCreateCycle(xiiUInt32 uiObject, xiiUInt32 uiParent) const;
  void               DetachFromParent(xiiUInt32 uiObject);
  void               AttachToParent(xiiUInt32 uiObject, xiiUInt32 uiParent);
  void               MarkSubtreeDirty(xiiUInt32 uiObject);
  void               UpdateDirtyTransforms();
  void               RebuildGpuInstances();
  void               BuildSkinningSnapshot(xiiUInt32 uiObject, xiiGpuSceneInstance& instance);
  void               PrepareSkinningSnapshots();
  void               AdvanceSkinningPoses();

  struct SkinningPose
  {
    xiiDynamicArray<xiiMat4> m_Current;
    xiiDynamicArray<xiiMat4> m_Previous;
    xiiUInt32                m_uiVertexStride      = 0U;
    xiiUInt32                m_uiBoneIndicesOffset = 0U;
    xiiUInt32                m_uiBoneWeightsOffset = 0U;
    xiiVec4U32               m_SnapshotRange       = xiiVec4U32::MakeZero();
  };
  /// Only posed instances allocate palette state; ordinary static scenes pay no per-object arrays.
  xiiHashTable<xiiUInt32, SkinningPose> m_SkinningPoses;
  xiiDynamicArray<xiiMat4>              m_GpuSkinningMatrices;

  xiiIdTable<xiiSceneObjectId, xiiUInt32>           m_Objects;
  xiiDynamicArray<xiiSceneObjectHandle>             m_ObjectHandles;
  xiiDynamicArray<xiiUInt8>                         m_TransformDirty;
  xiiDynamicArray<xiiUInt8>                         m_GpuDirty;
  xiiDynamicArray<xiiUInt32>                        m_Parent;
  xiiDynamicArray<xiiUInt32>                        m_FirstChild;
  xiiDynamicArray<xiiUInt32>                        m_NextSibling;
  xiiDynamicArray<xiiMat4>                          m_LocalTransforms;
  xiiDynamicArray<xiiMat4>                          m_GlobalTransforms;
  xiiDynamicArray<xiiMat4>                          m_PreviousGlobalTransforms;
  xiiDynamicArray<xiiBoundingBoxSphere>             m_LocalBounds;
  xiiDynamicArray<xiiBoundingBoxSphere>             m_GlobalBounds;
  xiiDynamicArray<xiiUInt32>                        m_GeometryIndices;
  xiiDynamicArray<xiiUInt32>                        m_MaterialIndices;
  xiiDynamicArray<xiiUInt32>                        m_FirstPrimitives;
  xiiDynamicArray<xiiUInt32>                        m_PrimitiveCounts;
  xiiDynamicArray<xiiUInt32>                        m_ForcedLods;
  xiiDynamicArray<xiiUInt32>                        m_DrawLods;
  xiiDynamicArray<xiiSceneObjectHandle>             m_LodGroups;
  xiiDynamicArray<float>                            m_LodBiases;
  xiiDynamicArray<xiiUInt32>                        m_VisibilityMasks;
  xiiDynamicArray<xiiUInt32>                        m_UserData;
  xiiDynamicArray<xiiBitflags<xiiSceneObjectFlags>> m_Flags;

  xiiDynamicArray<xiiUInt32>           m_ObjectToGpuIndex;
  xiiDynamicArray<xiiUInt32>           m_GpuToObjectIndex;
  xiiDynamicArray<xiiGpuSceneInstance> m_GpuInstances;
  xiiDynamicArray<xiiSceneUploadRange> m_UploadRanges;
  xiiDynamicArray<xiiUInt32>           m_WorkStack;

  xiiSceneDatabaseStats m_Stats;
  xiiUInt64             m_uiRevision           = 0U;
  xiiUInt64             m_uiLastCommittedFrame = static_cast<xiiUInt64>(-1);
};
