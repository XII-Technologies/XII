/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <QTreeView>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtPropertyAnimModel;
class QItemSelection;
class QItemSelectionModel;
class xiiQtCurve1DEditorWidget;
class xiiQtEventTrackEditorWidget;
struct xiiDocumentObjectPropertyEvent;
struct xiiDocumentObjectStructureEvent;
class xiiPropertyAnimAssetDocument;
class xiiQtColorGradientEditorWidget;
class xiiColorGradientAssetData;
class xiiQtQuadViewWidget;
class xiiQtTimeScrubberToolbar;
struct xiiPropertyAnimAssetDocumentEvent;
class QKeyEvent;
class xiiQtDocumentPanel;

class xiiQtPropertyAnimAssetTreeView : public QTreeView
{
  Q_OBJECT

public:
  xiiQtPropertyAnimAssetTreeView(QWidget* pParent);
  void initialize();

Q_SIGNALS:
  void DeleteSelectedItemsEvent();
  void FrameSelectedItemsEvent();
  void RebindSelectedItemsEvent();

protected slots:
  void onBeforeModelReset();
  void onAfterModelReset();

protected:
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void contextMenuEvent(QContextMenuEvent* event) override;
  void         storeExpandState(const QModelIndex& parent);
  void         restoreExpandState(const QModelIndex& parent, QModelIndexList& newSelection);

  QSet<QString> m_NotExpandedState;
  QSet<QString> m_SelectedItems;
};

class xiiQtPropertyAnimAssetDocumentWindow : public xiiQtGameObjectDocumentWindow, public xiiGameObjectGizmoInterface
{
  Q_OBJECT

public:
  xiiQtPropertyAnimAssetDocumentWindow(xiiPropertyAnimAssetDocument* pDocument);
  ~xiiQtPropertyAnimAssetDocumentWindow();

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "PropertyAnimAsset"; }

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

public:
  /// \name xiiGameObjectGizmoInterface implementation
  ///@{
  virtual xiiObjectAccessorBase* GetObjectAccessor() override;
  virtual bool                   CanDuplicateSelection() const override;
  virtual void                   DuplicateSelection() override;
  ///@}

protected:
  virtual void InternalRedraw() override;
  void         PropertyAnimAssetEventHandler(const xiiPropertyAnimAssetDocumentEvent& e);

private Q_SLOTS:
  void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void onScrubberPosChanged(xiiUInt64 uiTick);
  void onDeleteSelectedItems();
  void onRebindSelectedItems();
  void onPlaybackTick();
  void onPlayPauseClicked();
  void onRepeatClicked();
  void onAdjustDurationClicked();
  void onDurationChangedEvent(double duration);
  void onTreeItemDoubleClicked(const QModelIndex& index);
  void onFrameSelectedTracks();

  //////////////////////////////////////////////////////////////////////////
  // Curve editor events

  void onCurveInsertCpAt(xiiUInt32 uiCurveIdx, xiiInt64 tickX, double newPosY);
  void onCurveCpMoved(xiiUInt32 curveIdx, xiiUInt32 cpIdx, xiiInt64 iTickX, double newPosY);
  void onCurveCpDeleted(xiiUInt32 curveIdx, xiiUInt32 cpIdx);
  void onCurveTangentMoved(xiiUInt32 curveIdx, xiiUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onLinkCurveTangents(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool bLink);
  void onCurveTangentModeChanged(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool rightTangent, int mode);

  void onCurveBeginOperation(QString name);
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges(QString name);
  void onCurveEndCpChanges();

  //////////////////////////////////////////////////////////////////////////
  // Color gradient editor events

  void onGradientColorCpAdded(double posX, const xiiColorGammaUB& color);
  void onGradientAlphaCpAdded(double posX, xiiUInt8 alpha);
  void onGradientIntensityCpAdded(double posX, float intensity);
  void MoveGradientCP(xiiInt32 idx, double newPosX, const char* szArrayName);
  void onGradientColorCpMoved(xiiInt32 idx, double newPosX);
  void onGradientAlphaCpMoved(xiiInt32 idx, double newPosX);
  void onGradientIntensityCpMoved(xiiInt32 idx, double newPosX);
  void RemoveGradientCP(xiiInt32 idx, const char* szArrayName);
  void onGradientColorCpDeleted(xiiInt32 idx);
  void onGradientAlphaCpDeleted(xiiInt32 idx);
  void onGradientIntensityCpDeleted(xiiInt32 idx);
  void onGradientColorCpChanged(xiiInt32 idx, const xiiColorGammaUB& color);
  void onGradientAlphaCpChanged(xiiInt32 idx, xiiUInt8 alpha);
  void onGradientIntensityCpChanged(xiiInt32 idx, float intensity);
  void onGradientBeginOperation();
  void onGradientEndOperation(bool commit);
  // void onGradientNormalizeRange();

  //////////////////////////////////////////////////////////////////////////
  // Event track editor events
  void onEventTrackInsertCpAt(xiiInt64 tickX, QString value);
  void onEventTrackCpMoved(xiiUInt32 cpIdx, xiiInt64 iTickX);
  void onEventTrackCpDeleted(xiiUInt32 cpIdx);
  void onEventTrackBeginOperation(QString name);
  void onEventTrackEndOperation(bool commit);
  void onEventTrackBeginCpChanges(QString name);
  void onEventTrackEndCpChanges();

  //////////////////////////////////////////////////////////////////////////

private:
  xiiPropertyAnimAssetDocument* GetPropertyAnimDocument();
  // void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);
  void CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);
  void UpdateCurveEditor();
  void UpdateGradientEditor();
  void UpdateEventTrackEditor();
  void UpdateSelectionData();

  xiiQtQuadViewWidget*            m_pQuadViewWidget;
  xiiCurveGroupData               m_CurvesToDisplay;
  xiiColorGradientAssetData*      m_pGradientToDisplay  = nullptr;
  xiiInt32                        m_iMapGradientToTrack = -1;
  xiiDynamicArray<xiiInt32>       m_MapSelectionToTrack;
  xiiQtPropertyAnimAssetTreeView* m_pPropertyTreeView = nullptr;
  xiiQtPropertyAnimModel*         m_pPropertiesModel;
  QItemSelectionModel*            m_pSelectionModel     = nullptr;
  xiiQtCurve1DEditorWidget*       m_pCurveEditor        = nullptr;
  xiiQtEventTrackEditorWidget*    m_pEventTrackEditor   = nullptr;
  xiiQtColorGradientEditorWidget* m_pGradientEditor     = nullptr;
  xiiQtTimeScrubberToolbar*       m_pScrubberToolbar    = nullptr;
  xiiQtDocumentPanel*             m_pCurvePanel         = nullptr;
  xiiQtDocumentPanel*             m_pColorGradientPanel = nullptr;
  xiiQtDocumentPanel*             m_pEventTrackPanel    = nullptr;
  bool                            m_bAnimTimerInFlight  = false;
};
