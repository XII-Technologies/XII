#include <RendererTest/RendererTestPCH.h>

#include "TestClass.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>


xiiGraphicsTest::xiiGraphicsTest() = default;

xiiResult xiiGraphicsTest::InitializeSubTest(xiiInt32 iIdentifier)
{
  // Initialize everything up to 'Core'
  xiiStartup::StartupCoreSystems();

  return XII_SUCCESS;
}

xiiResult xiiGraphicsTest::DeInitializeSubTest(xiiInt32 iIdentifier)
{
  // Shut down completely
  xiiStartup::ShutdownCoreSystems();

  xiiMemoryTracker::DumpMemoryLeaks();

  return XII_SUCCESS;
}

xiiSizeU32 xiiGraphicsTest::GetResolution() const
{
  return m_pWindow->GetClientAreaSize();
}

xiiResult xiiGraphicsTest::SetupRenderer()
{
  {
    xiiFileSystem::SetSpecialDirectory("testout", xiiTestFramework::GetInstance()->GetAbsOutputPath());

    xiiStringBuilder sBaseDir = ">sdk/Data/Base/";
    xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
    sReadDir.PathParentDirectory();

    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", xiiFileSystem::AllowWrites)); // for shader files

    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sBaseDir, "Base"));

    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">xiitest/", "ImageComparisonDataDir", "imgout", xiiFileSystem::AllowWrites));

    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sReadDir, "UnitTestData"));

    sReadDir.Set(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir"));
  }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  constexpr const char* szDefaultRenderer = "DX11";
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
  constexpr const char* szDefaultRenderer = "Vulkan";
#else
#  error Renderer not implemented on platform
#endif

  constexpr const char* szDefaultLibraryName = "xiiRendererDiligent";
  xiiGALDeviceFactory::RegisterLibraryName("DX11", "xiiRendererDX11");
  xiiGALDeviceFactory::RegisterLibraryName("D3D11", szDefaultLibraryName);
  xiiGALDeviceFactory::RegisterLibraryName("D3D12", szDefaultLibraryName);
  xiiGALDeviceFactory::RegisterLibraryName("Vulkan", szDefaultLibraryName);

  const char* szRendererName   = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
  const char* szShaderModel    = "";
  const char* szShaderCompiler = "";
  xiiGALDeviceFactory::GetShaderModelAndCompiler(szRendererName, szShaderModel, szShaderCompiler);

  xiiShaderManager::Configure(szShaderModel, true);
  XII_VERIFY(xiiPlugin::LoadPlugin(szShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", szShaderCompiler);

  // Create a device
  {
    xiiGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bDebugDevice = false;
    m_pDevice                 = xiiGALDeviceFactory::CreateDevice(szRendererName, xiiFoundation::GetDefaultAllocator(), DeviceInit);
    if (m_pDevice->Init().Failed())
      return XII_FAILURE;

    xiiGALDevice::SetDefaultDevice(m_pDevice);
  }

  if (xiiStringUtils::IsEqual_NoCase(szRendererName, "DX11") || xiiStringUtils::IsEqual_NoCase(szRendererName, "D3D11") || xiiStringUtils::IsEqual_NoCase(szRendererName, "D3D12"))
  {
    if (m_pDevice->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || m_pDevice->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics"))
    {
      // Use different images for comparison when running the D3D Reference Device
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
    }
    else if (m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon"))
    {
      // Line rendering is different on AMD and requires separate images for tests rendering lines.
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_AMD");
    }
    else
    {
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
    }
  }
  else if (xiiStringUtils::IsEqual_NoCase(szRendererName, "Vulkan"))
  {
    if (m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("llvmpipe"))
    {
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_LLVMPIPE");
    }
    else
    {
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_Vulkan");
    }
  }

  m_hObjectTransformCB = xiiRenderContext::CreateConstantBufferStorage<ObjectCB>(XII_STRINGIZE(ObjectCB));
  m_hShader            = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/Default.xiiShader");

  xiiStartup::StartupHighLevelSystems();

  return XII_SUCCESS;
}

xiiResult xiiGraphicsTest::CreateWindow(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY)
{
  // Create a window for rendering
  {
    xiiWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width  = uiResolutionX;
    WindowCreationDesc.m_Resolution.height = uiResolutionY;
    WindowCreationDesc.m_bShowMouseCursor  = true;
    m_pWindow                              = XII_DEFAULT_NEW(xiiWindow);
    if (m_pWindow->Initialize(WindowCreationDesc).Failed())
      return XII_FAILURE;
  }

  // Create a Swapchain
  {
    xiiGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow           = m_pWindow;
    swapChainDesc.m_SampleCount       = xiiGALMSAASampleCount::None;
    swapChainDesc.m_bAllowScreenshots = true;
    m_hSwapChain                      = xiiGALWindowSwapChain::Create(swapChainDesc);
    if (m_hSwapChain.IsInvalidated())
    {
      return XII_FAILURE;
    }
  }

  {
    xiiGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth             = uiResolutionX;
    texDesc.m_uiHeight            = uiResolutionY;
    texDesc.m_Format              = xiiGALResourceFormat::D24S8;
    texDesc.m_bCreateRenderTarget = true;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
    if (m_hDepthStencilTexture.IsInvalidated())
    {
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

void xiiGraphicsTest::ShutdownRenderer()
{
  XII_ASSERT_DEV(m_pWindow == nullptr, "DestroyWindow needs to be called before ShutdownRenderer");
  m_hShader.Invalidate();

  xiiRenderContext::DeleteConstantBufferStorage(m_hObjectTransformCB);
  m_hObjectTransformCB.Invalidate();

  xiiStartup::ShutdownHighLevelSystems();

  xiiResourceManager::FreeAllUnusedResources();

  if (m_pDevice)
  {
    m_pDevice->Shutdown().IgnoreResult();
    XII_DEFAULT_DELETE(m_pDevice);
  }

  xiiGALDeviceFactory::UnregisterLibraryName("DX11");
  xiiGALDeviceFactory::UnregisterLibraryName("D3D11");
  xiiGALDeviceFactory::UnregisterLibraryName("D3D12");
  xiiGALDeviceFactory::UnregisterLibraryName("Vulkan");

  xiiFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");
}

void xiiGraphicsTest::DestroyWindow()
{
  if (m_pDevice)
  {
    if (!m_hSwapChain.IsInvalidated())
    {
      m_pDevice->DestroySwapChain(m_hSwapChain);
      m_hSwapChain.Invalidate();
    }

    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      m_pDevice->DestroyTexture(m_hDepthStencilTexture);
      m_hDepthStencilTexture.Invalidate();
    }
    m_pDevice->WaitIdle();
  }


  if (m_pWindow)
  {
    m_pWindow->Destroy().IgnoreResult();
    XII_DEFAULT_DELETE(m_pWindow);
  }
}

void xiiGraphicsTest::BeginFrame()
{
  m_pDevice->BeginFrame();
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);
}

void xiiGraphicsTest::EndFrame()
{
  m_pWindow->ProcessWindowMessages();

  xiiRenderContext::GetDefaultInstance()->EndRendering();
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  xiiRenderContext::GetDefaultInstance()->ResetContextState();
  m_pDevice->EndPipeline(m_hSwapChain);

  m_pDevice->EndFrame();

  xiiTaskSystem::FinishFrameTasks();
}

xiiResult xiiGraphicsTest::GetImage(xiiImage& img)
{
  auto pCommandEncoder = xiiRenderContext::GetDefaultInstance()->GetCommandEncoder();

  xiiGALTextureHandle  hBBTexture  = m_pDevice->GetSwapChain(m_hSwapChain)->GetBackBufferTexture();
  const xiiGALTexture* pBackbuffer = xiiGALDevice::GetDefaultDevice()->GetTexture(hBBTexture);
  pCommandEncoder->ReadbackTexture(hBBTexture);
  const xiiEnum<xiiGALResourceFormat> format = pBackbuffer->GetDescription().m_Format;

  xiiImageHeader header;
  header.SetWidth(m_pWindow->GetClientAreaSize().width);
  header.SetHeight(m_pWindow->GetClientAreaSize().height);
  header.SetImageFormat(xiiTextureUtils::GalFormatToImageFormat(format, true));
  img.ResetAndAlloc(header);

  xiiGALSystemMemoryDescription MemDesc;
  MemDesc.m_pData        = img.GetPixelPointer<xiiUInt8>();
  MemDesc.m_uiRowPitch   = 4 * m_pWindow->GetClientAreaSize().width;
  MemDesc.m_uiSlicePitch = 4 * m_pWindow->GetClientAreaSize().width * m_pWindow->GetClientAreaSize().height;

  xiiArrayPtr<xiiGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
  xiiGALTextureSubresource                   sourceSubResource;
  xiiArrayPtr<xiiGALTextureSubresource>      sourceSubResources(&sourceSubResource, 1);
  pCommandEncoder->CopyTextureReadbackResult(hBBTexture, sourceSubResources, SysMemDescs);

  return XII_SUCCESS;
}

void xiiGraphicsTest::ClearScreen(const xiiColor& color)
{
  m_pPass = m_pDevice->BeginPass("RendererTest");

  const xiiGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture())).SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
  renderingSetup.m_ClearColor              = color;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth             = true;
  renderingSetup.m_bClearStencil           = true;

  xiiRectFloat viewport = xiiRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

  xiiRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
}

void xiiGraphicsTest::SetClipSpace()
{
  static xiiHashedString  sClipSpaceFlipped = xiiMakeHashedString("CLIP_SPACE_FLIPPED");
  static xiiHashedString  sTrue             = xiiMakeHashedString("TRUE");
  static xiiHashedString  sFalse            = xiiMakeHashedString("FALSE");
  xiiClipSpaceYMode::Enum clipSpace         = xiiClipSpaceYMode::RenderToTextureDefault;
  xiiRenderContext::GetDefaultInstance()->SetShaderPermutationVariable(sClipSpaceFlipped, clipSpace == xiiClipSpaceYMode::Flipped ? sTrue : sFalse);
}

xiiMeshBufferResourceHandle xiiGraphicsTest::CreateMesh(const xiiGeometry& geom, const char* szResourceName)
{
  xiiMeshBufferResourceHandle hMesh;
  hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiGALPrimitiveTopology::Enum Topology = xiiGALPrimitiveTopology::Triangles;
  if (geom.GetLines().GetCount() > 0)
    Topology = xiiGALPrimitiveTopology::Lines;

  xiiMeshBufferResourceDescriptor desc;
  desc.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);
  desc.AddStream(xiiGALVertexAttributeSemantic::Color0, xiiGALResourceFormat::RGBAUByteNormalized);
  desc.AllocateStreamsFromGeometry(geom, Topology);

  hMesh = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szResourceName, std::move(desc), szResourceName);

  return hMesh;
}

xiiMeshBufferResourceHandle xiiGraphicsTest::CreateSphere(xiiInt32 iSubDivs, float fRadius)
{
  xiiGeometry geom;
  geom.AddGeodesicSphere(fRadius, static_cast<xiiUInt8>(iSubDivs));

  xiiStringBuilder sName;
  sName.Format("Sphere_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

xiiMeshBufferResourceHandle xiiGraphicsTest::CreateTorus(xiiInt32 iSubDivs, float fInnerRadius, float fOuterRadius)
{
  xiiGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, static_cast<xiiUInt16>(iSubDivs), static_cast<xiiUInt16>(iSubDivs), true);

  xiiStringBuilder sName;
  sName.Format("Torus_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

xiiMeshBufferResourceHandle xiiGraphicsTest::CreateBox(float fWidth, float fHeight, float fDepth)
{
  xiiGeometry geom;
  geom.AddBox(xiiVec3(fWidth, fHeight, fDepth), false);

  xiiStringBuilder sName;
  sName.Format("Box_{0}_{1}_{2}", xiiArgF(fWidth, 1), xiiArgF(fHeight, 1), xiiArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

xiiMeshBufferResourceHandle xiiGraphicsTest::CreateLineBox(float fWidth, float fHeight, float fDepth)
{
  xiiGeometry geom;
  geom.AddLineBox(xiiVec3(fWidth, fHeight, fDepth));

  xiiStringBuilder sName;
  sName.Format("LineBox_{0}_{1}_{2}", xiiArgF(fWidth, 1), xiiArgF(fHeight, 1), xiiArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

void xiiGraphicsTest::RenderObject(xiiMeshBufferResourceHandle hObject, const xiiMat4& mTransform, const xiiColor& color, xiiBitflags<xiiShaderBindFlags> ShaderBindFlags)
{
  xiiRenderContext::GetDefaultInstance()->BindShader(m_hShader, ShaderBindFlags);

  // xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  // xiiGALContext* pContext = pDevice->GetPrimaryContext();

  ObjectCB* ocb = xiiRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
  ocb->m_MVP    = mTransform;
  ocb->m_Color  = color;

  xiiRenderContext::GetDefaultInstance()->BindConstantBuffer("PerObject", m_hObjectTransformCB);

  xiiRenderContext::GetDefaultInstance()->BindMeshBuffer(hObject);
  xiiRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();
}
