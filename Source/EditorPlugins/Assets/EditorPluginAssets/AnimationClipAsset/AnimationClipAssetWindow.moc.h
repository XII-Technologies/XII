#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Time/Clock.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtOrbitCamViewWidget;
class xiiQtTimeScrubberWidget;
class xiiQtEventTrackEditorWidget;
class xiiQtDocumentPanel;
struct xiiCommandHistoryEvent;

class xiiQtAnimationClipAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtAnimationClipAssetDocumentWindow(xiiAnimationClipAssetDocument* pDocument);
  ~xiiQtAnimationClipAssetDocumentWindow();

  xiiAnimationClipAssetDocument* GetAnimationClipDocument();
  virtual xiiStringView          GetWindowLayoutGroupName() const override { return "AnimationClipAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg) override;

  virtual void CommonAssetUiEventHandler(const xiiCommonAssetUiState& e) override;
  void         CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);

protected Q_SLOTS:
  void OnScrubberPosChangedEvent(xiiUInt64 uiNewScrubberTickPos);

  //////////////////////////////////////////////////////////////////////////
  // Event track editor events
  void onEventTrackInsertCpAt(xiiInt64 tickX, QString value);
  void onEventTrackCpMoved(xiiUInt32 cpIdx, xiiInt64 iTickX);
  void onEventTrackCpDeleted(xiiUInt32 cpIdx);
  void onEventTrackBeginOperation(QString name);
  void onEventTrackEndOperation(bool commit);
  void onEventTrackBeginCpChanges(QString name);
  void onEventTrackEndCpChanges();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(xiiInt32 iPurpose = 0);
  void UpdateEventTrackEditor();

  xiiClock                 m_Clock;
  xiiEngineViewConfig      m_ViewConfig;
  xiiQtOrbitCamViewWidget* m_pViewWidget   = nullptr;
  xiiQtTimeScrubberWidget* m_pTimeScrubber = nullptr;
  xiiTime                  m_ClipDuration;
  xiiTime                  m_PlaybackPosition;

  xiiQtDocumentPanel*          m_pEventTrackPanel  = nullptr;
  xiiQtEventTrackEditorWidget* m_pEventTrackEditor = nullptr;
};
