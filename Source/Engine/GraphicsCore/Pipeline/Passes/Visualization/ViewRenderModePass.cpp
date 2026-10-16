/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/ViewRenderModePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Output/ViewRenderModeConstants.h>

using namespace xiiRenderPassUtils;
struct xiiViewRenderModePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Point;
};
xiiViewRenderModePass::xiiViewRenderModePass()  = default;
xiiViewRenderModePass::~xiiViewRenderModePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewRenderModePass, 1, xiiRTTIDefaultAllocator<xiiViewRenderModePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("UseViewMode", m_bUseViewMode)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiViewRenderMode, m_Mode)->AddAttributes(new xiiDefaultValueAttribute(xiiViewRenderMode::Default)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visualization") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiViewRenderModePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiViewRenderMode::IsSupportedVisualizationMode(m_Mode) ? XII_SUCCESS : XII_FAILURE;
}
void xiiViewRenderModePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  ResourcePins(pins);
  pins.PushBack({"DisplayLinearColor", xiiRenderPipelinePin::Texture, true, false, xiiGALResourceFormat::RGBA16Float});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UnorderedAccess;
  pins.PushBack({"DisplayLinearColor", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::RGBA16Float});
  for (xiiStringView name : {"SceneDepth", "GBufferAlbedo", "GBufferNormal", "GBufferMaterial", "GBufferEmissive", "GBufferObjectId", "WireframeSurface", "DilatedVelocityBuffer", "StableAO", "IndirectLighting", "EnvironmentSpecular", "BloomTexture"})
  {
    pins.PushBack({name, xiiRenderPipelinePin::Texture, true, true});
    pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  }
  pins.PushBack({"PerFrameCamera", xiiRenderPipelinePin::Buffer, true, true});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  for (auto& pin : pins)
    if (pin.m_Type == xiiRenderPipelinePin::Texture)
    {
      pin.m_Constraints.m_Dimension = xiiGALResourceDimension::Texture2D;
      pin.m_Constraints.m_uiSamples = 1;
    }
}
xiiResult xiiViewRenderModePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto mode  = static_cast<xiiViewRenderMode::Enum>(m_bUseViewMode && context.m_pView ? context.m_pView->GetViewRenderMode().GetValue() : m_Mode.GetValue());
  const auto input = context.GetInputTexture("DisplayLinearColor");
  if (mode == xiiViewRenderMode::Default)
    return context.SetOutput("DisplayLinearColor", input);
  if (!xiiViewRenderMode::IsSupportedVisualizationMode(mode))
    return XII_FAILURE;
  xiiRenderPassResourceScope scope(context);
  xiiStringView              source = "SceneDepth";
  switch (mode)
  {
    case xiiViewRenderMode::BaseColor:
      source = "GBufferAlbedo";
      break;
    case xiiViewRenderMode::NormalMap:
      source = "GBufferNormal";
      break;
    case xiiViewRenderMode::Metallic:
    case xiiViewRenderMode::Roughness:
    case xiiViewRenderMode::Specular:
      source = "GBufferMaterial";
      break;
    case xiiViewRenderMode::Emissive:
      source = "GBufferEmissive";
      break;
    case xiiViewRenderMode::AmbientOcclusion:
      source = "StableAO";
      break;
    case xiiViewRenderMode::MotionVectors:
      source = "DilatedVelocityBuffer";
      break;
    case xiiViewRenderMode::IndirectLighting:
      source = "IndirectLighting";
      break;
    case xiiViewRenderMode::ReflectionContribution:
      source = "EnvironmentSpecular";
      break;
    case xiiViewRenderMode::Bloom:
      source = "BloomTexture";
      break;
    default:
      break;
  }
  auto       selected = context.GetInputTexture(source);
  auto       depth    = context.GetInputTexture("SceneDepth");
  auto       objects  = context.GetInputTexture(mode == xiiViewRenderMode::Wireframe ? "WireframeSurface" : "GBufferObjectId");
  const auto camera   = context.GetInputBuffer("PerFrameCamera");
  if (!selected.IsValid() || !depth.IsValid() || !camera.IsValid() || ((mode == xiiViewRenderMode::Wireframe || mode == xiiViewRenderMode::InstanceID) && !objects.IsValid()))
  {
    xiiLog::Error("Visualization '{}' requires its selected channel, depth, camera, and identity/surface inputs.", context.m_sPassName);
    return XII_FAILURE;
  }
  if (!m_State)
    m_State = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_State->m_Pipeline, "Shaders/Pipeline/ViewRenderMode.xiiShader"));
  XII_SUCCEED_OR_RETURN(PointClampSampler(m_State->m_Point));
  if (!objects.IsValid())
  {
    struct Empty
    {
      xiiRenderGraphTextureHandle m_Texture;
    };
    objects = context.m_Graph.AddPass<Empty>("Empty visualization identities", xiiGALCommandQueueFlags::Graphics, [](Empty& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription description;
      description.m_Type = xiiGALResourceDimension::Texture2D;
      description.m_Size = {1, 1};
      description.m_Format = xiiGALResourceFormat::RGBA32UInt;
      description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      data.m_Texture = builder.WriteTexture("Empty identities", description, xiiGALResourceStateFlags::CopyDestination); }, [](const Empty& data, xiiRenderGraphPassContext& execute) { ClearRenderTargetPreservingState(execute.GetCommandList(), execute.GetTexture(data.m_Texture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero()); }).first->m_Texture;
  }
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, input, 0, 0, size));
  struct Data
  {
    xiiRenderGraphTextureHandle m_Source, m_Depth, m_Objects, m_Output;
    xiiRenderGraphBufferHandle  m_Camera, m_Constants;
    xiiViewRenderMode::Enum     m_Mode;
    xiiSizeU32                  m_Size;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Source = builder.ReadTexture(selected, xiiGALResourceStateFlags::ShaderResource);
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Objects = builder.ReadTexture(objects, xiiGALResourceStateFlags::ShaderResource);
    data.m_Output = builder.ReadWriteTexture(builder.ReadTexture(input, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Camera = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants = Constants(builder, "ViewRenderModeConstants", sizeof(xiiViewRenderModeConstants));
    data.m_Mode = mode;
    data.m_Size = size; }, [this](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiViewRenderModeConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      mapped->RenderMode = data.m_Mode;
      mapped->_Padding = xiiVec3::MakeZero();
    }
    commands.SetPipelineState(m_State->m_Pipeline);
    commands.ResolveAndSetSampler("g_sPointClamp", m_State->m_Point.Borrow());
    commands.ResolveAndSetConstantBuffer("PerFrameCamera", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiViewRenderModeConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Source", execute.GetTexture(data.m_Source)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Depth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_ObjectIds", execute.GetTexture(data.m_Objects)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Output", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Succeeded())
      commands.DispatchCompute({(data.m_Size.width + 7) / 8, (data.m_Size.height + 7) / 8, 1}); });
  return context.SetOutput("DisplayLinearColor", emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visualization_ViewRenderModePass);
