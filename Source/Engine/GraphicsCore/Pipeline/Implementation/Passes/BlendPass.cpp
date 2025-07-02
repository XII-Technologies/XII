#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/BlendPass.h>
#include <GraphicsCore/Pipeline/View.h>

#include <Core/Graphics/Geometry.h>
#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/BlendConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBlendPass, 1, xiiRTTIDefaultAllocator<xiiBlendPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("InputA", m_PinInputA),
    XII_MEMBER_PROPERTY("InputB", m_PinInputB),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_MEMBER_PROPERTY("BlendFactor", m_fBlendFactor)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBlendPass::xiiBlendPass() :
  xiiRenderPipelinePass("BlendPass", xiiRenderPipelinePassFlags::None, xiiRenderPipelinePassConcurrencyHint::Sequential)
{
  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Blend.xiiShader");
  XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load blend shader!");
}

xiiBlendPass::~xiiBlendPass() = default;

bool xiiBlendPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInputA.m_uiInputIndex] && inputs[m_PinInputB.m_uiInputIndex])
  {
    if (!inputs[m_PinInputA.m_uiInputIndex]->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || !inputs[m_PinInputB.m_uiInputIndex]->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    {
      xiiLog::Error("Blend pass inputs must allow shader resource view.");
      return false;
    }

    outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinInputA.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No input connected to blend pass!");
    return false;
  }

  return true;
}

void xiiBlendPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
#ifdef CORE_ENABLE
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    xiiConstantBufferStorage<xiiBlendConstants>* pBlendConstantBuffer;
    xiiConstantBufferStorageHandle               hBlendConstantBuffer = xiiRenderContext::CreateConstantBufferStorage(pBlendConstantBuffer);
    XII_SCOPE_EXIT(xiiRenderContext::DeleteConstantBufferStorage(hBlendConstantBuffer));

    renderViewContext.m_pRenderContext->BindConstantBuffer(XII_PP_STRINGIFY(xiiBlendConstants), hBlendConstantBuffer);

    xiiBlendConstants& cb = pBlendConstantBuffer->GetDataForWriting();
    cb.BlendFactor        = m_fBlendFactor;

    // Setup render target
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, outputs[m_PinOutput.m_uiOutputIndex]->m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    renderingSetup.m_uiRenderTargetClearMask = xiiInvalidIndex;
    renderingSetup.m_ClearColor              = xiiColor(1.0f, 0.0f, 0.0f);

    // Bind render target and viewport
    auto pCommandList = xiiRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    // Setup input view and sampler
    xiiGALTextureViewCreationDescription resourceViewDescription;
    xiiSharedPtr<xiiGALTextureView>      pResourceViewA = inputs[m_PinInputA.m_uiInputIndex]->m_pTexture->CreateView(resourceViewDescription);
    xiiSharedPtr<xiiGALTextureView>      pResourceViewB = inputs[m_PinInputB.m_uiInputIndex]->m_pTexture->CreateView(resourceViewDescription);

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(nullptr, nullptr, nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("InputA", pResourceViewA);
    renderViewContext.m_pRenderContext->BindTexture2D("InputB", pResourceViewB);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
#endif
}

xiiResult xiiBlendPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fBlendFactor;
  return XII_SUCCESS;
}

xiiResult xiiBlendPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fBlendFactor;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_BlendPass);
