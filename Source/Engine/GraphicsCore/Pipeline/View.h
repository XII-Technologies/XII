#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>

#include <GraphicsFoundation/Device/SwapChain.h>

#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelineNode.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/RenderTargetSetup.h>

class xiiFrustum;
class xiiWorld;
class xiiRenderPipeline;

/// \brief Encapsulates a view on the given world through the given camera
/// and rendered with the specified RenderPipeline into the given render target setup.
class XII_GRAPHICSCORE_DLL xiiView : public xiiRenderPipelineNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiView, xiiRenderPipelineNode);

private:
  /// \brief Use xiiRenderLoop::CreateView to create a view.
  xiiView();
  ~xiiView();

public:
  xiiViewHandle GetHandle() const;

  void          SetName(xiiStringView sName);
  xiiStringView GetName() const;

  void            SetWorld(xiiWorld* pWorld);
  xiiWorld*       GetWorld();
  const xiiWorld* GetWorld() const;

  /// \brief Sets the swapchain that this view will be rendering into. Can be invalid in case the render target is an off-screen buffer in which case SetRenderTargets needs to be called.
  /// Setting the swap-chain is necessary in order to acquire and present the image to the window.
  /// SetSwapChain and SetRenderTargets are mutually exclusive. Calling this function will reset the render targets.
  void                  SetSwapChain(xiiGALSwapChainHandle hSwapChain);
  xiiGALSwapChainHandle GetSwapChain() const;

  /// \brief Sets the off-screen render targets. Use SetSwapChain if rendering to a window.
  /// SetSwapChain and SetRenderTargets are mutually exclusive. Calling this function will reset the swap chain.
  void                       SetRenderTargets(const xiiGALRenderTargets& renderTargets);
  const xiiGALRenderTargets& GetRenderTargets() const;

  /// \brief Returns the render targets that were either set via the swapchain or via the manually set render targets.
  const xiiGALRenderTargets& GetActiveRenderTargets() const;

  void                            SetRenderPipelineResource(xiiRenderPipelineResourceHandle hPipeline);
  xiiRenderPipelineResourceHandle GetRenderPipelineResource() const;

  void             SetCamera(xiiCamera* pCamera);
  xiiCamera*       GetCamera();
  const xiiCamera* GetCamera() const;

  void             SetCullingCamera(const xiiCamera* pCamera);
  const xiiCamera* GetCullingCamera() const;

  void             SetLodCamera(const xiiCamera* pCamera);
  const xiiCamera* GetLodCamera() const;

  /// \brief Returns the camera usage hint for the view.
  xiiEnum<xiiCameraUsageHint> GetCameraUsageHint() const;
  /// \brief Sets the camera usage hint for the view. If not 'None', the camera component of the same usage will be auto-connected
  ///   to this view.
  void SetCameraUsageHint(xiiEnum<xiiCameraUsageHint> val);

  void                       SetViewRenderMode(xiiEnum<xiiViewRenderMode> value);
  xiiEnum<xiiViewRenderMode> GetViewRenderMode() const;

  void                SetViewport(const xiiRectFloat& viewport);
  const xiiRectFloat& GetViewport() const;

  /// \brief Forces the render pipeline to be rebuilt.
  void ForceUpdate();

  const xiiViewData& GetData() const;

  bool IsValid() const;

  /// \brief Extracts all relevant data from the world to render the view.
  void ExtractData();

  /// \brief Returns a task implementation that calls ExtractData on this view.
  const xiiSharedPtr<xiiTask>& GetExtractTask();


  /// \brief Calculates the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fNormalizedScreenPosX and fNormalizedScreenPosY are expected to be in [0; 1] range (normalized screen coordinates).
  /// If no ray can be computed, EZ_FAILURE is returned.
  xiiResult ComputePickingRay(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vRayStartPos, xiiVec3& out_vRayDir) const;

  /// \brief Calculates the normalized screen-space coordinate ([0; 1] range) that the given world-space point projects to.
  ///
  /// Returns EZ_FAILURE, if the point could not be projected into screen-space.
  xiiResult ComputeScreenSpacePos(const xiiVec3& vWorldPos, xiiVec3& out_vScreenPosNormalized) const;

  /// \brief Calculates the world-space position that the given normalized screen-space coordinate maps to
  xiiResult ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vWorldPos) const;

  /// \brief Converts a screen-space position from pixel coordinates to normalized coordinates.
  void ConvertScreenPixelPosToNormalizedPos(xiiVec3& inout_vPixelPos);

  /// \brief Converts a screen-space position from normalized coordinates to pixel coordinates.
  void ConvertScreenNormalizedPosToPixelPos(xiiVec3& inout_vNormalizedPos);


  /// \brief Returns the current projection matrix.
  const xiiMat4& GetProjectionMatrix(xiiCameraEye eye) const;

  /// \brief Returns the current inverse projection matrix.
  const xiiMat4& GetInverseProjectionMatrix(xiiCameraEye eye) const;

  /// \brief Returns the current view matrix (camera orientation).
  const xiiMat4& GetViewMatrix(xiiCameraEye eye) const;

  /// \brief Returns the current inverse view matrix (inverse camera orientation).
  const xiiMat4& GetInverseViewMatrix(xiiCameraEye eye) const;

  /// \brief Returns the current view-projection matrix.
  const xiiMat4& GetViewProjectionMatrix(xiiCameraEye eye) const;

  /// \brief Returns the current inverse view-projection matrix.
  const xiiMat4& GetInverseViewProjectionMatrix(xiiCameraEye eye) const;

  /// \brief Returns the frustum that should be used for determine visible objects for this view.
  void ComputeCullingFrustum(xiiFrustum& out_frustum) const;

  void SetShaderPermutationVariable(xiiStringView sName, xiiStringView sValue);

  void SetRenderPassProperty(xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value);
  void SetExtractorProperty(xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value);

  void ResetRenderPassProperties();
  void ResetExtractorProperties();

  void       SetRenderPassReadBackProperty(xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value);
  xiiVariant GetRenderPassReadBackProperty(xiiStringView sPassName, xiiStringView sPropertyName);
  bool       IsRenderPassReadBackPropertyExisting(xiiStringView sPassName, xiiStringView sPropertyName) const;

  /// \brief Pushes the view and camera data into the extracted data of the pipeline.
  ///
  /// Use xiiRenderWorld::GetDataIndexForExtraction() to update the data from the extraction thread. Can't be used if this view is currently extracted.
  /// Use xiiRenderWorld::GetDataIndexForRendering() to update the data from the render thread.
  void UpdateViewData(xiiUInt32 uiDataIndex);

  xiiTagSet m_IncludeTags;
  xiiTagSet m_ExcludeTags;

private:
  friend class xiiRenderWorld;
  friend class xiiMemoryUtils;

  xiiViewId       m_InternalId;
  xiiHashedString m_sName;

  xiiSharedPtr<xiiTask> m_pExtractTask;

  xiiWorld* m_pWorld = nullptr;

  xiiRenderPipelineResourceHandle m_hRenderPipeline;
  xiiUInt32                       m_uiRenderPipelineResourceDescriptionCounter = 0;
  xiiSharedPtr<xiiRenderPipeline> m_pRenderPipeline;
  xiiCamera*                      m_pCamera        = nullptr;
  const xiiCamera*                m_pCullingCamera = nullptr;
  const xiiCamera*                m_pLodCamera     = nullptr;


private:
  xiiRenderPipelineNodeInputPin m_PinRenderTarget0;
  xiiRenderPipelineNodeInputPin m_PinRenderTarget1;
  xiiRenderPipelineNodeInputPin m_PinRenderTarget2;
  xiiRenderPipelineNodeInputPin m_PinRenderTarget3;
  xiiRenderPipelineNodeInputPin m_PinDepthStencil;

private:
  void UpdateCachedMatrices() const;

  /// \brief Rebuilds pipeline if necessary and pushes double-buffered settings into the pipeline.
  void EnsureUpToDate();

  mutable xiiUInt32 m_uiLastCameraSettingsModification    = 0;
  mutable xiiUInt32 m_uiLastCameraOrientationModification = 0;
  mutable float     m_fLastViewportAspectRatio            = 1.0f;

  mutable xiiViewData m_Data;

  xiiInternal::RenderDataCache* m_pRenderDataCache = nullptr;

  xiiDynamicArray<xiiPermutationVar> m_PermutationVars;
  bool                               m_bPermutationVarsDirty = false;

  void ApplyPermutationVars();

  struct PropertyValue
  {
    xiiString  m_sObjectName;
    xiiString  m_sPropertyName;
    xiiVariant m_DefaultValue;
    xiiVariant m_CurrentValue;
    bool       m_bIsValid;
    bool       m_bIsDirty;
  };

  void SetProperty(xiiMap<xiiString, PropertyValue>& map, xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value);
  void SetReadBackProperty(xiiMap<xiiString, PropertyValue>& map, xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value);

  void ReadBackPassProperties();

  void ResetAllPropertyStates(xiiMap<xiiString, PropertyValue>& map);

  void ApplyRenderPassProperties();
  void ApplyExtractorProperties();

  void ApplyProperty(xiiReflectedClass* pObject, PropertyValue& data, xiiStringView sTypeName);

  xiiMap<xiiString, PropertyValue> m_PassProperties;
  xiiMap<xiiString, PropertyValue> m_PassReadBackProperties;
  xiiMap<xiiString, PropertyValue> m_ExtractorProperties;
};

#include <GraphicsCore/Pipeline/Implementation/View_inl.h>
