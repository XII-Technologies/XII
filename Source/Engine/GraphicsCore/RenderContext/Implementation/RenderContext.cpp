#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

xiiRenderContext::xiiRenderContext(xiiSharedPtr<xiiGALCommandList> pCommandList) :
  m_pCommandList(pCommandList)
{
  XII_ASSERT_DEV(m_pCommandList != nullptr, "An invalid command list is given. A render context requires a valid command list reference.");
}

xiiRenderContext::~xiiRenderContext() = default;

void xiiRenderContext::BeginRendering(const xiiRenderingSetup& renderingSetup, const xiiRectFloat& viewport, xiiStringView sName, bool bStereoRendering)
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::None, "Already in a scope.");

  m_RenderContextScope = RenderContextScope::Graphics;
  m_bStereoRendering   = bStereoRendering;

  const xiiGALRenderPassCreationDescription& renderPassDescription = renderingSetup.GetRenderPassDescription();

  xiiUInt8 uiSampleCount = xiiGALMSAASampleCount::OneSample;

  if (!renderPassDescription.m_Attachments.IsEmpty())
  {
    uiSampleCount = renderPassDescription.m_Attachments.PeekBack().m_uiSampleCount;
  }

  if (uiSampleCount > 1)
  {
    SetShaderPermutationVariable("MSAA", "TRUE");
  }
  else
  {
    SetShaderPermutationVariable("MSAA", "FALSE");
  }

  m_GlobalConstants.ViewportSize   = xiiVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
  m_GlobalConstants.NumMsaaSamples = uiSampleCount;

  {
    m_bHasDebugGroup = !sName.IsEmpty();

    if (m_bHasDebugGroup)
    {
      m_pCommandList->BeginDebugGroup(sName);
    }
  }

  m_pCommandList->SetViewport({viewport.x, viewport.y, viewport.width, viewport.height, 0.0f, 0.1f});
}

void xiiRenderContext::EndRendering()
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::Graphics, "BeginRendering() has not been called.");

  if (m_bHasDebugGroup)
  {
    m_pCommandList->EndDebugGroup();

    m_bHasDebugGroup = false;
  }

  m_bStereoRendering   = false;
  m_RenderContextScope = RenderContextScope::None;
}

void xiiRenderContext::BeginCompute(xiiStringView sName)
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::None, "Already in a scope.");

  m_RenderContextScope = RenderContextScope::Compute;

  {
    m_bHasDebugGroup = !sName.IsEmpty();

    if (m_bHasDebugGroup)
    {
      m_pCommandList->BeginDebugGroup(sName);
    }
  }
}

void xiiRenderContext::EndCompute()
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::Graphics, "BeginCompute() has not been called.");

  if (m_bHasDebugGroup)
  {
    m_pCommandList->EndDebugGroup();

    m_bHasDebugGroup = false;
  }

  m_RenderContextScope = RenderContextScope::None;
}

void xiiRenderContext::SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sValue)
{
  xiiTempHashedString sHashedName(sName);

  xiiHashedString sNameHash, sValueHash;
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sHashedName, sValue, sNameHash, sValueHash))
  {
    SetShaderPermutationVariableInternal(sNameHash, sValueHash);
  }
}

void xiiRenderContext::SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue)
{
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void xiiRenderContext::BindConstantBuffer(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pConstantBuffer)
{
}

void xiiRenderContext::BindBufferView(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
}

void xiiRenderContext::BindTextureView(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
}

void xiiRenderContext::BindSampler(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALSampler> pSamplerSate)
{
}

void xiiRenderContext::BindBufferViewUAV(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
}

void xiiRenderContext::BindTextureViewUAV(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
}

void xiiRenderContext::BindTexture2D(const xiiTempHashedString& sSlotName, const xiiTexture2DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode)
{
}

void xiiRenderContext::BindTexture3D(const xiiTempHashedString& sSlotName, const xiiTexture3DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode)
{
}

void xiiRenderContext::BindTextureCube(const xiiTempHashedString& sSlotName, const xiiTextureCubeResourceHandle& hTexture, xiiResourceAcquireMode acquireMode)
{
}

void xiiRenderContext::BindMaterial(const xiiMaterialResourceHandle& hMaterial)
{
}

void xiiRenderContext::BindShader(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags)
{
}

void xiiRenderContext::BindMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hDynamicMeshBuffer)
{
}

void xiiRenderContext::BindMeshBuffer(const xiiMeshBufferResourceHandle& hMeshBuffer)
{
}

void xiiRenderContext::BindMeshBuffer(xiiSharedPtr<xiiGALBuffer> pVertexBuffer, xiiSharedPtr<xiiGALBuffer> pIndexBuffer, const xiiInputLayoutInfo* pInputLayoutInfo, xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount, xiiSharedPtr<xiiGALBuffer> pVertexBuffer2, xiiSharedPtr<xiiGALBuffer> pVertexBuffer3, xiiSharedPtr<xiiGALBuffer> pVertexBuffer4)
{
}

xiiResult xiiRenderContext::DrawMeshBuffer(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiInstanceCount)
{
  return XII_SUCCESS;
}

xiiResult xiiRenderContext::Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  return XII_SUCCESS;
}

xiiResult xiiRenderContext::ApplyContextStates(bool bForce)
{
  return XII_SUCCESS;
}

void xiiRenderContext::ResetContextState()
{
}

void xiiRenderContext::SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue)
{
  xiiHashedString* pOldValue = nullptr;
  m_PermutationVariables.TryGetValue(sName, pOldValue);

  if (pOldValue == nullptr || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);

    m_StateFlags.Add(xiiRenderContextFlags::ShaderStateChanged);
  }
}

// static
xiiGALSamplerCreationDescription xiiRenderContext::GetDefaultSamplerDescription(xiiBitflags<xiiDefaultSamplerFlags> flags)
{
  xiiGALSamplerCreationDescription samplerDescription;
  samplerDescription.m_ComparisonFunction = xiiGALComparisonFunction::Never;
  samplerDescription.m_BorderColor        = xiiColor::Black;
  samplerDescription.m_fMipLODBias        = 0.0f;
  samplerDescription.m_fMinLOD            = -1.0f;
  samplerDescription.m_fMaxLOD            = 42000.0f;
  samplerDescription.m_uiMaxAnisotropy    = 4U;

  samplerDescription.m_MinFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALFilterType::Linear : xiiGALFilterType::Point;
  samplerDescription.m_MagFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALFilterType::Linear : xiiGALFilterType::Point;
  samplerDescription.m_MipFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALFilterType::Linear : xiiGALFilterType::Point;

  samplerDescription.m_AddressU = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp) : xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Repeat);
  samplerDescription.m_AddressV = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp) : xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Repeat);
  samplerDescription.m_AddressW = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp) : xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Repeat);

  return samplerDescription;
}
