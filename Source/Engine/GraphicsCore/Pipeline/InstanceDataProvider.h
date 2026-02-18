#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/FrameDataProvider.h>

struct xiiPerInstanceData;
class xiiGALCommandList;
class xiiInstanceDataProvider;
class xiiInstancedMeshComponent;

struct XII_GRAPHICSCORE_DLL xiiInstanceData
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiInstanceData);

public:
  xiiInstanceData(xiiUInt32 uiMaxInstanceCount = 1024);
  ~xiiInstanceData();

  xiiSharedPtr<xiiGALBuffer> m_pInstanceDataBuffer;

  xiiSharedPtr<xiiGALBuffer> m_pObjectConstantsBuffer;

  void BindResources();

  xiiArrayPtr<xiiPerInstanceData> GetInstanceData(xiiUInt32 uiCount, xiiUInt32& out_uiOffset);
  void                            UpdateInstanceData(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiUInt32 uiCount);

private:
  friend xiiInstanceDataProvider;
  friend xiiInstancedMeshComponent;

  void CreateBuffer(xiiUInt32 uiSize);
  void Reset();

  xiiUInt32                                                       m_uiBufferSize   = 0;
  xiiUInt32                                                       m_uiBufferOffset = 0;
  xiiDynamicArray<xiiPerInstanceData, xiiAlignedAllocatorWrapper> m_PerInstanceData;
};

class XII_GRAPHICSCORE_DLL xiiInstanceDataProvider : public xiiFrameDataProvider<xiiInstanceData>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInstanceDataProvider, xiiFrameDataProviderBase);

public:
  xiiInstanceDataProvider();
  ~xiiInstanceDataProvider();

private:
  virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData) override;

  xiiInstanceData m_Data;
};
