#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class xiiRendererTestAdvancedFeatures : public xiiGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "AdvancedFeatures"; }

private:
  enum SubTests
  {
    ST_ReadRenderTarget,
    ST_VertexShaderRenderTargetArrayIndex,
    //ST_BackgroundResourceLoading,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,

  };

  virtual void SetupSubTests() override
  {
    AddSubTest("01 - ReadRenderTarget", SubTests::ST_ReadRenderTarget);
    AddSubTest("02 - VertexShaderRenderTargetArrayIndex", SubTests::ST_VertexShaderRenderTargetArrayIndex);
  }

  virtual xiiResult     InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiResult     DeInitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  void RenderToScreen(xiiUInt32 uiRenderTargetClearMask, xiiRectFloat viewport, xiiDelegate<void(xiiGALRenderCommandEncoder*)> func);
  void RenderCube(xiiRectFloat viewport, xiiMat4 mMVP, xiiUInt32 uiRenderTargetClearMask, xiiGALResourceViewHandle hSRV);

  void ReadRenderTarget();
  void VertexShaderRenderTargetArrayIndex();

private:
  xiiInt32                     m_iFrame        = 0;
  bool                         m_bCaptureImage = false;
  xiiHybridArray<xiiUInt32, 8> m_ImgCompFrames;

  xiiShaderResourceHandle m_hShader2;

  xiiGALTextureHandle      m_hTexture2D;
  xiiGALResourceViewHandle m_hTexture2DMips[4];
  xiiGALTextureHandle      m_hTexture2DArray;

  xiiMeshBufferResourceHandle m_hCubeUV;
};
