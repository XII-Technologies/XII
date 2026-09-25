/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GraphicsCore/Scene/SceneTypes.h>

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
  xiiSceneDatabase();
  ~xiiSceneDatabase();

  /// Removes every object and resets generations, hierarchy links, GPU mirrors, and revisions.
  /// Capacity is retained so simulation worlds can be restarted without reallocating hot arrays.
  void Clear();
  void Reserve(xiiUInt32 uiObjectCapacity);

  [[nodiscard]] xiiSceneObjectHandle CreateObject(const xiiSceneObjectDesc& desc);
  bool                               DestroyObject(xiiSceneObjectHandle hObject);

  [[nodiscard]] bool IsAlive(xiiSceneObjectHandle hObject) const;
  [[nodiscard]] xiiUInt32 GetObjectCount() const { return m_uiObjectCount; }
  [[nodiscard]] xiiUInt64 GetRevision() const { return m_uiRevision; }

  bool SetParent(xiiSceneObjectHandle hObject, xiiSceneObjectHandle hParent);
  bool SetLocalTransform(xiiSceneObjectHandle hObject, const xiiMat4& localTransform);
  bool SetLocalBounds(xiiSceneObjectHandle hObject, const xiiBoundingBoxSphere& localBounds);
  bool SetGeometry(xiiSceneObjectHandle hObject, xiiUInt32 uiGeometryIndex);
  bool SetMaterial(xiiSceneObjectHandle hObject, xiiUInt32 uiMaterialIndex);
  bool SetFlags(xiiSceneObjectHandle hObject, xiiBitflags<xiiSceneObjectFlags> flags);
  bool SetVisibilityMask(xiiSceneObjectHandle hObject, xiiUInt32 uiVisibilityMask);

  [[nodiscard]] const xiiMat4&              GetGlobalTransform(xiiSceneObjectHandle hObject) const;
  [[nodiscard]] const xiiBoundingBoxSphere& GetGlobalBounds(xiiSceneObjectHandle hObject) const;
  [[nodiscard]] xiiBitflags<xiiSceneObjectFlags> GetFlags(xiiSceneObjectHandle hObject) const;
  [[nodiscard]] xiiUInt32 GetVisibilityMask(xiiSceneObjectHandle hObject) const;

  /// Resolves all dirty transforms, compacts live objects into GPU order, and emits coalesced
  /// ranges which can be uploaded without rewriting unchanged regions.
  void CommitFrame(xiiUInt64 uiFrameIndex);

  [[nodiscard]] xiiArrayPtr<const xiiGpuSceneInstance> GetGpuInstances() const { return m_GpuInstances; }
  [[nodiscard]] xiiArrayPtr<const xiiSceneUploadRange> GetUploadRanges() const { return m_UploadRanges; }
  [[nodiscard]] const xiiSceneDatabaseStats&           GetStats() const { return m_Stats; }

private:
  static constexpr xiiUInt32 s_uiInvalidIndex = xiiInvalidIndex;

  [[nodiscard]] bool WouldCreateCycle(xiiUInt32 uiObject, xiiUInt32 uiParent) const;
  void               DetachFromParent(xiiUInt32 uiObject);
  void               AttachToParent(xiiUInt32 uiObject, xiiUInt32 uiParent);
  void               MarkSubtreeDirty(xiiUInt32 uiObject);
  void               UpdateDirtyTransforms();
  void               RebuildGpuInstances();

  xiiDynamicArray<xiiUInt32>            m_Generations;
  xiiDynamicArray<xiiUInt32>            m_FreeIndices;
  xiiDynamicArray<xiiUInt8>             m_Alive;
  xiiDynamicArray<xiiUInt8>             m_TransformDirty;
  xiiDynamicArray<xiiUInt8>             m_GpuDirty;
  xiiDynamicArray<xiiUInt32>            m_Parent;
  xiiDynamicArray<xiiUInt32>            m_FirstChild;
  xiiDynamicArray<xiiUInt32>            m_NextSibling;
  xiiDynamicArray<xiiMat4>              m_LocalTransforms;
  xiiDynamicArray<xiiMat4>              m_GlobalTransforms;
  xiiDynamicArray<xiiMat4>              m_PreviousGlobalTransforms;
  xiiDynamicArray<xiiBoundingBoxSphere> m_LocalBounds;
  xiiDynamicArray<xiiBoundingBoxSphere> m_GlobalBounds;
  xiiDynamicArray<xiiUInt32>            m_GeometryIndices;
  xiiDynamicArray<xiiUInt32>            m_MaterialIndices;
  xiiDynamicArray<xiiUInt32>            m_VisibilityMasks;
  xiiDynamicArray<xiiUInt32>            m_UserData;
  xiiDynamicArray<xiiBitflags<xiiSceneObjectFlags>> m_Flags;

  xiiDynamicArray<xiiUInt32>            m_ObjectToGpuIndex;
  xiiDynamicArray<xiiUInt32>            m_GpuToObjectIndex;
  xiiDynamicArray<xiiGpuSceneInstance>  m_GpuInstances;
  xiiDynamicArray<xiiSceneUploadRange>  m_UploadRanges;
  xiiDynamicArray<xiiUInt32>            m_WorkStack;

  xiiSceneDatabaseStats m_Stats;
  xiiUInt32             m_uiObjectCount = 0U;
  xiiUInt64             m_uiRevision    = 0U;
  xiiUInt64             m_uiLastCommittedFrame = static_cast<xiiUInt64>(-1);
};
