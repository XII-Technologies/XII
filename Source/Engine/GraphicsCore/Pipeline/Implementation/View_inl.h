
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

XII_ALWAYS_INLINE xiiGALSwapChain* xiiView::GetSwapChain() const
{
  return m_Data.m_pSwapChain;
}

XII_ALWAYS_INLINE const xiiRenderTargets& xiiView::GetRenderTargets() const
{
  return m_Data.m_RenderTargets;
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
  return m_pWorld != nullptr && m_pCamera != nullptr && m_Data.m_ViewPortRect.HasNonZeroArea();
}

XII_ALWAYS_INLINE const xiiSharedPtr<xiiTask>& xiiView::GetExtractTask()
{
  return m_pExtractTask;
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
  return m_Data.m_ProjectionMatrix[static_cast<xiiInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetInverseProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseProjectionMatrix[static_cast<xiiInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetViewMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewMatrix[static_cast<xiiInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetInverseViewMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewMatrix[static_cast<xiiInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetViewProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewProjectionMatrix[static_cast<xiiInt32>(eye)];
}

XII_ALWAYS_INLINE const xiiMat4& xiiView::GetInverseViewProjectionMatrix(xiiCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewProjectionMatrix[static_cast<xiiInt32>(eye)];
}
