#pragma once

#include <GraphicsCore/Pipeline/Extractor.h>

class XII_GRAPHICSCORE_DLL xiiSelectedObjectsExtractorBase : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSelectedObjectsExtractorBase, xiiExtractor);

public:
  xiiSelectedObjectsExtractorBase(xiiStringView sName = "SelectedObjectsExtractor");
  ~xiiSelectedObjectsExtractorBase();

  virtual void Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override {}

  virtual const xiiDeque<xiiGameObjectHandle>* GetSelection() = 0;

  xiiRenderData::Category m_OverrideCategory;

private:
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiCVarBool*   m_pSpatialVisBoundsCVar           = nullptr;
  xiiCVarBool*   m_pSpatialVisLocalBBoxCVar        = nullptr;
  xiiCVarBool*   m_pSpatialVisDataCVar             = nullptr;
  xiiCVarBool*   m_pSpatialVisDataOnlySelectedCVar = nullptr;
  xiiCVarString* m_pSpatialVisDataOnlyCategoryCVar = nullptr;
#endif
};

//////////////////////////////////////////////////////////////////////////

/// \brief Stores a list of game objects that should get highlighted by the renderer.
///
/// Store an instance somewhere in your game code:
/// xiiSelectedObjectsContext m_SelectedObjects;
/// Add handles to game object that should be get the highlighting outline (as the editor uses for selected objects).
/// On a xiiView call:
/// xiiView::SetExtractorProperty("HighlightObjects", "SelectionContext", &m_SelectedObjects);
/// The first name must be the name of a xiiSelectedObjectsExtractor that is instantiated by the render pipeline.
///
/// As long as there is also a xiiSelectionHighlightPass in the render pipeline, all objects in this selection will be rendered
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

//////////////////////////////////////////////////////////////////////////

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

  /// \brief The context is typically set through a xiiView, through xiiView::SetExtractorProperty("<name>", "SelectionContext", pointer);
  void                       SetSelectionContext(xiiSelectedObjectsContext* pSelectionContext) { m_pSelectionContext = pSelectionContext; } // [ property ]
  xiiSelectedObjectsContext* GetSelectionContext() const { return m_pSelectionContext; }                                                    // [ property ]

private:
  xiiSelectedObjectsContext* m_pSelectionContext = nullptr;
};
