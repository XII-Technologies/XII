
XII_ALWAYS_INLINE xiiViewHandle xiiView::GetHandle() const
{
  return xiiViewHandle(m_InternalId);
}

XII_ALWAYS_INLINE xiiStringView xiiView::GetName() const
{
  return m_sName.GetView();
}

XII_ALWAYS_INLINE xiiWorld* xiiView::GetWorld()
{
  return m_pWorld;
}

XII_ALWAYS_INLINE const xiiWorld* xiiView::GetWorld() const
{
  return m_pWorld;
}

XII_ALWAYS_INLINE xiiGALSwapChainHandle xiiView::GetSwapChain() const
{
  return m_Data.m_hSwapChain;
}

XII_ALWAYS_INLINE const xiiGALRenderTargets& xiiView::GetRenderTargets() const
{
  return m_Data.m_renderTargets;
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

XII_ALWAYS_INLINE xiiEnum<xiiViewRenderMode> xiiView::GetViewRenderMode() const
{
  return m_Data.m_ViewRenderMode;
}

XII_ALWAYS_INLINE const xiiRectFloat& xiiView::GetViewport() const
{
  return m_Data.m_ViewPortRect;
}

XII_ALWAYS_INLINE const xiiViewData& xiiView::GetData() const
{
  UpdateCachedMatrices();
  return m_Data;
}

XII_FORCE_INLINE bool xiiView::IsValid() const
{
  return m_pWorld != nullptr && m_pRenderPipeline != nullptr && m_pCamera != nullptr && m_Data.m_ViewPortRect.HasNonZeroArea();
}

XII_ALWAYS_INLINE const xiiSharedPtr<xiiTask>& xiiView::GetExtractTask()
{
  return m_pExtractTask;
}

XII_FORCE_INLINE xiiResult xiiView::ComputePickingRay(float fScreenPosX, float fScreenPosY, xiiVec3& out_RayStartPos, xiiVec3& out_RayDir) const
{
  UpdateCachedMatrices();
  return m_Data.ComputePickingRay(fScreenPosX, fScreenPosY, out_RayStartPos, out_RayDir);
}

XII_FORCE_INLINE xiiResult xiiView::ComputeScreenSpacePos(const xiiVec3& vPoint, xiiVec3& out_vScreenPos) const
{
  UpdateCachedMatrices();
  return m_Data.ComputeScreenSpacePos(vPoint, out_vScreenPos);
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ProjectionMatrix[static_cast<int>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetInverseProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseProjectionMatrix[static_cast<int>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetViewMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewMatrix[static_cast<int>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetInverseViewMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewMatrix[static_cast<int>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetViewProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewProjectionMatrix[static_cast<int>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetInverseViewProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewProjectionMatrix[static_cast<int>(eye)];
}
