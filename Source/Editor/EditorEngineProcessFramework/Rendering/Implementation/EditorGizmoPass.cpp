/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoGeometry.h>
#include <EditorEngineProcessFramework/Rendering/EditorGizmoPass.h>
#include <EditorEngineProcessFramework/Rendering/Implementation/EditorViewportUtils.h>
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Resources/Texture.h>

static_assert(static_cast<xiiUInt32>(xiiEngineGizmoHandleType::RotationRing) == XII_EDITOR_GIZMO_RING);
static_assert(static_cast<xiiUInt32>(xiiEngineGizmoHandleType::ScaleUniform) == XII_EDITOR_GIZMO_UNIFORM);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorGizmoPass, 1, xiiRTTIDefaultAllocator<xiiEditorGizmoPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("ColorInput", m_InputColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Color")),
      XII_MEMBER_PROPERTY("SceneDepth", m_InputDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("ColorOutput", m_OutputColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Color")),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  struct Primitive
  {
    xiiSharedPtr<xiiGALBuffer> m_pVertices;
    xiiUInt32                  m_uiVertexCount = 0;
  };

  void Stroke(xiiDynamicArray<xiiEditorGizmoVertex>& vertices, const xiiVec3& a, const xiiVec3& b)
  {
    for (xiiUInt32 corner : {0, 1, 2, 2, 1, 3})
    {
      auto& vertex     = vertices.ExpandAndGetRef();
      vertex.Position  = xiiVec4(a.x, a.y, a.z, 1);
      vertex.Auxiliary = xiiVec4(b.x, b.y, b.z, float(corner));
    }
  }

  void Circle(xiiDynamicArray<xiiEditorGizmoVertex>& vertices, const xiiVec3& center, const xiiVec3& u, const xiiVec3& v, bool half = false)
  {
    const xiiUInt32 count = half ? 32 : 64;
    for (xiiUInt32 i = 0; i < count; ++i)
    {
      auto point = [&](xiiUInt32 index) {
        const auto angle = xiiAngle::MakeFromDegree(360.0f * index / 64);
        return center + u * xiiMath::Cos(angle) + v * xiiMath::Sin(angle);
      };
      Stroke(vertices, point(i), point(i + 1));
    }
  }

  xiiResult CreatePrimitive(xiiEngineGizmoHandleType type, xiiStringView file, bool wire, Primitive& result)
  {
    xiiDynamicArray<xiiEditorGizmoVertex> vertices;
    if (type == xiiEngineGizmoHandleType::RotationRing)
    {
      // The vertex shader generates the ring, ticks and delta guides. Only binding
      // storage is needed here; no per-ring CPU render geometry is generated.
      vertices.SetCount(1);
      vertices[0]            = {};
      result.m_uiVertexCount = 154 * 6;
    }
    else if (wire && (type == xiiEngineGizmoHandleType::Sphere || type == xiiEngineGizmoHandleType::HalfSphereZ || type == xiiEngineGizmoHandleType::CylinderZ))
    {
      if (type == xiiEngineGizmoHandleType::CylinderZ)
      {
        for (float z : {-0.5f, 0.5f})
          Circle(vertices, xiiVec3(0, 0, z), xiiVec3::MakeAxisX(), xiiVec3::MakeAxisY());
        for (auto xy : {xiiVec3(1, 0, 0), xiiVec3(-1, 0, 0), xiiVec3(0, 1, 0), xiiVec3(0, -1, 0)})
          Stroke(vertices, xy + xiiVec3(0, 0, -0.5f), xy + xiiVec3(0, 0, 0.5f));
      }
      else
      {
        Circle(vertices, xiiVec3::MakeZero(), xiiVec3::MakeAxisX(), xiiVec3::MakeAxisY());
        Circle(vertices, xiiVec3::MakeZero(), xiiVec3::MakeAxisX(), xiiVec3::MakeAxisZ(), type == xiiEngineGizmoHandleType::HalfSphereZ);
        Circle(vertices, xiiVec3::MakeZero(), xiiVec3::MakeAxisY(), xiiVec3::MakeAxisZ(), type == xiiEngineGizmoHandleType::HalfSphereZ);
      }
    }
    else
    {
      xiiGizmoGeometry primitive;
      XII_SUCCEED_OR_RETURN(primitive.Build(type, file));
      const auto& geometry = primitive.GetGeometry();
      const auto& points   = geometry.GetVertices();
      if (wire)
      {
        xiiHashSet<xiiUInt64> edges;
        auto                  append = [&](xiiUInt32 a, xiiUInt32 b) {
          const auto key = (xiiUInt64(xiiMath::Min(a, b)) << 32) | xiiMath::Max(a, b);
          if (!edges.Insert(key))
            Stroke(vertices, points[a].m_vPosition, points[b].m_vPosition);
        };
        for (const auto& face : geometry.GetPolygons())
          for (xiiUInt32 i = 0; i < 3; ++i)
            append(face.m_Vertices[i], face.m_Vertices[(i + 1) % 3]);
        for (const auto& line : geometry.GetLines())
          append(line.m_uiStartVertex, line.m_uiEndVertex);
      }
      else
      {
        for (const auto& face : geometry.GetPolygons())
        {
          const auto a = points[face.m_Vertices[0]].m_vPosition, b = points[face.m_Vertices[1]].m_vPosition, c = points[face.m_Vertices[2]].m_vPosition;
          auto       normal = (b - a).CrossRH(c - a);
          if (normal.NormalizeIfNotZero(xiiVec3::MakeAxisZ()).Failed())
            continue;
          for (auto position : {a, b, c})
          {
            auto& vertex     = vertices.ExpandAndGetRef();
            vertex.Position  = xiiVec4(position.x, position.y, position.z, 0);
            vertex.Auxiliary = xiiVec4(normal.x, normal.y, normal.z, 0);
          }
        }
        for (const auto& line : geometry.GetLines())
          Stroke(vertices, points[line.m_uiStartVertex].m_vPosition, points[line.m_uiEndVertex].m_vPosition);
      }
    }
    if (vertices.IsEmpty())
      return XII_FAILURE;
    if (type == xiiEngineGizmoHandleType::TranslationAxis || type == xiiEngineGizmoHandleType::TranslationPlane)
    {
      const auto first = vertices.GetCount();
      Stroke(vertices, xiiVec3::MakeZero(), xiiVec3::MakeZero());
      for (xiiUInt32 i = first; i < vertices.GetCount(); ++i)
        vertices[i].Position.w = 3;
      const auto marker = vertices.GetCount();
      Stroke(vertices, xiiVec3::MakeZero(), xiiVec3::MakeZero());
      for (xiiUInt32 i = marker; i < vertices.GetCount(); ++i)
        vertices[i].Position.w = 5;
    }
    if (type != xiiEngineGizmoHandleType::RotationRing)
      result.m_uiVertexCount = vertices.GetCount();
    xiiGALBufferCreationDescription description;
    description.m_uiSize              = vertices.GetCount() * sizeof(xiiEditorGizmoVertex);
    description.m_uiElementByteStride = sizeof(xiiEditorGizmoVertex);
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Immutable;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    const xiiGALBufferData initial(vertices.GetData(), description.m_uiSize);
    result.m_pVertices = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description, &initial);
    return result.m_pVertices ? XII_SUCCESS : XII_FAILURE;
  }

  struct Batch
  {
    xiiRenderGraphBufferHandle m_Vertices;
    xiiUInt32                  m_uiFirst = 0, m_uiCount = 0, m_uiVertices = 0;
  };
  struct DrawData
  {
    xiiRenderGraphTextureHandle             m_Color, m_Depth, m_SceneDepth;
    xiiRenderGraphBufferHandle              m_Instances, m_Constants;
    xiiEditorGizmoConstants                 m_Settings;
    xiiDynamicArray<xiiEditorGizmoInstance> m_Data;
    xiiDynamicArray<Batch>                  m_Batches;
    bool                                    m_bReady = false;
  };
  struct ImageData
  {
    xiiRenderGraphTextureHandle m_Color, m_Depth;
  };
} // namespace

struct xiiEditorGizmoPass::State
{
  xiiMap<xiiString, Primitive> m_Primitives;
};
xiiEditorGizmoPass::xiiEditorGizmoPass()  = default;
xiiEditorGizmoPass::~xiiEditorGizmoPass() = default;

xiiResult xiiEditorGizmoPass::Submit(xiiView& view, xiiArrayPtr<const xiiEditorGizmoInstance> instances, const xiiVariantArray& customMeshes)
{
  view.RemovePipelineParameter("Editor.Gizmos");
  view.RemovePipelineParameter("Editor.GizmoMeshes");
  if (instances.GetCount() != customMeshes.GetCount() || instances.GetCount() > 65536)
    return XII_FAILURE;
  if (instances.IsEmpty())
    return XII_SUCCESS;
  xiiDataBuffer snapshot;
  snapshot.SetCountUninitialized(instances.GetCount() * sizeof(xiiEditorGizmoInstance));
  xiiMemoryUtils::Copy(snapshot.GetData(), reinterpret_cast<const xiiUInt8*>(instances.GetPtr()), snapshot.GetCount());
  view.SetPipelineParameter("Editor.Gizmos", snapshot);
  view.SetPipelineParameter("Editor.GizmoMeshes", customMeshes);
  return XII_SUCCESS;
}

xiiResult xiiEditorGizmoPass::Build(xiiRenderPipelinePassContext& context)
{
  auto        color     = context.GetInputTexture(m_InputColor);
  const auto* parameter = context.m_Parameters.GetValue("Editor.Gizmos");
  if (!parameter)
    return context.SetOutput(m_OutputColor, color);
  const auto* meshes = context.m_Parameters.GetValue("Editor.GizmoMeshes");
  if (!context.m_pView || !parameter->IsA<xiiDataBuffer>() || !meshes || !meshes->IsA<xiiVariantArray>())
    return XII_FAILURE;
  const auto& bytes = parameter->Get<xiiDataBuffer>();
  const auto& files = meshes->Get<xiiVariantArray>();
  if (bytes.GetCount() % sizeof(xiiEditorGizmoInstance) || bytes.GetCount() / sizeof(xiiEditorGizmoInstance) != files.GetCount() || files.GetCount() > 65536)
    return XII_FAILURE;
  if (files.IsEmpty())
    return context.SetOutput(m_OutputColor, color);
  auto depth = context.GetInputTexture(m_InputDepth);
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::ValidateTargets(context.m_Graph, color, depth));
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  xiiDynamicArray<xiiEditorGizmoInstance> instances;
  instances.SetCountUninitialized(files.GetCount());
  xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(instances.GetData()), bytes.GetData(), bytes.GetCount());
  xiiDynamicArray<xiiString> keys;
  for (xiiUInt32 i = 0; i < instances.GetCount(); ++i)
  {
    const auto& instance = instances[i];
    if (instance.Identity.x > xiiUInt32(xiiEngineGizmoHandleType::ScaleUniform) || !xiiEditorViewportUtils::IsFinite(instance.Transform) || !instance.GuideStart.IsValid() || !instance.Color.IsValid() || !instance.Parameters.IsValid() || instance.Parameters.x <= 0 || instance.Parameters.w <= 0 || !files[i].IsA<xiiString>())
      return context.ReportFailure("Invalid Editor gizmo description.");
    const auto       file = files[i].Get<xiiString>();
    xiiStringBuilder key;
    key.SetFormat("{}/{}/{}", instance.Identity.x, instance.Identity.y & XII_EDITOR_GIZMO_WIRE, file);
    keys.PushBack(key);
    if (!m_pState->m_Primitives.Contains(key))
    {
      Primitive primitive;
      XII_SUCCEED_OR_RETURN(CreatePrimitive(static_cast<xiiEngineGizmoHandleType>(instance.Identity.x), file, (instance.Identity.y & XII_EDITOR_GIZMO_WIRE) != 0, primitive));
      m_pState->m_Primitives.Insert(key, std::move(primitive));
    }
  }
  xiiGALGraphicsPipelineStateCreationDescription pipeline, composite;
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::LoadPipeline("Shaders/Editor/Gizmos.xiiShader", pipeline));
  pipeline.m_GraphicsPipeline.m_SampleDescription.m_uiCount = 4;
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::LoadPipeline("Shaders/Editor/GizmoComposite.xiiShader", composite));
  auto&                            graph = context.m_Graph;
  const auto                       size  = graph.GetTextureDescription(color)->m_Size;
  xiiGALTextureCreationDescription image;
  image.m_Type           = xiiGALResourceDimension::Texture2D;
  image.m_Size           = size;
  image.m_Format         = xiiGALResourceFormat::RGBA16Float;
  image.m_uiSampleCount  = 4;
  image.m_BindFlags      = xiiGALBindFlags::RenderTarget;
  auto depthImage        = image;
  depthImage.m_Format    = xiiGALResourceFormat::D32Float;
  depthImage.m_BindFlags = xiiGALBindFlags::DepthStencil;
  xiiRenderGraphTextureHandle overlay, handleDepth;
  graph.AddPass<ImageData>(xiiStringBuilder(context.m_sPassName, ": Clear"), xiiGALCommandQueueFlags::Graphics, [&](ImageData& data, xiiRenderGraphBuilder& builder) {
    overlay = data.m_Color = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, ": Overlay"), image, xiiGALResourceStateFlags::RenderTarget);
    handleDepth = data.m_Depth = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, ": Handle depth"), depthImage, xiiGALResourceStateFlags::DepthWrite);
    builder.SetPassRenderPassManaged(); }, [](const ImageData& data, xiiRenderGraphPassContext& execution) {
    auto& commands = execution.GetCommandList();
    commands.ClearRenderTargetView(execution.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    commands.ClearDepthStencilView(execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0, 0); });
  for (xiiUInt32 layer = 0; layer < 3; ++layer)
  {
    xiiDynamicArray<xiiUInt32> order;
    for (xiiUInt32 i = 0; i < instances.GetCount(); ++i)
    {
      const auto priority = !(instances[i].Identity.y & XII_EDITOR_GIZMO_ON_TOP) ? 0U : instances[i].Identity.x == XII_EDITOR_GIZMO_UNIFORM ? 2U :
                                                                                                                                              1U;
      if (priority == layer)
        order.PushBack(i);
    }
    if (order.IsEmpty())
      continue;
    order.Sort([&](xiiUInt32 a, xiiUInt32 b) { return keys[a] == keys[b] ? instances[a].Identity.z < instances[b].Identity.z : keys[a] < keys[b]; });
    if (layer)
    {
      graph.AddPass<ImageData>(xiiStringBuilder(context.m_sPassName, ": Overlay priority"), xiiGALCommandQueueFlags::Graphics, [&](ImageData& data, xiiRenderGraphBuilder& builder) {
        handleDepth = data.m_Depth = builder.ReadWriteTexture(handleDepth, xiiGALResourceStateFlags::DepthWrite);
        builder.SetPassRenderPassManaged(); }, [](const ImageData& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().ClearDepthStencilView(execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0, 0); });
    }
    graph.AddPass<DrawData>(xiiStringBuilder(context.m_sPassName, layer ? ": On top" : ": Depth tested"), xiiGALCommandQueueFlags::Graphics, [&](DrawData& data, xiiRenderGraphBuilder& builder) {
      overlay = data.m_Color = builder.ReadWriteTexture(overlay, xiiGALResourceStateFlags::RenderTarget);
      handleDepth = data.m_Depth = builder.ReadWriteTexture(handleDepth, xiiGALResourceStateFlags::DepthWrite);
      data.m_SceneDepth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
      auto rotation = context.m_pView->GetViewMatrix(xiiCameraEye::Left);
      rotation.SetTranslationVector(xiiVec3::MakeZero());
      data.m_Settings.RelativeViewProjection = context.m_pView->GetProjectionMatrix(xiiCameraEye::Left) * rotation;
      data.m_Settings.Viewport = xiiVec4(float(size.width), float(size.height), 1.0f / size.width, 1.0f / size.height);
      xiiString previous;
      for (auto index : order)
      {
        const auto position = data.m_Data.GetCount();
        data.m_Data.PushBack(instances[index]);
        data.m_Data.PeekBack().Parameters.w *= float(size.height) / context.m_pView->GetViewport().height;
        if (previous == keys[index] && !data.m_Batches.IsEmpty()) { ++data.m_Batches.PeekBack().m_uiCount; continue; }
        previous = keys[index];
        const auto& primitive = m_pState->m_Primitives[keys[index]];
        auto& batch = data.m_Batches.ExpandAndGetRef();
        batch.m_uiFirst = position;
        batch.m_uiCount = 1;
        batch.m_uiVertices = primitive.m_uiVertexCount;
        batch.m_Vertices = builder.ReadBuffer(builder.ImportBuffer(xiiStringBuilder(context.m_sPassName, ": Primitive/", keys[index]), primitive.m_pVertices, primitive.m_pVertices->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
      }
      xiiGALBufferCreationDescription buffer;
      buffer.m_uiSize = data.m_Data.GetCount() * sizeof(xiiEditorGizmoInstance);
      buffer.m_uiElementByteStride = sizeof(xiiEditorGizmoInstance);
      buffer.m_Mode = xiiGALBufferMode::Structured;
      buffer.m_BindFlags = xiiGALBindFlags::ShaderResource;
      buffer.m_Usage = xiiGALResourceUsage::Dynamic;
      buffer.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_Instances = builder.WriteBuffer(xiiStringBuilder(context.m_sPassName, layer ? ": Instances/OnTop" : ": Instances/Depth"), buffer, xiiGALResourceStateFlags::ShaderResource);
      buffer.m_uiSize = sizeof(xiiEditorGizmoConstants);
      buffer.m_uiElementByteStride = 0;
      buffer.m_Mode = xiiGALBufferMode::Undefined;
      buffer.m_BindFlags = xiiGALBindFlags::UniformBuffer;
      data.m_Constants = builder.WriteBuffer(xiiStringBuilder(context.m_sPassName, layer ? ": View/OnTop" : ": View/Depth"), buffer, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false);
      builder.SetPassRenderPassManaged();
      builder.SetPassPreparation([&data](xiiRenderGraphPassContext& execution) {
        data.m_bReady = xiiEditorViewportUtils::Upload(execution, data.m_Instances, xiiArrayPtr<const xiiEditorGizmoInstance>(data.m_Data), xiiGALResourceStateFlags::ShaderResource) && xiiEditorViewportUtils::Upload(execution, data.m_Constants, xiiArrayPtr<const xiiEditorGizmoConstants>(&data.m_Settings, 1), xiiGALResourceStateFlags::ConstantBuffer);
      }); }, [pipeline](const DrawData& data, xiiRenderGraphPassContext& execution) mutable {
      if (!data.m_bReady) return;
      pipeline.m_GraphicsPipeline.m_pRenderPass = execution.GetRenderPass();
      pipeline.m_GraphicsPipeline.m_uiSubpassIndex = execution.GetSubpassIndex();
      auto state = xiiGALPipelineCache::GetPipeline(pipeline);
      if (!state) { xiiLog::Error("Failed to create gizmo graphics pipeline."); return; }
      auto& commands = execution.GetCommandList();
      const auto size = execution.GetTexture(data.m_Color)->GetDescription().m_Size;
      commands.SetViewport({0, 0, float(size.width), float(size.height), 0, 1});
      commands.SetPipelineState(state.Borrow());
      commands.ResolveAndSetConstantBuffer("xiiEditorGizmoConstants", execution.GetBuffer(data.m_Constants), xiiGALShaderType::Vertex | xiiGALShaderType::Pixel);
      commands.ResolveAndSetShaderResourceBufferView("GizmoInstances", execution.GetBuffer(data.m_Instances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex | xiiGALShaderType::Pixel);
      commands.ResolveAndSetShaderResourceTextureView("SceneDepth", execution.GetTexture(data.m_SceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      for (const auto& batch : data.m_Batches)
      {
        commands.ResolveAndSetShaderResourceBufferView("GizmoVertices", execution.GetBuffer(batch.m_Vertices)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
        if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Succeeded()) commands.Draw({batch.m_uiVertices, batch.m_uiCount, 0, batch.m_uiFirst});
      } });
  }
  auto resolvedDescription            = image;
  resolvedDescription.m_uiSampleCount = 1;
  resolvedDescription.m_BindFlags     = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
  xiiRenderGraphTextureHandle resolved;
  graph.AddPass<ImageData>(xiiStringBuilder(context.m_sPassName, ": Resolve antialiasing"), xiiGALCommandQueueFlags::Graphics, [&](ImageData& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth = builder.ReadTexture(overlay, xiiGALResourceStateFlags::ResolveSource);
    resolved = data.m_Color = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, ": Resolved overlay"), resolvedDescription, xiiGALResourceStateFlags::ResolveDestination); }, [](const ImageData& data, xiiRenderGraphPassContext& execution) {
    xiiGALResolveTextureSubresourceDescription resolve;
    resolve.m_SourceTextureTransitionMode = xiiGALStateTransitionMode::Verify;
    resolve.m_DestinationTextureTransitionMode = xiiGALStateTransitionMode::Verify;
    execution.GetCommandList().ResolveTextureSubResource(execution.GetTexture(data.m_Depth), execution.GetTexture(data.m_Color), resolve); });
  graph.AddPass<ImageData>(xiiStringBuilder(context.m_sPassName, ": Composite"), xiiGALCommandQueueFlags::Graphics, [&](ImageData& data, xiiRenderGraphBuilder& builder) {
    color = data.m_Color = builder.ReadWriteTexture(color, xiiGALResourceStateFlags::RenderTarget);
    data.m_Depth = builder.ReadTexture(resolved, xiiGALResourceStateFlags::ShaderResource);
    builder.SetPassRenderPassManaged(); }, [composite](const ImageData& data, xiiRenderGraphPassContext& execution) mutable {
    composite.m_GraphicsPipeline.m_pRenderPass = execution.GetRenderPass();
    composite.m_GraphicsPipeline.m_uiSubpassIndex = execution.GetSubpassIndex();
    auto state = xiiGALPipelineCache::GetPipeline(composite);
    if (!state) { xiiLog::Error("Failed to create gizmo composite pipeline."); return; }
    auto& commands = execution.GetCommandList();
    const auto size = execution.GetTexture(data.m_Color)->GetDescription().m_Size;
    commands.SetViewport({0, 0, float(size.width), float(size.height), 0, 1});
    commands.SetPipelineState(state.Borrow());
    commands.ResolveAndSetShaderResourceTextureView("GizmoOverlay", execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Succeeded()) commands.Draw({3}); });
  return context.SetOutput(m_OutputColor, color);
}
