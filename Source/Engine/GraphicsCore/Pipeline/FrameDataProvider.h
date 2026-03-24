#pragma once

#include <GraphicsCore/Pipeline/Declarations.h>

class XII_GRAPHICSCORE_DLL xiiFrameDataProviderBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFrameDataProviderBase, xiiReflectedClass);

protected:
  xiiFrameDataProviderBase();

  virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData) = 0;

  void* GetData(const xiiRenderViewContext& renderViewContext);

private:
  void*     m_pData             = nullptr;
  xiiUInt64 m_uiLastUpdateFrame = 0;
};

template <typename T>
class xiiFrameDataProvider : public xiiFrameDataProviderBase
{
public:
  XII_ALWAYS_INLINE T* GetData(const xiiRenderViewContext& renderViewContext)
  {
    return static_cast<T*>(xiiFrameDataProviderBase::GetData(renderViewContext));
  }
};
