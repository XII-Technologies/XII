/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Composition/CompositionPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Composition/RadianceAddPass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Composition/RadianceAddConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRadianceAddPass,1,xiiRTTIDefaultAllocator<xiiRadianceAddPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("A",m_A)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("B",m_B)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ScaleA",m_fScaleA)->AddAttributes(new xiiDefaultValueAttribute(1.0f),new xiiClampValueAttribute(0.0f,xiiVariant())),
    XII_MEMBER_PROPERTY("ScaleB",m_fScaleB)->AddAttributes(new xiiDefaultValueAttribute(1.0f),new xiiClampValueAttribute(0.0f,xiiVariant())),
    XII_MEMBER_PROPERTY("Radiance",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AllowAbsentB", m_bAllowAbsentB)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Composition") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiRadianceAddPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fScaleA) && xiiMath::IsFinite(m_fScaleB) && m_fScaleA >= 0 && m_fScaleB >= 0 ? XII_SUCCESS : XII_FAILURE;
}
void xiiRadianceAddPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  const auto first = out_pins.GetCount();
  xiiRenderPipelinePass::GetPins(out_pins);
  for (xiiUInt32 i = first; i < out_pins.GetCount(); ++i)
  {
    if (out_pins[i].m_bInput && out_pins[i].m_sName == "B")
      out_pins[i].m_bOptional = m_bAllowAbsentB;
    if (!out_pins[i].m_bInput && out_pins[i].m_sName == "Radiance" && m_bAllowAbsentB)
    {
      out_pins[i].m_Format                          = xiiGALResourceFormat::Unknown;
      out_pins[i].m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
    }
  }
}

xiiResult xiiRadianceAddPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  a            = context.GetInputTexture(this->m_A);
  const auto* aDescription = xiiCompositionPassUtils::Image(context, a);
  const auto  b            = context.GetInputTexture(this->m_B);
  const auto* bDescription = xiiCompositionPassUtils::Image(context, b);
  if (!aDescription || (!b.IsValid() && !m_bAllowAbsentB) || (b.IsValid() && (!bDescription || aDescription->m_Size != bDescription->m_Size)))
    return context.ReportFailure("RadianceAdd requires matching nonempty float-readable RGBA single-sample 2D images.");
  if (!b.IsValid() && m_fScaleA == 1.0f)
    return context.SetOutput(m_Output, a);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Composition/RadianceAdd.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_A, m_B, m_Output;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiRenderGraphBufferHandle               m_Constants;
    xiiRadianceAddConstants                  m_Snapshot;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_A=builder.ReadTexture(a,xiiGALResourceStateFlags::ShaderResource);
    data.m_B=builder.ReadTexture(b.IsValid()?b:a,xiiGALResourceStateFlags::ShaderResource);
    data.m_Size=aDescription->m_Size; data.m_Output=xiiCompositionPassUtils::Output(builder,data.m_Size,xiiGALResourceFormat::RGBA32Float); data.m_Pipeline=m_Pipeline;
    data.m_Snapshot.RadianceScales=xiiVec4(m_fScaleA,b.IsValid()?m_fScaleB:0.0f,0,0); data.m_Constants=xiiRenderPassUtils::Constants(builder,"Radiance weights",sizeof(xiiRadianceAddConstants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList(); commands.SetPipelineState(data.m_Pipeline.Borrow());
    { xiiGALMapHelper<xiiRadianceAddConstants> mapped(commands,execute.GetBuffer(data.m_Constants),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_Snapshot; }
    xiiGALStateTransitionDescription transition; transition.m_pResource=execute.GetBuffer(data.m_Constants); transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1));
    commands.ResolveAndSetConstantBuffer("xiiRadianceAddConstants",execute.GetBuffer(data.m_Constants),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_A",execute.GetTexture(data.m_A)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_B",execute.GetTexture(data.m_B)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Radiance",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    if(commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()){execute.ReportFailure("Could not bind radiance composition resources.");return;} commands.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Composition_RadianceAddPass);
