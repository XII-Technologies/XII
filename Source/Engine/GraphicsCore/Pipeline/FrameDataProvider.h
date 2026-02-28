#pragma once

#include <GraphicsCore/Pipeline/Declarations.h>

/// \brief Base class for frame data providers.
///
/// Frame data providers supply per-frame data to the rendering pipeline (e.g., clustered light data).
/// The data is computed once per frame and cached. Derived classes implement UpdateData() to create or update the data. The pipeline calls GetData() to retrieve it.
class XII_GRAPHICSCORE_DLL xiiFrameDataProviderBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFrameDataProviderBase, xiiReflectedClass);

protected:
  xiiFrameDataProviderBase();

  /// Derived classes implement this to create or update frame data.
  ///
  /// Called once per frame when the data is first requested. Returns a pointer to the data.
  virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData) = 0;

  /// Returns the cached frame data, updating it if necessary.
  void* GetData(const xiiRenderViewContext& renderViewContext);

private:
  friend class xiiRenderPipeline;

  const xiiRenderPipeline* m_pOwnerPipeline    = nullptr;
  void*                    m_pData             = nullptr;
  xiiUInt64                m_uiLastUpdateFrame = 0U;
};

/// \brief Typed frame data provider template.
///
/// Simplifies creating frame data providers by providing type-safe access to the data.
template <typename T>
class xiiFrameDataProvider : public xiiFrameDataProviderBase
{
public:
  XII_ALWAYS_INLINE T* GetData(const xiiRenderViewContext& renderViewContext)
  {
    return static_cast<T*>(xiiFrameDataProviderBase::GetData(renderViewContext));
  }
};
