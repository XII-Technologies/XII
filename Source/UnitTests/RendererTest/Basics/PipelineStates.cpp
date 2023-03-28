#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <RendererTest/Basics/PipelineStates.h>

#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestConstants.h>
#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestInstancing.h>

XII_DEFINE_AS_POD_TYPE(xiiTestShaderData);

namespace
{
  xiiTransform CreateTransform(const xiiUInt32 uiColumns, const xiiUInt32 uiRows, xiiUInt32 x, xiiUInt32 y)
  {
    xiiTransform t = xiiTransform::IdentityTransform();
    t.m_vScale     = xiiVec3(1.0f / float(uiColumns), 1.0f / float(uiRows), 1);
    t.m_vPosition  = xiiVec3(xiiMath::Lerp(-1.f, 1.f, (float(x) + 0.5f) / float(uiColumns)), xiiMath::Lerp(1.f, -1.f, (float(y) + 0.5f) / float(uiRows)), 0);
    if (xiiClipSpaceYMode::RenderToTextureDefault == xiiClipSpaceYMode::Flipped)
    {
      xiiTransform flipY = xiiTransform::IdentityTransform();
      flipY.m_vScale.y *= -1.0f;
      t = flipY * t;
    }
    return t;
  }

  void FillStructuredBuffer(xiiHybridArray<xiiTestShaderData, 16>& instanceData, xiiUInt32 uiColorOffset = 0, xiiUInt32 uiSlotOffset = 0)
  {
    instanceData.SetCount(16);
    const xiiUInt32 uiColumns = 4;
    const xiiUInt32 uiRows    = 2;

    for (xiiUInt32 x = 0; x < uiColumns; ++x)
    {
      for (xiiUInt32 y = 0; y < uiRows; ++y)
      {
        xiiTestShaderData& instance    = instanceData[uiSlotOffset + x * uiRows + y];
        const float        fColorIndex = float(uiColorOffset + x * uiRows + y) / 32.0f;
        instance.InstanceColor         = xiiColorScheme::LightUI(fColorIndex).GetAsVec4();
        xiiTransform t                 = CreateTransform(uiColumns, uiRows, x, y);
        instance.InstanceTransform     = t;
      }
    }
  }

  struct ImgColor
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt8 b;
    xiiUInt8 g;
    xiiUInt8 r;
    xiiUInt8 a;
  };

  void CreateImage(xiiImage& image, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiMipLevelCount, bool bMipLevelIsBlue, xiiUInt8 uiFixedBlue = 0)
  {
    xiiImageHeader header;
    header.SetImageFormat(xiiImageFormat::B8G8R8A8_UNORM_SRGB);
    header.SetWidth(uiWidth);
    header.SetHeight(uiHeight);
    header.SetNumMipLevels(uiMipLevelCount);

    image.ResetAndAlloc(header);
    for (xiiUInt32 m = 0; m < uiMipLevelCount; m++)
    {
      const xiiUInt32 uiHeight = image.GetHeight(m);
      const xiiUInt32 uiWidth  = image.GetWidth(m);

      const xiiUInt8 uiBlue = bMipLevelIsBlue ? static_cast<xiiUInt8>(255.0f * float(m) / (uiMipLevelCount - 1)) : uiFixedBlue;
      for (xiiUInt32 y = 0; y < uiHeight; y++)
      {
        const xiiUInt8 uiGreen = static_cast<xiiUInt8>(255.0f * float(y) / (uiHeight - 1));
        for (xiiUInt32 x = 0; x < uiWidth; x++)
        {
          ImgColor* pColor = image.GetPixelPointer<ImgColor>(m, 0u, 0u, x, y);
          pColor->a        = 255;
          pColor->b        = uiBlue;
          pColor->g        = uiGreen;
          pColor->r        = static_cast<xiiUInt8>(255.0f * float(x) / (uiWidth - 1));
        }
      }
    }
  }

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



xiiResult xiiRendererTestPipelineStates::InitializeSubTest(xiiInt32 iIdentifier)
{
  m_iFrame        = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();

  {
    m_bTimestampsValid = false;
    m_CPUTime[0]       = {};
    m_CPUTime[1]       = {};
    m_GPUTime[0]       = {};
    m_GPUTime[1]       = {};
    m_timestamps[0]    = {};
    m_timestamps[1]    = {};
  }

  XII_SUCCEED_OR_RETURN(xiiGraphicsTest::InitializeSubTest(iIdentifier));
  XII_SUCCEED_OR_RETURN(SetupRenderer());
  XII_SUCCEED_OR_RETURN(CreateWindow(320, 240));
  m_hMostBasicTriangleShader = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/MostBasicTriangle.xiiShader");
  m_hNDCPositionOnlyShader   = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/NDCPositionOnly.xiiShader");
  m_hConstantBufferShader    = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/ConstantBuffer.xiiShader");
  m_hInstancingShader        = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/Instancing.xiiShader");

  {
    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);
    desc.AllocateStreams(3);

    if (xiiClipSpaceYMode::RenderToTextureDefault == xiiClipSpaceYMode::Flipped)
    {
      desc.SetVertexData<xiiVec3>(0, 0, xiiVec3(1.f, 1.f, 0.0f));
      desc.SetVertexData<xiiVec3>(0, 1, xiiVec3(-1.f, 1.f, 0.0f));
      desc.SetVertexData<xiiVec3>(0, 2, xiiVec3(0.f, -1.f, 0.0f));
    }
    else
    {
      desc.SetVertexData<xiiVec3>(0, 0, xiiVec3(1.f, -1.f, 0.0f));
      desc.SetVertexData<xiiVec3>(0, 1, xiiVec3(-1.f, -1.f, 0.0f));
      desc.SetVertexData<xiiVec3>(0, 2, xiiVec3(0.f, 1.f, 0.0f));
    }

    m_hTriangleMesh = xiiResourceManager::CreateResource<xiiMeshBufferResource>("UnitTest-TriangleMesh", std::move(desc), "TriangleMesh");
  }
  {
    xiiGeometry geom;
    geom.AddSphere(0.5f, 16, 16);

    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::Triangles);

    m_hSphereMesh = xiiResourceManager::CreateResource<xiiMeshBufferResource>("UnitTest-SphereMesh", std::move(desc), "SphereMesh");
  }
  m_hTestColorsConstantBuffer    = xiiRenderContext::CreateConstantBufferStorage<xiiTestColors>(XII_STRINGIZE(xiiTestColors));
  m_hTestPositionsConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiTestPositions>(XII_STRINGIZE(xiiTestPositions));

  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiStructSize                = sizeof(xiiTestShaderData);
    desc.m_uiTotalSize                 = 16 * desc.m_uiStructSize;
    desc.m_BufferType                  = xiiGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer      = true;
    desc.m_bAllowShaderResourceView    = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    // We only fill the first 8 elements with data. The rest is dynamically updated during testing.
    xiiHybridArray<xiiTestShaderData, 16> instanceData;
    FillStructuredBuffer(instanceData);
    m_hInstancingData = m_pDevice->CreateBuffer(desc, instanceData.GetByteArrayPtr());

    xiiGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hBuffer         = m_hInstancingData;
    viewDesc.m_uiFirstElement  = 8;
    viewDesc.m_uiNumElements   = 4;
    m_hInstancingDataView_8_4  = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiFirstElement  = 12;
    m_hInstancingDataView_12_4 = m_pDevice->CreateResourceView(viewDesc);
  }

  {
    // Texture2D
    xiiGALTextureCreationDescription desc;
    desc.m_uiWidth         = 8;
    desc.m_uiHeight        = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_Format          = xiiGALResourceFormat::BGRAUByteNormalizedsRGB;

    xiiImage coloredMips;
    CreateImage(coloredMips, desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, true);

    if (iIdentifier == SubTests::ST_GenerateMipMaps)
    {
      // Clear all mips except the fist one and let them be regenerated.
      desc.m_ResourceAccess.m_bImmutable = false;
      desc.m_bAllowDynamicMipGeneration  = true;
      for (xiiUInt32 m = 1; m < desc.m_uiMipLevelCount; m++)
      {
        const xiiUInt32 uiHeight = coloredMips.GetHeight(m);
        const xiiUInt32 uiWidth  = coloredMips.GetWidth(m);
        for (xiiUInt32 y = 0; y < uiHeight; y++)
        {
          for (xiiUInt32 x = 0; x < uiWidth; x++)
          {
            ImgColor* pColor = coloredMips.GetPixelPointer<ImgColor>(m, 0u, 0u, x, y);
            pColor->a        = 255;
            pColor->b        = 0;
            pColor->g        = 0;
            pColor->r        = 0;
          }
        }
      }
    }

    xiiHybridArray<xiiGALSystemMemoryDescription, 4> initialData;
    initialData.SetCount(desc.m_uiMipLevelCount);
    for (xiiUInt32 m = 0; m < desc.m_uiMipLevelCount; m++)
    {
      xiiGALSystemMemoryDescription& memoryDesc = initialData[m];
      memoryDesc.m_pData                        = coloredMips.GetPixelPointer<xiiUInt8>(m);
      memoryDesc.m_uiRowPitch                   = static_cast<xiiUInt32>(coloredMips.GetRowPitch(m));
      memoryDesc.m_uiSlicePitch                 = static_cast<xiiUInt32>(coloredMips.GetDepthPitch(m));
    }
    m_hTexture2D = m_pDevice->CreateTexture(desc, initialData);

    xiiGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture               = m_hTexture2D;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    viewDesc.m_uiMipLevelsToUse       = 1;
    m_hTexture2D_Mip0                 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 1;
    m_hTexture2D_Mip1                 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 2;
    m_hTexture2D_Mip2                 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 3;
    m_hTexture2D_Mip3                 = m_pDevice->CreateResourceView(viewDesc);
  }

  {
    // Texture2DArray
    xiiGALTextureCreationDescription desc;
    desc.m_uiWidth         = 8;
    desc.m_uiHeight        = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_uiArraySize     = 2;
    desc.m_Type            = xiiGALTextureType::Texture2D;
    desc.m_Format          = xiiGALResourceFormat::BGRAUByteNormalizedsRGB;

    xiiImage coloredMips[2];
    CreateImage(coloredMips[0], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 0);
    CreateImage(coloredMips[1], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 255);

    xiiHybridArray<xiiGALSystemMemoryDescription, 8> initialData;
    initialData.SetCount(desc.m_uiArraySize * desc.m_uiMipLevelCount);
    for (xiiUInt32 l = 0; l < desc.m_uiArraySize; l++)
    {
      for (xiiUInt32 m = 0; m < desc.m_uiMipLevelCount; m++)
      {
        xiiGALSystemMemoryDescription& memoryDesc = initialData[m + l * desc.m_uiMipLevelCount];
        memoryDesc.m_pData                        = coloredMips[l].GetPixelPointer<xiiUInt8>(m);
        memoryDesc.m_uiRowPitch                   = static_cast<xiiUInt32>(coloredMips[l].GetRowPitch(m));
        memoryDesc.m_uiSlicePitch                 = static_cast<xiiUInt32>(coloredMips[l].GetDepthPitch(m));
      }
    }
    m_hTexture2DArray = m_pDevice->CreateTexture(desc, initialData);

    xiiGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture               = m_hTexture2DArray;
    viewDesc.m_uiMipLevelsToUse       = 1;
    viewDesc.m_uiFirstArraySlice      = 0;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    m_hTexture2DArray_Layer0_Mip0     = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 1;
    m_hTexture2DArray_Layer0_Mip1     = m_pDevice->CreateResourceView(viewDesc);

    viewDesc.m_uiFirstArraySlice      = 1;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    m_hTexture2DArray_Layer1_Mip0     = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 1;
    m_hTexture2DArray_Layer1_Mip1     = m_pDevice->CreateResourceView(viewDesc);
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
    case SubTests::ST_MostBasicShader:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ViewportScissor:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_IndexBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ConstantBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_StructuredBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_InitialData);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_Discard);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_NoOverwrite);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_CopyToTempStorage);
      break;
    case SubTests::ST_GenerateMipMaps:
    case SubTests::ST_Texture2D:
    {
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/Texture2D.xiiShader");
    }
    break;
    case SubTests::ST_Texture2DArray:
    {
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/Texture2DArray.xiiShader");
    }
    break;
    case SubTests::ST_Timestamps:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Timestamps_MaxWaitTime);
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return XII_SUCCESS;
}

xiiResult xiiRendererTestPipelineStates::DeInitializeSubTest(xiiInt32 iIdentifier)
{
  m_hTriangleMesh.Invalidate();
  m_hSphereMesh.Invalidate();

  m_hMostBasicTriangleShader.Invalidate();
  m_hNDCPositionOnlyShader.Invalidate();
  m_hConstantBufferShader.Invalidate();
  m_hInstancingShader.Invalidate();

  m_hTestColorsConstantBuffer.Invalidate();
  m_hTestPositionsConstantBuffer.Invalidate();

  if (!m_hInstancingData.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingData);
    m_hInstancingData.Invalidate();
  }
  m_hInstancingDataView_8_4.Invalidate();
  m_hInstancingDataView_12_4.Invalidate();

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
  m_hTexture2D_Mip0.Invalidate();
  m_hTexture2D_Mip1.Invalidate();
  m_hTexture2D_Mip2.Invalidate();
  m_hTexture2D_Mip3.Invalidate();
  m_hCubeUV.Invalidate();
  m_hShader.Invalidate();

  DestroyWindow();
  ShutdownRenderer();
  XII_SUCCEED_OR_RETURN(xiiGraphicsTest::DeInitializeSubTest(iIdentifier));
  return XII_SUCCESS;
}

xiiTestAppRun xiiRendererTestPipelineStates::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  m_iFrame        = uiInvocationCount;
  m_bCaptureImage = false;
  m_pDevice->BeginFrame(uiInvocationCount);
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);

  switch (iIdentifier)
  {
    case SubTests::ST_MostBasicShader:
      MostBasicTriangleTest();
      break;
    case SubTests::ST_ViewportScissor:
      ViewportScissorTest();
      break;
    case SubTests::ST_VertexBuffer:
      VertexBufferTest();
      break;
    case SubTests::ST_IndexBuffer:
      IndexBufferTest();
      break;
    case SubTests::ST_ConstantBuffer:
      ConstantBufferTest();
      break;
    case SubTests::ST_StructuredBuffer:
      StructuredBufferTest();
      break;
    case SubTests::ST_Texture2D:
      Texture2D();
      break;
    case SubTests::ST_Texture2DArray:
      Texture2DArray();
      break;
    case SubTests::ST_GenerateMipMaps:
      GenerateMipMaps();
      break;
    case SubTests::ST_Timestamps:
      Timestamps();
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  xiiRenderContext::GetDefaultInstance()->ResetContextState();
  m_pDevice->EndPipeline(m_hSwapChain);
  m_pDevice->EndFrame();

  xiiTaskSystem::FinishFrameTasks();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return xiiTestAppRun::Quit;
  }
  return xiiTestAppRun::Continue;
}

void xiiRendererTestPipelineStates::RenderBlock(xiiMeshBufferResourceHandle mesh, xiiColor clearColor, xiiUInt32 uiRenderTargetClearMask, xiiRectFloat* pViewport, xiiRectU32* pScissor)
{
  m_pPass = m_pDevice->BeginPass("MostBasicTriangle");
  {
    xiiGALRenderCommandEncoder* pCommandEncoder = BeginRendering(clearColor, uiRenderTargetClearMask, pViewport, pScissor);
    {

      if (mesh.IsValid())
      {
        xiiRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
        xiiRenderContext::GetDefaultInstance()->BindMeshBuffer(mesh);
        xiiRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();
      }
      else
      {
        xiiRenderContext::GetDefaultInstance()->BindShader(m_hMostBasicTriangleShader);
        xiiRenderContext::GetDefaultInstance()->BindNullMeshBuffer(xiiGALPrimitiveTopology::Triangles, 1);
        xiiRenderContext::GetDefaultInstance()->DrawMeshBuffer(1).AssertSuccess();
      }
    }

    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      XII_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
  }

  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

xiiGALRenderCommandEncoder* xiiRendererTestPipelineStates::BeginRendering(xiiColor clearColor, xiiUInt32 uiRenderTargetClearMask, xiiRectFloat* pViewport, xiiRectU32* pScissor)
{
  const xiiGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
  renderingSetup.m_ClearColor              = clearColor;
  renderingSetup.m_uiRenderTargetClearMask = uiRenderTargetClearMask;
  if (!m_hDepthStencilTexture.IsInvalidated())
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
    renderingSetup.m_bClearDepth   = true;
    renderingSetup.m_bClearStencil = true;
  }
  xiiRectFloat viewport = xiiRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);
  if (pViewport)
  {
    viewport = *pViewport;
  }

  xiiGALRenderCommandEncoder* pCommandEncoder = xiiRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
  xiiRectU32                  scissor         = xiiRectU32(0, 0, m_pWindow->GetClientAreaSize().width, m_pWindow->GetClientAreaSize().height);
  if (pScissor)
  {
    scissor = *pScissor;
  }
  pCommandEncoder->SetScissorRect(scissor);

  SetClipSpace();
  return pCommandEncoder;
}

void xiiRendererTestPipelineStates::EndRendering()
{
  xiiRenderContext::GetDefaultInstance()->EndRendering();
  m_pWindow->ProcessWindowMessages();
}

void xiiRendererTestPipelineStates::MostBasicTriangleTest()
{
  m_bCaptureImage = true;
  RenderBlock({}, xiiColor::RebeccaPurple);
}

void xiiRendererTestPipelineStates::ViewportScissorTest()
{
  const float     fWidth         = (float)m_pWindow->GetClientAreaSize().width;
  const float     fHeight        = (float)m_pWindow->GetClientAreaSize().height;
  const xiiUInt32 uiColumns      = 2;
  const xiiUInt32 uiRows         = 2;
  const float     fElementWidth  = fWidth / uiColumns;
  const float     fElementHeight = fHeight / uiRows;

  xiiRectFloat viewport = xiiRectFloat(0, 0, fElementWidth, fElementHeight);
  RenderBlock({}, xiiColor::CornflowerBlue, 0xFFFFFFFF, &viewport);

  viewport = xiiRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
  RenderBlock({}, xiiColor::Green, 0, &viewport);

  viewport           = xiiRectFloat(0, 0, fElementWidth, fHeight);
  xiiRectU32 scissor = xiiRectU32(0, (xiiUInt32)fElementHeight, (xiiUInt32)fElementWidth, (xiiUInt32)fElementHeight);
  RenderBlock({}, xiiColor::Green, 0, &viewport, &scissor);

  m_bCaptureImage = true;
  viewport        = xiiRectFloat(0, 0, fWidth, fHeight);
  scissor         = xiiRectU32((xiiUInt32)fElementWidth, 0, (xiiUInt32)fElementWidth, (xiiUInt32)fElementHeight);
  RenderBlock({}, xiiColor::Green, 0, &viewport, &scissor);
}

void xiiRendererTestPipelineStates::VertexBufferTest()
{
  m_bCaptureImage = true;
  RenderBlock(m_hTriangleMesh, xiiColor::RebeccaPurple);
}

void xiiRendererTestPipelineStates::IndexBufferTest()
{
  m_bCaptureImage = true;
  RenderBlock(m_hSphereMesh, xiiColor::Orange);
}

void xiiRendererTestPipelineStates::ConstantBufferTest()
{
  const xiiUInt32 uiColumns = 4;
  const xiiUInt32 uiRows    = 2;

  m_pPass = m_pDevice->BeginPass("ConstantBufferTest");
  {
    xiiGALRenderCommandEncoder* pCommandEncoder = BeginRendering(xiiColor::CornflowerBlue, 0xFFFFFFFF);
    xiiRenderContext*           pContext        = xiiRenderContext::GetDefaultInstance();
    {
      pContext->BindConstantBuffer("xiiTestColors", m_hTestColorsConstantBuffer);
      pContext->BindConstantBuffer("xiiTestPositions", m_hTestPositionsConstantBuffer);
      pContext->BindShader(m_hConstantBufferShader);
      pContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::Triangles, 1);

      for (xiiUInt32 x = 0; x < uiColumns; ++x)
      {
        for (xiiUInt32 y = 0; y < uiRows; ++y)
        {
          {
            auto constants         = xiiRenderContext::GetConstantBufferData<xiiTestColors>(m_hTestColorsConstantBuffer);
            constants->VertexColor = xiiColorScheme::LightUI(float(x * uiRows + y) / (uiColumns * uiRows)).GetAsVec4();
          }
          {
            xiiTransform t         = CreateTransform(uiColumns, uiRows, x, y);
            auto         constants = xiiRenderContext::GetConstantBufferData<xiiTestPositions>(m_hTestPositionsConstantBuffer);
            constants->Vertex0     = (t * xiiVec3(1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex1     = (t * xiiVec3(-1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex2     = (t * xiiVec3(-0.f, 1.f, 0.0f)).GetAsVec4(1.0f);
          }
          pContext->DrawMeshBuffer(1).AssertSuccess();
        }
      }
    }
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      XII_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}


void xiiRendererTestPipelineStates::StructuredBufferTest()
{
  m_pPass = m_pDevice->BeginPass("InstancingTest");
  {

    xiiGALRenderCommandEncoder* pCommandEncoder = BeginRendering(xiiColor::CornflowerBlue, 0xFFFFFFFF);
    if (m_iFrame == ImageCaptureFrames::StructuredBuffer_Discard)
    {
      // Discard previous buffer.
      xiiHybridArray<xiiTestShaderData, 16> instanceData;
      FillStructuredBuffer(instanceData, 16);
      pCommandEncoder->UpdateBuffer(m_hInstancingData, 0, instanceData.GetArrayPtr().ToByteArray(), xiiGALUpdateMode::Discard);
    }
    else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_NoOverwrite)
    {
      // Nothing has touched the second half of the new buffer yet. Fill it with the original data of the first 8 elements.
      xiiHybridArray<xiiTestShaderData, 16> instanceData;
      FillStructuredBuffer(instanceData);
      instanceData.SetCount(8);
      pCommandEncoder->UpdateBuffer(m_hInstancingData, 8 * sizeof(xiiTestShaderData), instanceData.GetArrayPtr().ToByteArray(), xiiGALUpdateMode::NoOverWrite);
    }
    else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_CopyToTempStorage)
    {
      // Now we replace the first 4 elements of the second half of the buffer.
      xiiHybridArray<xiiTestShaderData, 16> instanceData;
      FillStructuredBuffer(instanceData, 16);
      instanceData.SetCount(4);
      pCommandEncoder->UpdateBuffer(m_hInstancingData, 8 * sizeof(xiiTestShaderData), instanceData.GetArrayPtr().ToByteArray(), xiiGALUpdateMode::CopyToTempStorage);
    }

    xiiRenderContext* pContext = xiiRenderContext::GetDefaultInstance();
    {
      pContext->BindShader(m_hInstancingShader);
      pContext->BindMeshBuffer(m_hTriangleMesh);

      if (m_iFrame < ImageCaptureFrames::StructuredBuffer_NoOverwrite)
      {
        pContext->BindBuffer("instancingData", m_pDevice->GetDefaultResourceView(m_hInstancingData));
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame >= ImageCaptureFrames::StructuredBuffer_NoOverwrite)
      {
        pContext->BindBuffer("instancingData", m_hInstancingDataView_8_4);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
        pContext->BindBuffer("instancingData", m_hInstancingDataView_12_4);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
      }
    }
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      XII_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void xiiRendererTestPipelineStates::RenderCube(xiiRectFloat viewport, xiiMat4 mMVP, xiiUInt32 uiRenderTargetClearMask, xiiGALResourceViewHandle hSRV)
{
  xiiGALRenderCommandEncoder* pCommandEncoder = BeginRendering(xiiColor::RebeccaPurple, uiRenderTargetClearMask, &viewport);

  xiiRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", hSRV);
  RenderObject(m_hCubeUV, mMVP, xiiColor(1, 1, 1, 1), xiiShaderBindFlags::None);
  if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
  {
    XII_TEST_IMAGE(m_iFrame, 100);
  }
  EndRendering();
};

void xiiRendererTestPipelineStates::Texture2D()
{
  const float     fWidth         = (float)m_pWindow->GetClientAreaSize().width;
  const float     fHeight        = (float)m_pWindow->GetClientAreaSize().height;
  const xiiUInt32 uiColumns      = 2;
  const xiiUInt32 uiRows         = 2;
  const float     fElementWidth  = fWidth / uiColumns;
  const float     fElementHeight = fHeight / uiRows;

  const xiiMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  m_pPass = m_pDevice->BeginPass("Texture2D");
  {
    xiiRectFloat viewport = xiiRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D_Mip0);
    viewport = xiiRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip1);
    viewport = xiiRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip2);
    m_bCaptureImage = true;
    viewport        = xiiRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip3);
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void xiiRendererTestPipelineStates::Texture2DArray()
{
  const float     fWidth         = (float)m_pWindow->GetClientAreaSize().width;
  const float     fHeight        = (float)m_pWindow->GetClientAreaSize().height;
  const xiiUInt32 uiColumns      = 2;
  const xiiUInt32 uiRows         = 2;
  const float     fElementWidth  = fWidth / uiColumns;
  const float     fElementHeight = fHeight / uiRows;

  const xiiMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  m_pPass = m_pDevice->BeginPass("Texture2DArray");
  {
    xiiRectFloat viewport = xiiRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DArray_Layer0_Mip0);
    viewport = xiiRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray_Layer0_Mip1);
    viewport = xiiRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray_Layer1_Mip0);
    m_bCaptureImage = true;
    viewport        = xiiRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray_Layer1_Mip1);
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void xiiRendererTestPipelineStates::GenerateMipMaps()
{
  const float     fWidth         = (float)m_pWindow->GetClientAreaSize().width;
  const float     fHeight        = (float)m_pWindow->GetClientAreaSize().height;
  const xiiUInt32 uiColumns      = 2;
  const xiiUInt32 uiRows         = 2;
  const float     fElementWidth  = fWidth / uiColumns;
  const float     fElementHeight = fHeight / uiRows;

  const xiiMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  m_pPass            = m_pDevice->BeginPass("GenerateMipMaps");
  {
    xiiRectFloat                viewport        = xiiRectFloat(0, 0, fElementWidth, fElementHeight);
    xiiGALRenderCommandEncoder* pCommandEncoder = BeginRendering(xiiColor::RebeccaPurple, 0, &viewport);
    pCommandEncoder->GenerateMipMaps(m_pDevice->GetDefaultResourceView(m_hTexture2D));
    EndRendering();

    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D_Mip0);
    viewport = xiiRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip1);
    viewport = xiiRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip2);
    m_bCaptureImage = true;
    viewport        = xiiRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip3);
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void xiiRendererTestPipelineStates::Timestamps()
{
  m_pPass = m_pDevice->BeginPass("Timestamps");
  {
    xiiGALRenderCommandEncoder* pCommandEncoder = BeginRendering(xiiColor::RebeccaPurple, 0xFFFFFFFF);

    if (m_iFrame == 2)
    {
      m_CPUTime[0]    = xiiTime::Now();
      m_timestamps[0] = pCommandEncoder->InsertTimestamp();
    }
    xiiRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
    xiiRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hSphereMesh);
    xiiRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_iFrame == 2)
      m_timestamps[1] = pCommandEncoder->InsertTimestamp();
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  if (m_iFrame > 2 && !m_bTimestampsValid)
  {
    if ((m_bTimestampsValid = m_pDevice->GetTimestampResult(m_timestamps[0], m_GPUTime[0]).Succeeded() && m_pDevice->GetTimestampResult(m_timestamps[1], m_GPUTime[1]).Succeeded()))
    {
      m_CPUTime[1] = xiiTime::Now();
      XII_TEST_BOOL_MSG(m_CPUTime[0] <= m_GPUTime[0], "%.6f < %.6f", m_CPUTime[0].GetSeconds(), m_GPUTime[0].GetSeconds());
      XII_TEST_BOOL_MSG(m_GPUTime[0] <= m_GPUTime[1], "%.6f < %.6f", m_GPUTime[0].GetSeconds(), m_GPUTime[1].GetSeconds());
      XII_TEST_BOOL_MSG(m_GPUTime[1] <= m_CPUTime[1], "%.6f < %.6f", m_GPUTime[1].GetSeconds(), m_CPUTime[1].GetSeconds());
      xiiTestFramework::GetInstance()->Output(xiiTestOutput::Message, "Timestamp results received after %d frames and %.3f seconds.", m_iFrame, (xiiTime::Now() - m_CPUTime[0]).AsFloatInSeconds());
      m_ImgCompFrames.Clear();
    }
  }
  xiiThreadUtils::Sleep(xiiTime::Milliseconds(16));
  if (m_iFrame > 2 && (xiiTime::Now() - m_CPUTime[0]).AsFloatInSeconds() > 10.0f)
  {
    XII_TEST_BOOL_MSG(m_bTimestampsValid, "Timestamp results are not present after 10 seconds.");
    m_ImgCompFrames.Clear();
  }
}

static xiiRendererTestPipelineStates g_PipelineStatesTest;
