/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Rendering/Implementation/EditorViewportUtils.h>
#include <EditorEngineProcessFramework/Rendering/InfiniteGridPass.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInfiniteGridPass, 1, xiiRTTIDefaultAllocator<xiiInfiniteGridPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("ColorInput", m_InputColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Color")),
      XII_MEMBER_PROPERTY("SceneDepth", m_InputDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("ColorOutput", m_OutputColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Color")),
      XII_MEMBER_PROPERTY("LineWidth", m_fLineWidth)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.25f, 4.0f)),
      XII_MEMBER_PROPERTY("MinorOpacity", m_fMinorOpacity)->AddAttributes(new xiiDefaultValueAttribute(0.28f), new xiiClampValueAttribute(0.0f, 1.0f)),
      XII_MEMBER_PROPERTY("MajorOpacity", m_fMajorOpacity)->AddAttributes(new xiiDefaultValueAttribute(0.52f), new xiiClampValueAttribute(0.0f, 1.0f)),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiResult xiiInfiniteGridPass::ComputeSettings(const xiiView& view, float spacing, const xiiTransform& transform, bool global, xiiEditorGridConstants& settings)
{
  settings             = {};
  const auto* camera   = view.GetCamera();
  const auto& viewport = view.GetViewport();
  if (!xiiMath::IsFinite(spacing) || spacing <= 0 || !transform.IsValid() || transform.m_vScale.IsZero(0) || !camera || camera->GetCameraMode() == xiiCameraMode::None || !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0)
    return XII_FAILURE;
  xiiVec3 u      = transform.m_qRotation * xiiVec3::MakeAxisX();
  xiiVec3 v      = transform.m_qRotation * xiiVec3::MakeAxisY();
  xiiVec3 origin = transform.m_vPosition;
  if (global)
    origin.SetZero();
  if (camera->IsOrthographic() && global)
  {
    u = camera->GetCenterDirRight();
    v = camera->GetCenterDirUp();
    origin.SetZero();
  }
  XII_SUCCEED_OR_RETURN(u.NormalizeIfNotZero(xiiVec3::MakeAxisX()));
  XII_SUCCEED_OR_RETURN(v.NormalizeIfNotZero(xiiVec3::MakeAxisY()));
  const auto     normal   = u.CrossRH(v).GetNormalized();
  const auto     position = camera->GetCenterPosition();
  const xiiVec3d relative(double(position.x) - origin.x, double(position.y) - origin.y, double(position.z) - origin.z);
  const double   distance   = relative.Dot(xiiVec3d(normal.x, normal.y, normal.z));
  const double   projection = xiiMath::Abs(view.GetProjectionMatrix(xiiCameraEye::Left).Element(1, 1));
  if (!relative.IsValid() || !xiiMath::IsFinite(projection) || projection <= 0)
    return XII_FAILURE;
  const double pixel = 2 * (camera->IsOrthographic() ? 1.0 : xiiMath::Max(xiiMath::Abs(distance), double(camera->GetNearPlane()))) / (projection * viewport.height);
  float        ratio;
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::PixelRatio(view, ratio));
  const double level = xiiMath::Max(0.0, xiiMath::Log10(xiiMath::Max(pixel * ratio * 8 / spacing, 1.0)));
  const double step  = double(spacing) * xiiMath::Pow(10.0, xiiMath::Floor(level));
  if (!xiiMath::IsFinite(step) || step <= 0 || step > xiiMath::MaxValue<float>() / 100)
    return XII_FAILURE;
  const double alongU  = relative.Dot(xiiVec3d(u.x, u.y, u.z));
  const double alongV  = relative.Dot(xiiVec3d(v.x, v.y, v.z));
  settings.Plane       = xiiVec4(normal.x, normal.y, normal.z, static_cast<float>(-distance));
  settings.AxisU       = xiiVec4(u.x, u.y, u.z, static_cast<float>(alongU));
  settings.AxisV       = xiiVec4(v.x, v.y, v.z, static_cast<float>(alongV));
  const auto axisColor = [](const xiiVec3& direction) {
    const auto axis = direction.Abs();
    return axis.x >= axis.y && axis.x >= axis.z ? xiiVec4(0.48f, 0.12f, 0.10f, 1) : axis.y >= axis.z ? xiiVec4(0.12f, 0.38f, 0.18f, 1) :
                                                                                                       xiiVec4(0.12f, 0.22f, 0.48f, 1);
  };
  settings.AxisColorU = axisColor(global ? u : xiiVec3::MakeAxisX());
  settings.AxisColorV = axisColor(global ? v : xiiVec3::MakeAxisY());
  // Periodic phases are reduced before converting to floats. Shader frac never
  // receives huge world coordinates, even for small snap steps far from the origin.
  settings.Spacing  = xiiVec4(static_cast<float>(step), static_cast<float>(level - xiiMath::Floor(level)), static_cast<float>(xiiMath::Mod(alongU, step * 100) / step), static_cast<float>(xiiMath::Mod(alongV, step * 100) / step));
  auto rotationView = view.GetViewMatrix(xiiCameraEye::Left);
  rotationView.SetTranslationVector(xiiVec3::MakeZero());
  const auto projectionMatrix = view.GetProjectionMatrix(xiiCameraEye::Left) * rotationView;
  auto       inverse          = projectionMatrix;
  XII_SUCCEED_OR_RETURN(inverse.Invert(0));
  if (!inverse.IsValid() || !settings.Plane.IsValid() || !settings.Spacing.IsValid())
    return XII_FAILURE;
  settings.RelativeViewProjection        = projectionMatrix;
  settings.InverseRelativeViewProjection = inverse;
  settings.Appearance                    = xiiVec4(1.0f, 0.28f, 0.52f, 0);
  return XII_SUCCESS;
}

xiiResult xiiInfiniteGridPass::ConfigureView(xiiView& view, float spacing, const xiiTransform& transform, bool global)
{
  if (spacing == 0 || (transform.IsValid() && transform.m_vScale.IsZero(0)))
  {
    view.RemovePipelineParameter("Editor.Grid");
    return XII_SUCCESS;
  }
  xiiEditorGridConstants settings;
  if (ComputeSettings(view, spacing, transform, global, settings).Failed())
  {
    view.RemovePipelineParameter("Editor.Grid");
    return XII_FAILURE;
  }
  xiiDataBuffer snapshot;
  snapshot.SetCountUninitialized(sizeof(settings));
  xiiMemoryUtils::Copy(snapshot.GetData(), reinterpret_cast<const xiiUInt8*>(&settings), sizeof(settings));
  view.SetPipelineParameter("Editor.Grid", snapshot);
  return XII_SUCCESS;
}

namespace
{
  struct GridDrawData
  {
    xiiRenderGraphTextureHandle m_Color, m_Depth;
    xiiRenderGraphBufferHandle  m_Constants;
    xiiEditorGridConstants      m_Settings;
    bool                        m_bReady = false;
  };
} // namespace

xiiResult xiiInfiniteGridPass::Build(xiiRenderPipelinePassContext& context)
{
  auto        color     = context.GetInputTexture(m_InputColor);
  const auto* parameter = context.m_Parameters.GetValue("Editor.Grid");
  if (!parameter)
    return context.SetOutput(m_OutputColor, color);
  if (!parameter->IsA<xiiDataBuffer>() || parameter->Get<xiiDataBuffer>().GetCount() != sizeof(xiiEditorGridConstants))
    return context.ReportFailure("Invalid Editor grid snapshot.");
  xiiEditorGridConstants settings;
  xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&settings), parameter->Get<xiiDataBuffer>().GetData(), sizeof(settings));
  if (!xiiEditorViewportUtils::IsFinite(settings.RelativeViewProjection) || !xiiEditorViewportUtils::IsFinite(settings.InverseRelativeViewProjection) || !settings.Plane.IsValid() || !settings.AxisU.IsValid() || !settings.AxisV.IsValid() || !settings.Spacing.IsValid() || settings.Spacing.x <= 0)
    return context.ReportFailure("Invalid Editor grid description.");
  if (!xiiMath::IsFinite(m_fLineWidth) || m_fLineWidth <= 0 || !xiiMath::IsFinite(m_fMinorOpacity) || !xiiMath::IsFinite(m_fMajorOpacity) || m_fMinorOpacity < 0 || m_fMinorOpacity > 1 || m_fMajorOpacity < 0 || m_fMajorOpacity > 1)
    return XII_FAILURE;
  float ratio;
  if (!context.m_pView)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::PixelRatio(*context.m_pView, ratio));
  const auto* output = context.m_Graph.GetTextureDescription(color);
  if (!output || context.m_pView->GetViewport().height <= 0)
    return XII_FAILURE;
  settings.Appearance = xiiVec4(m_fLineWidth * ratio * output->m_Size.height / context.m_pView->GetViewport().height, m_fMinorOpacity, m_fMajorOpacity, 0);
  auto depth          = context.GetInputTexture(m_InputDepth);
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::ValidateTargets(context.m_Graph, color, depth));
  xiiGALGraphicsPipelineStateCreationDescription pipeline;
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::LoadPipeline("Shaders/Editor/InfiniteGrid.xiiShader", pipeline));
  context.m_Graph.AddPass<GridDrawData>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](GridDrawData& data, xiiRenderGraphBuilder& builder) {
    color = data.m_Color = builder.ReadWriteTexture(color, xiiGALResourceStateFlags::RenderTarget);
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Settings = settings;
    xiiGALBufferCreationDescription buffer;
    buffer.m_uiSize = sizeof(settings);
    buffer.m_BindFlags = xiiGALBindFlags::UniformBuffer;
    buffer.m_Usage = xiiGALResourceUsage::Dynamic;
    buffer.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
    data.m_Constants = builder.WriteBuffer(xiiStringBuilder(context.m_sPassName, ": Settings"), buffer, xiiGALResourceStateFlags::ConstantBuffer);
    builder.SetPassAllowMerge(false);
    builder.SetPassRenderPassManaged();
    builder.SetPassPreparation([&data](xiiRenderGraphPassContext& execution) {
      data.m_bReady = xiiEditorViewportUtils::Upload(execution, data.m_Constants, xiiArrayPtr<const xiiEditorGridConstants>(&data.m_Settings, 1), xiiGALResourceStateFlags::ConstantBuffer);
    }); }, [pipeline](const GridDrawData& data, xiiRenderGraphPassContext& execution) mutable {
    if (!data.m_bReady) return;
    pipeline.m_GraphicsPipeline.m_pRenderPass = execution.GetRenderPass();
    pipeline.m_GraphicsPipeline.m_uiSubpassIndex = execution.GetSubpassIndex();
    auto state = xiiGALPipelineCache::GetPipeline(pipeline);
    if (!state) { xiiLog::Error("Failed to create infinite grid pipeline."); return; }
    auto& commands = execution.GetCommandList();
    const auto size = execution.GetTexture(data.m_Color)->GetDescription().m_Size;
    commands.SetViewport({0, 0, float(size.width), float(size.height), 0, 1});
    commands.SetPipelineState(state.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiEditorGridConstants", execution.GetBuffer(data.m_Constants), xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceTextureView("SceneDepth", execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Succeeded()) commands.Draw({3}); });
  return context.SetOutput(m_OutputColor, color);
}
