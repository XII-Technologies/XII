#pragma once

#include <Foundation/Strings/HashedString.h>

#include <GraphicsCore/Pipeline/RenderData.h>

class xiiStreamWriter;

class XII_GRAPHICSCORE_DLL xiiExtractor : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExtractor, xiiReflectedClass);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiExtractor);

public:
  xiiExtractor(xiiStringView sName);
  virtual ~xiiExtractor();

  /// \brief Sets the name of the extractor.
  void SetName(xiiStringView sName);

  /// \brief returns the name of the extractor.
  xiiStringView GetName() const;

  virtual void Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) = 0;

  virtual void PostSortAndBatch(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) = 0;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream);

protected:
  /// \brief returns true if the given object should be filtered by view tags.
  bool FilterByViewTags(const xiiView& view, const xiiGameObject* pObject) const;

  /// \brief extracts the render data for the given object.
  void ExtractRenderData(const xiiView& view, const xiiGameObject* pObject, xiiMsgExtractRenderData& msg, xiiExtractedRenderData& extractedRenderData) const;

private:
  friend class xiiRenderPipeline;

  bool m_bActive;

  xiiHashedString m_sName;

protected:
  xiiHybridArray<xiiHashedString, 4> m_DependsOn;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  mutable xiiUInt32 m_uiNumCachedRenderData;
  mutable xiiUInt32 m_uiNumUncachedRenderData;
#endif
};
