#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class xiiRendererTestPipelineStates : public xiiGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "PipelineStates"; }

private:
  enum SubTests
  {
    ST_MostBasicShader,
    ST_ViewportScissor,
    ST_VertexBuffer,
    ST_IndexBuffer,
    ST_ConstantBuffer,
    ST_StructuredBuffer,
    ST_Texture2D,
    ST_Texture2DArray,
    ST_GenerateMipMaps,
    ST_Timestamps,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture                     = 5,
    StructuredBuffer_InitialData       = 5,
    StructuredBuffer_Discard           = 6,
    StructuredBuffer_NoOverwrite       = 8,
    StructuredBuffer_CopyToTempStorage = 9,
    Timestamps_MaxWaitTime             = xiiMath::MaxValue<xiiUInt32>(),
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("01 - MostBasicShader", SubTests::ST_MostBasicShader);
    AddSubTest("02 - ViewportScissor", SubTests::ST_ViewportScissor);
    AddSubTest("03 - VertexBuffer", SubTests::ST_VertexBuffer);
    AddSubTest("04 - IndexBuffer", SubTests::ST_IndexBuffer);
    AddSubTest("05 - ConstantBuffer", SubTests::ST_ConstantBuffer);
    AddSubTest("06 - StructuredBuffer", SubTests::ST_StructuredBuffer);
    AddSubTest("07 - Texture2D", SubTests::ST_Texture2D);
    AddSubTest("08 - Texture2DArray", SubTests::ST_Texture2DArray);
    AddSubTest("09 - GenerateMipMaps", SubTests::ST_GenerateMipMaps);
    //AddSubTest("10 - Timestamps", SubTests::ST_Timestamps);
  }

  virtual xiiResult     InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiResult     DeInitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  void RenderBlock(xiiMeshBufferResourceHandle mesh, xiiColor clearColor = xiiColor::CornflowerBlue, xiiUInt32 uiRenderTargetClearMask = 0xFFFFFFFF, xiiRectFloat* pViewport = nullptr, xiiRectU32* pScissor = nullptr);

  xiiGALRenderCommandEncoder* BeginRendering(xiiColor clearColor, xiiUInt32 uiRenderTargetClearMask, xiiRectFloat* pViewport = nullptr, xiiRectU32* pScissor = nullptr);
  void                        EndRendering();

  void RenderCube(xiiRectFloat viewport, xiiMat4 mMVP, xiiUInt32 uiRenderTargetClearMask, xiiGALResourceViewHandle hSRV);

  void MostBasicTriangleTest();
  void ViewportScissorTest();
  void VertexBufferTest();
  void IndexBufferTest();
  void ConstantBufferTest();
  void StructuredBufferTest();
  void Texture2D();
  void Texture2DArray();
  void GenerateMipMaps();
  void Timestamps();

private:
  xiiInt32                     m_iFrame        = 0;
  bool                         m_bCaptureImage = false;
  xiiHybridArray<xiiUInt32, 8> m_ImgCompFrames;

  xiiShaderResourceHandle m_hMostBasicTriangleShader;
  xiiShaderResourceHandle m_hNDCPositionOnlyShader;
  xiiShaderResourceHandle m_hConstantBufferShader;
  xiiShaderResourceHandle m_hInstancingShader;

  xiiMeshBufferResourceHandle m_hTriangleMesh;
  xiiMeshBufferResourceHandle m_hSphereMesh;

  xiiConstantBufferStorageHandle m_hTestColorsConstantBuffer;
  xiiConstantBufferStorageHandle m_hTestPositionsConstantBuffer;

  xiiGALBufferHandle       m_hInstancingData;
  xiiGALResourceViewHandle m_hInstancingDataView_8_4;
  xiiGALResourceViewHandle m_hInstancingDataView_12_4;

  xiiGALTextureHandle         m_hTexture2D;
  xiiGALResourceViewHandle    m_hTexture2D_Mip0;
  xiiGALResourceViewHandle    m_hTexture2D_Mip1;
  xiiGALResourceViewHandle    m_hTexture2D_Mip2;
  xiiGALResourceViewHandle    m_hTexture2D_Mip3;
  xiiGALTextureHandle         m_hTexture2DArray;
  xiiGALResourceViewHandle    m_hTexture2DArray_Layer0_Mip0;
  xiiGALResourceViewHandle    m_hTexture2DArray_Layer0_Mip1;
  xiiGALResourceViewHandle    m_hTexture2DArray_Layer1_Mip0;
  xiiGALResourceViewHandle    m_hTexture2DArray_Layer1_Mip1;
  xiiMeshBufferResourceHandle m_hCubeUV;

  bool                  m_bTimestampsValid = false;
  xiiTime               m_CPUTime[2];
  xiiTime               m_GPUTime[2];
  xiiGALTimestampHandle m_timestamps[2];
};
