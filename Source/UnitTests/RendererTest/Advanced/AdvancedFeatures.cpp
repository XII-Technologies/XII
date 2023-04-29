#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <RendererTest/Advanced/AdvancedFeatures.h>

namespace
{
  xiiMat4 CreateSimpleMVP(float fAspectRatio)
  {
    xiiCamera cam;
    cam.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
    cam.LookAt(xiiVec3(0, 0, 0), xiiVec3(0, 0, -1), xiiVec3(0, 1, 0));
    xiiMat4 mProj;
    cam.GetProjectionMatrix(fAspectRatio, mProj);
    xiiMat4 mView = cam.GetViewMatrix();

    xiiMat4 mTransform;
    mTransform.SetTranslationMatrix(xiiVec3(0.0f, 0.0f, -1.2f));
    return mProj * mView * mTransform;
  }
} // namespace



xiiResult xiiRendererTestAdvancedFeatures::InitializeSubTest(xiiInt32 iIdentifier)
{
  m_iFrame        = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();

  XII_SUCCEED_OR_RETURN(xiiGraphicsTest::InitializeSubTest(iIdentifier));
  XII_SUCCEED_OR_RETURN(SetupRenderer());
  XII_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  if (iIdentifier == ST_ReadRenderTarget)
  {
    // Texture2D
    xiiGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, xiiGALResourceFormat::BGRAUByteNormalizedsRGB, xiiGALMSAASampleCount::None);
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    xiiGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture         = m_hTexture2D;
    viewDesc.m_uiMipLevelsToUse = 1;
    for (xiiUInt32 i = 0; i < 4; i++)
    {
      viewDesc.m_uiMostDetailedMipLevel = 0;
      m_hTexture2DMips[i]               = m_pDevice->CreateResourceView(viewDesc);
    }

    m_hShader2 = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/UVColor.xiiShader");
    m_hShader  = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/Texture2D.xiiShader");
  }

  if (iIdentifier == ST_VertexShaderRenderTargetArrayIndex)
  {
    if (!m_pDevice->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
    {
      xiiTestFramework::GetInstance()->Output(xiiTestOutput::Warning, "VertexShaderRenderTargetArrayIndex capability not supported, skipping test.");
      return XII_SUCCESS;
    }
    // Texture2DArray
    xiiGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(320 / 2, 240, xiiGALResourceFormat::BGRAUByteNormalizedsRGB, xiiGALMSAASampleCount::None);
    desc.m_uiArraySize = 2;
    m_hTexture2DArray  = m_pDevice->CreateTexture(desc);

    m_hShader  = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/Stereo.xiiShader");
    m_hShader2 = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/StereoPreview.xiiShader");
  }

  {
    // Cube mesh
    xiiGeometry geom;
    geom.AddBox(xiiVec3(1.0f), true);

    xiiGALPrimitiveTopology::Enum   Topology = xiiGALPrimitiveTopology::Triangles;
    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);
    desc.AddStream(xiiGALVertexAttributeSemantic::TexCoord0, xiiGALResourceFormat::RGFloat);
    desc.AllocateStreamsFromGeometry(geom, Topology);

    m_hCubeUV = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>("Texture2DBox", std::move(desc), "Texture2DBox");
  }

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return XII_SUCCESS;
}

xiiResult xiiRendererTestAdvancedFeatures::DeInitializeSubTest(xiiInt32 iIdentifier)
{
  m_hShader2.Invalidate();

  if (!m_hTexture2D.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2D);
    m_hTexture2D.Invalidate();
  }

  if (!m_hTexture2DArray.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2DArray);
    m_hTexture2DArray.Invalidate();
  }

  m_hCubeUV.Invalidate();
  m_hShader.Invalidate();

  DestroyWindow();
  ShutdownRenderer();
  XII_SUCCEED_OR_RETURN(xiiGraphicsTest::DeInitializeSubTest(iIdentifier));
  return XII_SUCCESS;
}

xiiTestAppRun xiiRendererTestAdvancedFeatures::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  m_iFrame        = uiInvocationCount;
  m_bCaptureImage = false;
  m_pDevice->BeginFrame(uiInvocationCount);
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      ReadRenderTarget();
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      if (!m_pDevice->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
        return xiiTestAppRun::Quit;
      VertexShaderRenderTargetArrayIndex();
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  xiiRenderContext::GetDefaultInstance()->ResetContextState();
  m_pDevice->EndPipeline(m_hSwapChain);
  m_pDevice->EndFrame();
  m_pWindow->ProcessWindowMessages();

  xiiTaskSystem::FinishFrameTasks();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return xiiTestAppRun::Quit;
  }
  return xiiTestAppRun::Continue;
}

void xiiRendererTestAdvancedFeatures::RenderToScreen(xiiUInt32 uiRenderTargetClearMask, xiiRectFloat viewport, xiiDelegate<void(xiiGALRenderCommandEncoder*)> func)
{
  const xiiGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
  renderingSetup.m_ClearColor              = xiiColor::RebeccaPurple;
  renderingSetup.m_uiRenderTargetClearMask = uiRenderTargetClearMask;
  if (!m_hDepthStencilTexture.IsInvalidated())
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
    renderingSetup.m_bClearDepth   = true;
    renderingSetup.m_bClearStencil = true;
  }

  xiiGALRenderCommandEncoder* pCommandEncoder = xiiRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);

  SetClipSpace();

  func(pCommandEncoder);

  xiiRenderContext::GetDefaultInstance()->EndRendering();
}

void xiiRendererTestAdvancedFeatures::RenderCube(xiiRectFloat viewport, xiiMat4 mMVP, xiiUInt32 uiRenderTargetClearMask, xiiGALResourceViewHandle hSRV)
{
  RenderToScreen(uiRenderTargetClearMask, viewport, [&](xiiGALRenderCommandEncoder* pEncoder) {
    xiiRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", hSRV);
    RenderObject(m_hCubeUV, mMVP, xiiColor(1, 1, 1, 1), xiiShaderBindFlags::None);
    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      XII_TEST_IMAGE(m_iFrame, 100);
    }
  });
}

void xiiRendererTestAdvancedFeatures::ReadRenderTarget()
{
  m_pPass = m_pDevice->BeginPass("Offscreen");
  {
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2D));
    renderingSetup.m_ClearColor              = xiiColor::RebeccaPurple;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

    xiiRectFloat                viewport        = xiiRectFloat(0, 0, 8, 8);
    xiiGALRenderCommandEncoder* pCommandEncoder = xiiRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
    SetClipSpace();

    xiiRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    xiiRenderContext::GetDefaultInstance()->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
    xiiRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    xiiRenderContext::GetDefaultInstance()->EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  const float     fWidth         = (float)m_pWindow->GetClientAreaSize().width;
  const float     fHeight        = (float)m_pWindow->GetClientAreaSize().height;
  const xiiUInt32 uiColumns      = 2;
  const xiiUInt32 uiRows         = 2;
  const float     fElementWidth  = fWidth / uiColumns;
  const float     fElementHeight = fHeight / uiRows;

  const xiiMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  m_pPass            = m_pDevice->BeginPass("Texture2D");
  {
    xiiRectFloat viewport = xiiRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DMips[0]);
    viewport = xiiRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
    viewport = xiiRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
    m_bCaptureImage = true;
    viewport        = xiiRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void xiiRendererTestAdvancedFeatures::VertexShaderRenderTargetArrayIndex()
{
  m_bCaptureImage    = true;
  const xiiMat4 mMVP = CreateSimpleMVP((m_pWindow->GetClientAreaSize().width / 2.0f) / (float)m_pWindow->GetClientAreaSize().height);
  m_pPass            = m_pDevice->BeginPass("Offscreen Stereo");
  {
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.m_ClearColor              = xiiColor::RebeccaPurple;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

    xiiRectFloat                viewport        = xiiRectFloat(0, 0, m_pWindow->GetClientAreaSize().width / 2.0f, (float)m_pWindow->GetClientAreaSize().height);
    xiiGALRenderCommandEncoder* pCommandEncoder = xiiRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
    SetClipSpace();

    xiiRenderContext::GetDefaultInstance()->BindShader(m_hShader, xiiShaderBindFlags::None);
    ObjectCB* ocb = xiiRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP    = mMVP;
    ocb->m_Color  = xiiColor(1, 1, 1, 1);
    xiiRenderContext::GetDefaultInstance()->BindConstantBuffer(XII_STRINGIZE(PerObject), m_hObjectTransformCB);
    xiiRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
    xiiRenderContext::GetDefaultInstance()->DrawMeshBuffer(0xFFFFFFFF, 0, 2).IgnoreResult();

    xiiRenderContext::GetDefaultInstance()->EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  m_pPass = m_pDevice->BeginPass("Texture2DArray");
  {
    xiiRectFloat viewport = xiiRectFloat(0, 0, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    RenderToScreen(0xFFFFFFFF, viewport, [&](xiiGALRenderCommandEncoder* pEncoder) {
      xiiRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_pDevice->GetDefaultResourceView(m_hTexture2DArray));

      xiiRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
      xiiRenderContext::GetDefaultInstance()->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
      xiiRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

      if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
      {
        XII_TEST_IMAGE(m_iFrame, 100);
      }
    });
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

static xiiRendererTestAdvancedFeatures g_AdvancedFeaturesTest;
