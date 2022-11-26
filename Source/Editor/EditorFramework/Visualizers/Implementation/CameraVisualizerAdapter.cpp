#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiCameraVisualizerAdapter::xiiCameraVisualizerAdapter() {}

xiiCameraVisualizerAdapter::~xiiCameraVisualizerAdapter() {}

void xiiCameraVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");

  const xiiCameraVisualizerAttribute* pAttr = static_cast<const xiiCameraVisualizerAttribute*>(m_pVisualizerAttr);

  m_hBoxGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::LineBox, xiiColor::DodgerBlue, xiiGizmoFlags::Visualizer | xiiGizmoFlags::ShowInOrtho);
  m_hFrustumGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Frustum, xiiColor::DodgerBlue, xiiGizmoFlags::Visualizer | xiiGizmoFlags::ShowInOrtho);
  m_hNearPlaneGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::LineRect, xiiColor::LightBlue, xiiGizmoFlags::Visualizer | xiiGizmoFlags::ShowInOrtho);
  m_hFarPlaneGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::LineRect, xiiColor::PaleVioletRed, xiiGizmoFlags::Visualizer | xiiGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hBoxGizmo);
  pAssetDocument->AddSyncObject(&m_hFrustumGizmo);
  pAssetDocument->AddSyncObject(&m_hNearPlaneGizmo);
  pAssetDocument->AddSyncObject(&m_hFarPlaneGizmo);

  m_hBoxGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hFrustumGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hNearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hFarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
}

void xiiCameraVisualizerAdapter::Update()
{

  const xiiCameraVisualizerAttribute* pAttr           = static_cast<const xiiCameraVisualizerAttribute*>(m_pVisualizerAttr);
  xiiObjectAccessorBase*              pObjectAccessor = GetObjectAccessor();

  float    fNearPlane = 1.0f;
  float    fFarPlane  = 10.0f;
  xiiInt32 iMode      = 0;

  if (!pAttr->GetModeProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetModeProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiInt32>(), "Invalid property bound to xiiCameraVisualizerAttribute 'mode'");
    iMode = value.ConvertTo<xiiInt32>();
  }

  if (!pAttr->GetNearPlaneProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetNearPlaneProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to xiiCameraVisualizerAttribute 'near plane'");
    fNearPlane = value.ConvertTo<float>();
  }

  if (!pAttr->GetFarPlaneProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFarPlaneProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to xiiCameraVisualizerAttribute 'far plane'");
    fFarPlane = value.ConvertTo<float>();
  }

  if (iMode == xiiCameraMode::OrthoFixedHeight || iMode == xiiCameraMode::OrthoFixedWidth)
  {
    float fDimensions = 1.0f;

    if (!pAttr->GetOrthoDimProperty().IsEmpty())
    {
      xiiVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOrthoDimProperty()), value);

      XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to xiiCameraVisualizerAttribute 'ortho dim'");
      fDimensions = value.ConvertTo<float>();
    }

    {
      const float fRange = fFarPlane - fNearPlane;

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fRange, fDimensions, fDimensions);
      m_LocalTransformFrustum.m_vPosition.Set(fNearPlane + fRange * 0.5f, 0, 0);
    }

    m_hBoxGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hFrustumGizmo.SetVisible(false);
    m_hNearPlaneGizmo.SetVisible(false);
    m_hFarPlaneGizmo.SetVisible(false);

    m_LocalTransformNearPlane.SetIdentity();
    m_LocalTransformFarPlane.SetIdentity();
  }
  else
  {
    float fFOV = 45.0f;

    if (!pAttr->GetFovProperty().IsEmpty())
    {
      xiiVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFovProperty()), value);

      XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to xiiCameraVisualizerAttribute 'fov'");
      fFOV = value.ConvertTo<float>();
    }

    {
      const float fAngleScale    = xiiMath::Tan(xiiAngle::Degree(fFOV) * 0.5f);
      const float fFrustumScale  = xiiMath::Min(fFarPlane, 10.0f);
      const float fFarPlaneScale = xiiMath::Min(fFarPlane, 9.0f);
      ;

      // indicate whether the shown far plane is the actual distance, or just the maximum visualization distance
      m_hFarPlaneGizmo.SetColor(fFarPlane > 9.0f ? xiiColor::DodgerBlue : xiiColor::PaleVioletRed);

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fFrustumScale, fAngleScale * fFrustumScale, fAngleScale * fFrustumScale);
      m_LocalTransformFrustum.m_vPosition.Set(0, 0, 0);

      m_LocalTransformNearPlane.m_qRotation.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(90));
      m_LocalTransformNearPlane.m_vScale.Set(fAngleScale * fNearPlane, fAngleScale * fNearPlane, 1);
      m_LocalTransformNearPlane.m_vPosition.Set(fNearPlane, 0, 0);

      m_LocalTransformFarPlane.m_qRotation.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(90));
      m_LocalTransformFarPlane.m_vScale.Set(fAngleScale * fFarPlaneScale, fAngleScale * fFarPlaneScale, 1);
      m_LocalTransformFarPlane.m_vPosition.Set(fFarPlaneScale, 0, 0);
    }

    m_hBoxGizmo.SetVisible(false);
    m_hFrustumGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hNearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hFarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  }
}

void xiiCameraVisualizerAdapter::UpdateGizmoTransform()
{
  xiiTransform t = GetObjectTransform();
  m_hBoxGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_hFrustumGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_hNearPlaneGizmo.SetTransformation(t * m_LocalTransformNearPlane);
  m_hFarPlaneGizmo.SetTransformation(t * m_LocalTransformFarPlane);
}
