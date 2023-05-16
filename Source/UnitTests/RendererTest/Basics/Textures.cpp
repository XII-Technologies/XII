#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererTest/Basics/Basics.h>

xiiTestAppRun xiiRendererTestBasics::SubtestTextures2D()
{
  BeginFrame();

  xiiRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(xiiTextureFilterSetting::FixedTrilinear);

  const xiiInt32 iNumFrames = 14;

  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/Textured.xiiShader");

  if (m_iFrame == 0)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_ABGR_Mips_D.dds");
  }

  if (m_iFrame == 1)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_ABGR_NoMips_D.dds");
  }

  if (m_iFrame == 2)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_ARGB_Mips_D.dds");
  }

  if (m_iFrame == 3)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_ARGB_NoMips_D.dds");
  }

  if (m_iFrame == 4)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_DXT1_Mips_D.dds");
  }

  if (m_iFrame == 5)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_DXT1_NoMips_D.dds");
  }

  if (m_iFrame == 6)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_DXT3_Mips_D.dds");
  }

  if (m_iFrame == 7)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_DXT3_NoMips_D.dds");
  }

  if (m_iFrame == 8)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_DXT5_Mips_D.dds");
  }

  if (m_iFrame == 9)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_DXT5_NoMips_D.dds");
  }

  if (m_iFrame == 10)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_RGB_Mips_D.dds");
  }

  if (m_iFrame == 11)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_RGB_NoMips_D.dds");
  }

  if (m_iFrame == 12)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_R5G6B5_NoMips_D.dds");
  }

  if (m_iFrame == 13)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/ezLogo_R5G6B5_MipsD.dds");
  }

  xiiRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_hTexture2D);

  ClearScreen(xiiColor::Black);

  RenderObjects(xiiShaderBindFlags::Default);

  XII_TEST_IMAGE(m_iFrame, 100);

  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? xiiTestAppRun::Continue : xiiTestAppRun::Quit;
}


xiiTestAppRun xiiRendererTestBasics::SubtestTextures3D()
{
  BeginFrame();

  xiiRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(xiiTextureFilterSetting::FixedTrilinear);

  const xiiInt32 iNumFrames = 1;

  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/TexturedVolume.xiiShader");

  if (m_iFrame == 0)
  {
    m_hTexture2D = xiiResourceManager::LoadResource<xiiTexture2DResource>("SharedData/Textures/Volume/ezLogo_Volume_A8_NoMips_D.dds");
  }

  xiiRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_hTexture2D);

  ClearScreen(xiiColor::Black);

  RenderObjects(xiiShaderBindFlags::Default);

  XII_TEST_IMAGE(m_iFrame, 100);

  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? xiiTestAppRun::Continue : xiiTestAppRun::Quit;
}


xiiTestAppRun xiiRendererTestBasics::SubtestTexturesCube()
{
  BeginFrame();

  xiiRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(xiiTextureFilterSetting::FixedTrilinear);

  const xiiInt32 iNumFrames = 12;

  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("RendererTest/Shaders/TexturedCube.xiiShader");

  if (m_iFrame == 0)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_XRGB_NoMips_D.dds");
  }

  if (m_iFrame == 1)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_XRGB_Mips_D.dds");
  }

  if (m_iFrame == 2)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_RGBA_NoMips_D.dds");
  }

  if (m_iFrame == 3)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_RGBA_Mips_D.dds");
  }

  if (m_iFrame == 4)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT1_NoMips_D.dds");
  }

  if (m_iFrame == 5)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds");
  }

  if (m_iFrame == 6)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT3_NoMips_D.dds");
  }

  if (m_iFrame == 7)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT3_Mips_D.dds");
  }

  if (m_iFrame == 8)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT5_NoMips_D.dds");
  }

  if (m_iFrame == 9)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT5_Mips_D.dds");
  }

  if (m_iFrame == 10)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_RGB_NoMips_D.dds");
  }

  if (m_iFrame == 11)
  {
    m_hTextureCube = xiiResourceManager::LoadResource<xiiTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_RGB_Mips_D.dds");
  }

  xiiRenderContext::GetDefaultInstance()->BindTextureCube("DiffuseTexture", m_hTextureCube);

  ClearScreen(xiiColor::Black);

  RenderObjects(xiiShaderBindFlags::Default);

  XII_TEST_IMAGE(m_iFrame, 100);

  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? xiiTestAppRun::Continue : xiiTestAppRun::Quit;
}
