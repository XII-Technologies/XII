/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_ALWAYS_INLINE xiiWorld* xiiView::GetWorld() const
{
  return m_pWorld;
}

XII_ALWAYS_INLINE xiiViewHandle xiiView::GetHandle() const
{
  return xiiViewHandle(m_InternalId);
}

XII_ALWAYS_INLINE xiiStringView xiiView::GetName() const
{
  return m_sName.GetView();
}

XII_ALWAYS_INLINE void xiiView::SetName(xiiStringView sName)
{
  m_sName.Assign(sName);
}

XII_ALWAYS_INLINE xiiGALTextureView* xiiView::GetRenderTargetView() const
{
  return m_pRenderTargetView;
}

XII_ALWAYS_INLINE void xiiView::SetRenderTargetView(xiiGALTextureView* pRenderTargetView)
{
  m_pRenderTargetView = pRenderTargetView;
}

XII_ALWAYS_INLINE const xiiGALSwapChain* xiiView::GetSwapChain() const
{
  return m_pSwapChain;
}

XII_ALWAYS_INLINE void xiiView::SetSwapChain(const xiiGALSwapChain* pSwapChain)
{
  m_pSwapChain = pSwapChain;
}

XII_ALWAYS_INLINE void xiiView::SetCamera(xiiCamera* pCamera)
{
  m_pCamera = pCamera;
}

XII_ALWAYS_INLINE xiiCamera* xiiView::GetCamera()
{
  return m_pCamera;
}

XII_ALWAYS_INLINE const xiiCamera* xiiView::GetCamera() const
{
  return m_pCamera;
}

XII_ALWAYS_INLINE void xiiView::SetCullingCamera(const xiiCamera* pCamera)
{
  m_pCullingCamera = pCamera;
}

XII_ALWAYS_INLINE const xiiCamera* xiiView::GetCullingCamera() const
{
  return m_pCullingCamera != nullptr ? m_pCullingCamera : m_pCamera;
}

XII_ALWAYS_INLINE void xiiView::SetLodCamera(const xiiCamera* pCamera)
{
  m_pLodCamera = pCamera;
}

XII_ALWAYS_INLINE const xiiCamera* xiiView::GetLodCamera() const
{
  return m_pLodCamera != nullptr ? m_pLodCamera : m_pCamera;
}

XII_ALWAYS_INLINE xiiEnum<xiiCameraUsageHint> xiiView::GetCameraUsageHint() const
{
  return m_Data.m_CameraUsageHint;
}

XII_ALWAYS_INLINE void xiiView::SetCameraUsageHint(xiiEnum<xiiCameraUsageHint> hint)
{
  m_Data.m_CameraUsageHint = hint;
}

XII_ALWAYS_INLINE xiiEnum<xiiViewRenderMode> xiiView::GetViewRenderMode() const
{
  return m_Data.m_ViewRenderMode;
}

XII_ALWAYS_INLINE void xiiView::SetViewRenderMode(xiiEnum<xiiViewRenderMode> value)
{
  m_Data.m_ViewRenderMode = value;
}

XII_ALWAYS_INLINE const xiiRectFloat& xiiView::GetViewport() const
{
  return m_Data.m_ViewPortRect;
}

XII_ALWAYS_INLINE void xiiView::SetViewport(const xiiRectFloat& viewport)
{
  m_Data.m_ViewPortRect = viewport;
  UpdateRenderResolutionState();
}

XII_ALWAYS_INLINE void xiiView::SetRenderGraphBuilder(RenderGraphBuilder builder)
{
  m_RenderGraphBuilder = std::move(builder);
  ++m_uiRenderGraphBuilderVersion;
}

XII_ALWAYS_INLINE const xiiView::RenderGraphBuilder& xiiView::GetRenderGraphBuilder() const
{
  return m_RenderGraphBuilder;
}

XII_ALWAYS_INLINE xiiUInt32 xiiView::GetRenderGraphBuilderVersion() const
{
  return m_uiRenderGraphBuilderVersion;
}

XII_ALWAYS_INLINE const xiiViewData& xiiView::GetData() const
{
  UpdateCachedMatrices();

  return m_Data;
}

XII_FORCE_INLINE bool xiiView::IsValid() const
{
  return m_pCamera != nullptr && m_Data.m_ViewPortRect.HasNonZeroArea();
}

XII_FORCE_INLINE xiiResult xiiView::ComputePickingRay(float fScreenPosX, float fScreenPosY, xiiVec3& out_vRayStartPos, xiiVec3& out_vRayDir) const
{
  UpdateCachedMatrices();

  return m_Data.ComputePickingRay(fScreenPosX, fScreenPosY, out_vRayStartPos, out_vRayDir);
}

XII_FORCE_INLINE xiiResult xiiView::ComputeScreenSpacePos(const xiiVec3& vPoint, xiiVec3& out_vScreenPos) const
{
  UpdateCachedMatrices();

  return m_Data.ComputeScreenSpacePos(vPoint, out_vScreenPos);
}

XII_FORCE_INLINE xiiResult xiiView::ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vWorldPos) const
{
  UpdateCachedMatrices();

  return m_Data.ComputeWorldSpacePos(fNormalizedScreenPosX, fNormalizedScreenPosY, out_vWorldPos);
}

XII_FORCE_INLINE void xiiView::ConvertScreenPixelPosToNormalizedPos(xiiVec3& inout_vPixelPos)
{
  m_Data.ConvertScreenPixelPosToNormalizedPos(inout_vPixelPos);
}

XII_FORCE_INLINE void xiiView::ConvertScreenNormalizedPosToPixelPos(xiiVec3& inout_vNormalizedPos)
{
  m_Data.ConvertScreenNormalizedPosToPixelPos(inout_vNormalizedPos);
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();

  return m_Data.m_ProjectionMatrix[static_cast<xiiUInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetInverseProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();

  return m_Data.m_InverseProjectionMatrix[static_cast<xiiUInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetViewMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();

  return m_Data.m_ViewMatrix[static_cast<xiiUInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetInverseViewMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();

  return m_Data.m_InverseViewMatrix[static_cast<xiiUInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetViewProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();

  return m_Data.m_ViewProjectionMatrix[static_cast<xiiUInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetInverseViewProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();

  return m_Data.m_InverseViewProjectionMatrix[static_cast<xiiUInt32>(eye)];
}
