#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class xiiRendererTestBasics : public xiiGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "Basics"; }

private:
  enum SubTests
  {
    ST_ClearScreen,
    ST_RasterizerStates,
    ST_BlendStates,
    ST_Textures2D,
    ST_Textures3D,
    ST_TexturesCube,
    ST_LineRendering,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Clear Screen", SubTests::ST_ClearScreen);
    AddSubTest("Rasterizer States", SubTests::ST_RasterizerStates);
    AddSubTest("Blend States", SubTests::ST_BlendStates);
    AddSubTest("2D Textures", SubTests::ST_Textures2D);
    // AddSubTest("3D Textures", SubTests::ST_Textures3D); /// \todo 3D Texture support is currently not implemented
    AddSubTest("Cube Textures", SubTests::ST_TexturesCube);
    AddSubTest("Line Rendering", SubTests::ST_LineRendering);
  }


  virtual xiiResult InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiResult DeInitializeSubTest(xiiInt32 iIdentifier) override;

  xiiTestAppRun SubtestClearScreen();
  xiiTestAppRun SubtestRasterizerStates();
  xiiTestAppRun SubtestBlendStates();
  xiiTestAppRun SubtestTextures2D();
  xiiTestAppRun SubtestTextures3D();
  xiiTestAppRun SubtestTexturesCube();
  xiiTestAppRun SubtestLineRendering();

  void RenderObjects(xiiBitflags<xiiShaderBindFlags> ShaderBindFlags);
  void RenderLineObjects(xiiBitflags<xiiShaderBindFlags> ShaderBindFlags);

  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override
  {
    ++m_iFrame;

    if (iIdentifier == SubTests::ST_ClearScreen)
      return SubtestClearScreen();

    if (iIdentifier == SubTests::ST_RasterizerStates)
      return SubtestRasterizerStates();

    if (iIdentifier == SubTests::ST_BlendStates)
      return SubtestBlendStates();

    if (iIdentifier == SubTests::ST_Textures2D)
      return SubtestTextures2D();

    if (iIdentifier == SubTests::ST_Textures3D)
      return SubtestTextures3D();

    if (iIdentifier == SubTests::ST_TexturesCube)
      return SubtestTexturesCube();

    if (iIdentifier == SubTests::ST_LineRendering)
      return SubtestLineRendering();

    return xiiTestAppRun::Quit;
  }

  xiiInt32                     m_iFrame;
  xiiMeshBufferResourceHandle  m_hSphere;
  xiiMeshBufferResourceHandle  m_hSphere2;
  xiiMeshBufferResourceHandle  m_hTorus;
  xiiMeshBufferResourceHandle  m_hLongBox;
  xiiMeshBufferResourceHandle  m_hLineBox;
  xiiTexture2DResourceHandle   m_hTexture2D;
  xiiTextureCubeResourceHandle m_hTextureCube;
};
