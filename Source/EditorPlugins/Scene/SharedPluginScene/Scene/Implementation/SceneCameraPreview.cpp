/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/World/World.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <SharedPluginScene/Scene/SceneCameraPreview.h>

xiiSceneCameraPreview::xiiSceneCameraPreview(xiiWorld& world) : m_World(world) {}

xiiSceneCameraPreview::~xiiSceneCameraPreview()
{
  XII_LOCK(m_World.GetWriteMarker());
  if (auto* module = m_World.GetModule<xiiRenderWorldModule>())
    module->DestroyView(m_hView);
}

xiiView* xiiSceneCameraPreview::GetView() const
{
  xiiView* view = nullptr;
  if (auto* module = m_World.GetModule<xiiRenderWorldModule>())
    module->TryGetView(m_hView, view);
  return view;
}

void xiiSceneCameraPreview::SuspendRendering()
{
  XII_LOCK(m_World.GetWriteMarker());
  if (auto* view = GetView())
    view->SetRenderEnabled(false);
}

xiiResult xiiSceneCameraPreview::EnsureView()
{
  if (GetView())
    return XII_SUCCESS;

  if (!m_pTarget)
  {
    auto device = xiiGALDevice::GetDefaultDevice();
    if (!device)
      return XII_FAILURE;
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = {256, 144};
    description.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;

    // A newly selected camera can be composited before its first render. Initialize that image
    // rather than sampling undefined GPU memory; this upload happens only on first allocation.
    xiiDynamicArray<xiiUInt8> pixels;
    pixels.SetCount(256 * 144 * 4);
    xiiMemoryUtils::ZeroFill(pixels.GetData(), pixels.GetCount());
    for (xiiUInt32 i = 3; i < pixels.GetCount(); i += 4)
      pixels[i] = 255;
    const xiiGALTextureSubResourceData subresource(xiiConstByteBlobPtr(pixels.GetData(), pixels.GetCount()), 256 * 4, pixels.GetCount());
    const xiiGALTextureData            data(xiiMakeArrayPtr(&subresource, 1));
    m_pTarget = device->CreateTexture(description, &data);
    if (!m_pTarget)
      return XII_FAILURE;
    m_pTarget->SetDebugName("Scene camera preview");
  }

  xiiView* view = nullptr;
  m_hView       = m_World.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Scene Camera Preview", view);
  view->SetCamera(&m_Camera);
  view->SetCameraUsageHint(xiiCameraUsageHint::RenderTarget);
  view->SetRenderTargetView(m_pTarget->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  view->SetViewport(xiiRectFloat(0, 0, 256, 144));
  view->SetRenderEnabled(false);
  return XII_SUCCESS;
}

xiiResult xiiSceneCameraPreview::Update(const xiiView& editorView, const xiiCameraComponent& camera)
{
  XII_LOCK(m_World.GetWriteMarker());
  SuspendRendering();
  if (editorView.GetWorld() != &m_World || camera.GetWorld() != &m_World || !camera.IsActiveAndInitialized() || editorView.GetCameraUsageHint() != xiiCameraUsageHint::EditorView)
    return XII_FAILURE;

  xiiCameraMode::Enum mode      = camera.GetCameraMode();
  float               dimension = camera.GetFieldOfView();
  if (mode == xiiCameraMode::OrthoFixedHeight || mode == xiiCameraMode::OrthoFixedWidth)
    dimension = camera.GetOrthoDimension();
  else if (mode == xiiCameraMode::Stereo)
  {
    mode      = xiiCameraMode::PerspectiveFixedFovY;
    dimension = 45.0f;
  }
  else if (mode != xiiCameraMode::PerspectiveFixedFovX && mode != xiiCameraMode::PerspectiveFixedFovY)
    return XII_FAILURE;

  if (!xiiMath::IsFinite(dimension) || dimension <= 0 || !xiiMath::IsFinite(camera.GetNearPlane()) || !xiiMath::IsFinite(camera.GetFarPlane()) ||
      ((mode == xiiCameraMode::PerspectiveFixedFovX || mode == xiiCameraMode::PerspectiveFixedFovY) && (dimension >= 180 || camera.GetNearPlane() <= 0)))
    return XII_FAILURE;
  const float farPlane = xiiMath::Max(camera.GetNearPlane() + 0.00001f, camera.GetFarPlane());
  const float exposure = camera.GetExposure();
  if (farPlane <= camera.GetNearPlane() || !xiiMath::IsFinite(exposure) || exposure <= 0 || !editorView.GetRenderPipelineResource().IsValid())
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(EnsureView());
  auto* view = GetView();
  if (m_hCamera != camera.GetHandle() || m_Camera.GetCameraMode() != mode || m_Camera.GetFovOrDim() != dimension ||
      m_Camera.GetNearPlane() != camera.GetNearPlane() || m_Camera.GetFarPlane() != farPlane)
    view->InvalidateTemporalHistory();
  m_hCamera = camera.GetHandle();
  m_Camera.SetCameraMode(mode, dimension, camera.GetNearPlane(), farPlane);
  const auto* owner    = camera.GetOwner();
  const auto  position = owner->GetGlobalPosition();
  m_Camera.LookAt(position, position + owner->GetGlobalDirForwards(), owner->GetGlobalDirUp());
  m_Camera.SetExposure(exposure);
  view->m_IncludeTags = camera.m_IncludeTags;
  view->m_ExcludeTags = camera.m_ExcludeTags;
  view->m_ExcludeTags.SetByName("Editor");
  view->m_ExcludeTags.SetByName("EditorHidden");
  view->SetRenderScale(camera.GetRenderScale());
  view->SetRenderPipelineResource(editorView.GetRenderPipelineResource());
  view->SetRenderEnabled(true);
  xiiDebugRenderer::Draw2DRectangle(editorView, xiiRectFloat(20, 20, 256, 144), 0, xiiColor::White, m_pTarget->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiVec2(1), xiiDebugRendererDepthMode::Overlay);
  return XII_SUCCESS;
}
