#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>

#include <GraphicsFoundation/Device/SwapChain.h>

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/ViewData.h>

class xiiFrustum;
class xiiWorld;
class xiiRenderGraph;
class xiiExtractedRenderData;

/// \brief Encapsulates a view on the given world through the given camera
/// and rendered with the specified RenderPipeline into the given render target setup.
class XII_GRAPHICSCORE_DLL xiiView : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiView, xiiReflectedClass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiView);

private:
  /// \brief Use xiiRenderLoop::CreateView to create a view.
  xiiView();
  ~xiiView();

public:
  void          SetName(xiiStringView sName);
  xiiStringView GetName() const;

  /// \brief Sets the swapchain that this view will be rendering into. Can be invalid in case the render target is an off-screen buffer in which case SetRenderTargets needs to be called.
  /// Setting the swap-chain is necessary in order to acquire and present the image to the window.
  /// SetSwapChain and SetRenderTargets are mutually exclusive. Calling this function will reset the render targets.
  void             SetSwapChain(xiiGALSwapChain* pSwapChain);
  xiiGALSwapChain* GetSwapChain() const;

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

  const xiiViewData& GetData() const;

  bool IsValid() const;

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

  xiiTagSet m_IncludeTags;
  xiiTagSet m_ExcludeTags;

  xiiRenderGraph*         GetRenderGraph() { return m_pRenderGraph.Borrow(); }
  const xiiRenderGraph*   GetRenderGraph() const { return m_pRenderGraph.Borrow(); }

  xiiExtractedRenderData*       GetExtractedRenderData() { return m_pExtractedData.Borrow(); }
  const xiiExtractedRenderData* GetExtractedRenderData() const { return m_pExtractedData.Borrow(); }

private:
  friend class xiiRenderWorld;
  friend class xiiMemoryUtils;

  xiiHashedString m_sName;

  xiiUInt32                       m_uiRenderPipelineResourceDescriptionCounter = 0;
  xiiCamera*                      m_pCamera                                    = nullptr;
  const xiiCamera*                m_pCullingCamera                             = nullptr;
  const xiiCamera*                m_pLodCamera                                 = nullptr;

private:
  void UpdateCachedMatrices() const;

  mutable xiiUInt32 m_uiLastCameraSettingsModification    = 0;
  mutable xiiUInt32 m_uiLastCameraOrientationModification = 0;
  mutable float     m_fLastViewportAspectRatio            = 1.0f;

  mutable xiiViewData m_Data;

  xiiUniquePtr<xiiRenderGraph>         m_pRenderGraph;
  xiiUniquePtr<xiiExtractedRenderData> m_pExtractedData;
};

#include <GraphicsCore/Pipeline/Implementation/View_inl.h>
