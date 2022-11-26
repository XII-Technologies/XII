#pragma once

#include <Core/Graphics/Geometry.h>
#include <Core/System/Window.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererFoundation/Device/Device.h>
#include <TestFramework/Framework/TestBaseClass.h>

#undef CreateWindow

class xiiImage;

struct ObjectCB
{
  xiiMat4  m_MVP;
  xiiColor m_Color;
};

class xiiGraphicsTest : public xiiTestBaseClass
{
public:
  xiiGraphicsTest();

  virtual xiiResult GetImage(xiiImage& img) override;

protected:
  virtual void          SetupSubTests() override {}
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override { return xiiTestAppRun::Quit; }

  virtual xiiResult InitializeTest() override { return XII_SUCCESS; }
  virtual xiiResult DeInitializeTest() override { return XII_SUCCESS; }
  virtual xiiResult InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiResult DeInitializeSubTest(xiiInt32 iIdentifier) override;

  xiiSizeU32 GetResolution() const;

protected:
  xiiResult SetupRenderer();
  xiiResult CreateWindow(xiiUInt32 uiResolutionX = 960, xiiUInt32 uiResolutionY = 540);

  void ShutdownRenderer();
  void DestroyWindow();
  void ClearScreen(const xiiColor& color = xiiColor::Black);
  void SetClipSpace();

  void BeginFrame();
  void EndFrame();

  xiiMeshBufferResourceHandle CreateMesh(const xiiGeometry& geom, const char* szResourceName);
  xiiMeshBufferResourceHandle CreateSphere(xiiInt32 iSubDivs, float fRadius);
  xiiMeshBufferResourceHandle CreateTorus(xiiInt32 iSubDivs, float fInnerRadius, float fOuterRadius);
  xiiMeshBufferResourceHandle CreateBox(float fWidth, float fHeight, float fDepth);
  xiiMeshBufferResourceHandle CreateLineBox(float fWidth, float fHeight, float fDepth);
  void                        RenderObject(xiiMeshBufferResourceHandle hObject, const xiiMat4& mTransform, const xiiColor& color, xiiBitflags<xiiShaderBindFlags> ShaderBindFlags = xiiShaderBindFlags::Default);

  xiiWindow*            m_pWindow = nullptr;
  xiiGALDevice*         m_pDevice = nullptr;
  xiiGALSwapChainHandle m_hSwapChain;
  xiiGALPass*           m_pPass = nullptr;

  xiiConstantBufferStorageHandle m_hObjectTransformCB;
  xiiShaderResourceHandle        m_hShader;
  xiiGALTextureHandle            m_hDepthStencilTexture;
};
