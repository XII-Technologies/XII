#pragma once

#include <Core/World/WorldModule.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Shader/Types.h>
#include <GraphicsFoundation/Tools/DynamicBuffer.h>

struct xiiPerInstanceData;
struct xiiRenderWorldExtractionEvent;

struct XII_GRAPHICSCORE_DLL xiiInstanceDataOffset
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiOffset    = xiiInvalidIndex;
  xiiUInt8  m_uiIsDynamic = 0;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsInvalidated() const
  {
    return m_uiOffset == xiiInvalidIndex;
  }
};

struct XII_GRAPHICSCORE_DLL xiiCustomInstanceDataOffset
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiOffset = xiiInvalidIndex;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsInvalidated() const
  {
    return m_uiOffset == xiiInvalidIndex;
  }
};

struct XII_GRAPHICSCORE_DLL xiiMsgCustomInstanceDataOffsetChanged : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgCustomInstanceDataOffsetChanged, xiiMessage);

  xiiCustomInstanceDataOffset m_NewOffset;
};

/// \brief Manager for render data and instance data buffers.
///
/// Render data is used to extract rendering information from components during the extraction phase that is then used for rendering.
/// If many objects should be rendered with one instanced draw call, instance data buffers are used to hold the per-instance information.
/// For that the render data should derive from xiiInstanceableRenderData. See xiiPerInstanceData what data is supported by default for each instance.
/// When more per instance data is needed it is possible to register a custom instance data buffer and attach that to the render data as well.
class XII_GRAPHICSCORE_DLL xiiRenderDataManager : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();

  XII_ADD_DYNAMIC_REFLECTION(xiiRenderDataManager, xiiWorldModule);

public:
  xiiRenderDataManager(xiiWorld* pWorld);
  virtual ~xiiRenderDataManager();

  virtual void Initialize() override;

  /// \brief Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
  template <typename T>
  T* CreateRenderDataForThisFrame(const xiiGameObject* pOwner) const;

  // TODO: move render data caching into this world module as well

  /// \brief Gets or creates per-instance data for the given instance data offset.
  ///
  /// This function is thread-safe and is typically called in an xiiMsgExtractRenderData message handler.
  /// The render data manager holds two instance data buffers, one for static objects and one for dynamic objects.
  /// Typically one would pass GetOwner()->IsDynamic() as bDynamic. If the corresponding render data is not cached
  /// it is better to always pass true so that the static buffer does not need to be uploaded every frame.
  xiiArrayPtr<xiiPerInstanceData> GetOrCreateInstanceData(const xiiComponent* pOwnerComponent, bool bDynamic, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount = 1) const;

  /// \brief Deletes the instance data associated with the given instance data offset.
  ///
  /// This function is thread-safe but is typically called in the OnDeactivated function of a component.
  void DeleteInstanceData(xiiInstanceDataOffset& inout_instanceDataOffset) const;

  /// \brief Helper function to fill xiiPerInstanceData.
  static void FillPerInstanceData(xiiPerInstanceData& out_perInstanceData, const xiiGameObject* pObject, const xiiTransform& globalTransform, xiiUInt32 uiUniqueID = 0, const xiiColor& color = xiiColor::White, float fBoundingSphereRadius = 1.0f, xiiUInt32 uiRandomSeed = 0);

  /// \brief Helper function that combines GetOrCreateInstanceData and FillPerInstanceData.
  xiiSharedPtr<xiiGALDynamicBuffer> GetOrCreateInstanceDataAndFill(const xiiComponent& ownerComponent, bool bDynamic, const xiiTransform& globalTransform, xiiInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiUniqueID = 0, const xiiColor& color = xiiColor::White) const;


  /// \brief Registers a custom instance data buffer that can be used to store additional per-instance data.
  ///
  /// The beforeUploadCallback is called just before the buffer is uploaded each frame, so it can be used to e.g. wait for a task that generated the data.
  xiiUInt32 RegisterCustomInstanceData(const xiiGALBufferCreationDescription& desc, xiiStringView sDebugName, xiiDelegate<void()> beforeUploadCallback = {});

  /// \brief Gets or creates custom per-instance data for the given instance data offset.
  ///
  /// This function is thread-safe and is typically called in an xiiMsgExtractRenderData message handler.
  template <typename T>
  xiiArrayPtr<T> GetOrCreateCustomInstanceData(xiiUInt32 uiCustomDataIndex, const xiiComponent* pOwnerComponent, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount = 1) const;

  /// \brief Deletes the custom instance data associated with the given instance data offset.
  ///
  /// This function is thread-safe but is typically called in the OnDeactivated function of a component.
  void DeleteCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiCustomInstanceDataOffset& inout_instanceDataOffset) const;

  /// \brief Helper function that combines GetOrCreateCustomInstanceData and fills it with the given data.
  template <typename T>
  xiiSharedPtr<xiiGALDynamicBuffer> GetOrCreateCustomInstanceDataAndFill(xiiUInt32 uiCustomDataIndex, const xiiComponent& ownerComponent, xiiCustomInstanceDataOffset& inout_instanceDataOffset, const T& data) const;

  /// \brief Returns the underlying dynamic buffer for the given custom instance data buffer index.
  xiiSharedPtr<xiiGALDynamicBuffer> GetCustomInstanceDataBuffer(xiiUInt32 uiCustomDataIndex) const;

  /// \brief Compacts the given custom instance data buffer to reduce fragmentation.
  ///
  /// This is only necessary if allocations with different counts were created and deleted over time.
  void CompactCustomInstanceDataBuffer(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiMaxSteps = 16);


  /// \brief Gets or creates skinning data for the given instance data offset.
  ///
  /// xiiSkinningState wraps around these functions to manage skinning data for skinned meshes
  /// and should be preferred instead of calling these functions directly.
  xiiArrayPtr<xiiShaderTransform> GetOrCreateSkinningData(const xiiComponent* pOwnerComponent, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiNumTransforms) const;

  /// \brief Gets the skinning data for reading for the given instance data offset.
  xiiArrayPtr<const xiiShaderTransform> GetSkinningData(const xiiCustomInstanceDataOffset& instanceDataOffset) const;

  /// \brief Deletes the skinning data associated with the given instance data offset.
  void DeleteSkinningData(xiiCustomInstanceDataOffset& inout_instanceDataOffset) const;

  /// \brief Returns the underlying dynamic buffer that holds the skinning data.
  xiiSharedPtr<xiiGALDynamicBuffer> GetSkinningDataBuffer() const;

private:
  xiiByteArrayPtr GetOrCreateCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiStructByteSize, const xiiComponent* pOwnerComponent, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount) const;

  void CompactSkinningDataBuffer(const UpdateContext& context);
  void OnExtractionEvent(const xiiRenderWorldExtractionEvent& e);

  mutable xiiMutex m_Mutex;

  xiiHybridArray<xiiSharedPtr<xiiGALDynamicBuffer>, 16> m_Buffers;
  xiiDynamicArray<xiiDelegate<void()>>                  m_BeforeUploadCallbacks;

  struct ExtractionData
  {
    xiiHybridArray<xiiSharedPtr<xiiGALDynamicBuffer>, 16> m_pBuffers;
  };

  ExtractionData m_ExtractionData;
};

#include <GraphicsCore/Pipeline/Implementation/RenderDataManager_inl.h>
