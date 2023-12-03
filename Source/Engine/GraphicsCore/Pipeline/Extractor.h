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

  virtual void Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData);

  virtual void PostSortAndBatch(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData);

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


class XII_GRAPHICSCORE_DLL xiiVisibleObjectsExtractor : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisibleObjectsExtractor, xiiExtractor);

public:
  xiiVisibleObjectsExtractor(xiiStringView sName = "VisibleObjectsExtractor");
  ~xiiVisibleObjectsExtractor();

  virtual void      Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;
};

class XII_GRAPHICSCORE_DLL xiiSelectedObjectsExtractorBase : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSelectedObjectsExtractorBase, xiiExtractor);

public:
  xiiSelectedObjectsExtractorBase(xiiStringView sName = "SelectedObjectsExtractor");
  ~xiiSelectedObjectsExtractorBase();

  virtual void                                 Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override;
  virtual const xiiDeque<xiiGameObjectHandle>* GetSelection() = 0;

  xiiRenderData::Category m_OverrideCategory;
};

/// \brief Stores a list of game objects that should get highlighted by the renderer.
///
/// Store an instance somewhere in your game code:
/// xiiSelectedObjectsContext m_SelectedObjects;
/// Add handles to game object that should be get the highlighting outline (as the editor uses for selected objects).
/// On an xiiView call:
/// xiiView::SetExtractorProperty("HighlightObjects", "SelectionContext", &m_SelectedObjects);
/// The first name must be the name of an xiiSelectedObjectsExtractor that is instantiated by the render pipeline.
///
/// As long as there is also an xiiSelectionHighlightPass in the render pipeline, all objects in this selection will be rendered
/// with an outline.
class XII_GRAPHICSCORE_DLL xiiSelectedObjectsContext : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSelectedObjectsContext, xiiReflectedClass);

public:
  xiiSelectedObjectsContext();
  ~xiiSelectedObjectsContext();

  void RemoveDeadObjects(const xiiWorld& world);
  void AddObjectAndChildren(const xiiWorld& world, const xiiGameObjectHandle& hObject);
  void AddObjectAndChildren(const xiiWorld& world, const xiiGameObject* pObject);

  xiiDeque<xiiGameObjectHandle> m_Objects;
};

/// \brief An extractor that can be instantiated in a render pipeline, to define manually which objects should be rendered with a selection outline.
///
/// \sa xiiSelectedObjectsContext
class XII_GRAPHICSCORE_DLL xiiSelectedObjectsExtractor : public xiiSelectedObjectsExtractorBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSelectedObjectsExtractor, xiiSelectedObjectsExtractorBase);

public:
  xiiSelectedObjectsExtractor(xiiStringView sName = "ExplicitlySelectedObjectsExtractor");
  ~xiiSelectedObjectsExtractor();

  virtual const xiiDeque<xiiGameObjectHandle>* GetSelection() override;
  virtual xiiResult                            Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult                            Deserialize(xiiStreamReader& inout_stream) override;

  /// \brief The context is typically set through an xiiView, through xiiView::SetExtractorProperty("<name>", "SelectionContext", pointer);
  void                       SetSelectionContext(xiiSelectedObjectsContext* pSelectionContext) { m_pSelectionContext = pSelectionContext; } // [ property ]
  xiiSelectedObjectsContext* GetSelectionContext() const { return m_pSelectionContext; }                                                    // [ property ]

private:
  xiiSelectedObjectsContext* m_pSelectionContext = nullptr;
};
