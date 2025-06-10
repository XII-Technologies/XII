#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/RenderContext/RenderContext.h>
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
  if (m_bHasDebugGroup)
  {
    m_pCommandList->EndDebugGroup();

    m_bHasDebugGroup = false;
  }

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
