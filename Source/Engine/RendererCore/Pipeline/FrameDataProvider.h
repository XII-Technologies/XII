#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class XII_RENDERERCORE_DLL xiiFrameDataProviderBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFrameDataProviderBase, xiiReflectedClass);

protected:
  xiiFrameDataProviderBase();

  virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData) = 0;

  void* GetData(const xiiRenderViewContext& renderViewContext);

private:
  friend class xiiRenderPipeline;

  const xiiRenderPipeline* m_pOwnerPipeline    = nullptr;
  void*                    m_pData             = nullptr;
  xiiUInt64                m_uiLastUpdateFrame = 0;
};

template <typename T>
class xiiFrameDataProvider : public xiiFrameDataProviderBase
{
public:
  T* GetData(const xiiRenderViewContext& renderViewContext) { return static_cast<T*>(xiiFrameDataProviderBase::GetData(renderViewContext)); }
};
