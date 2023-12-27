#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/BlurPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/BlurConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBlurPass, 1, xiiRTTIDefaultAllocator<xiiBlurPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(15)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBlurPass::xiiBlurPass() :
  xiiRenderPipelinePass("BlurPass")
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Blur.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBlurCB = xiiRenderContext::CreateConstantBufferStorage<xiiBlurConstants>();
  }
}

xiiBlurPass::~xiiBlurPass()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hBlurCB);
  m_hBlurCB.Invalidate();
}

bool xiiBlurPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    {
      xiiLog::Error("Blur pass input must allow shader resoure view.");
      return false;
    }

    outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinInput.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No input connected to blur pass!");
    return false;
  }

  return true;
}

void xiiBlurPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

    // Setup render target
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
    renderingSetup.m_uiRenderTargetClearMask = xiiInvalidIndex;
    renderingSetup.m_ClearColor              = xiiColor(1.0f, 0.0f, 0.0f);

    // Bind render target and viewport
    auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    // Setup input view and sampler
    xiiGALTextureViewCreationDescription rvcd;
    rvcd.m_hTexture                       = inputs[m_PinInput.m_uiInputIndex]->m_TextureHandle;
    xiiGALTextureViewHandle hResourceView = xiiGALDevice::GetDefaultDevice()->CreateTextureView(rvcd);

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("Input", hResourceView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiBlurConstants", m_hBlurCB);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
}

xiiResult xiiBlurPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_iRadius;
  return XII_SUCCESS;
}

xiiResult xiiBlurPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_iRadius;
  return XII_SUCCESS;
}

void xiiBlurPass::SetRadius(xiiInt32 iRadius)
{
  m_iRadius = iRadius;

  xiiBlurConstants* cb = xiiRenderContext::GetConstantBufferData<xiiBlurConstants>(m_hBlurCB);
  cb->BlurRadius       = m_iRadius;
}

xiiInt32 xiiBlurPass::GetRadius() const
{
  return m_iRadius;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_BlurPass);
