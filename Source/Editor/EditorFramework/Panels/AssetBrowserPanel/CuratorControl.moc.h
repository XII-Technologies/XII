/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QWidget>

struct xiiAssetCuratorEvent;
struct xiiToolsProjectEvent;
struct xiiAssetProcessorEvent;
class QToolButton;

/// \brief
class XII_EDITORFRAMEWORK_DLL xiiQtCuratorControl : public QWidget
{
  Q_OBJECT
public:
  explicit xiiQtCuratorControl(QWidget* pParent);
  ~xiiQtCuratorControl();

protected:
  virtual void paintEvent(QPaintEvent* e) override;

private Q_SLOTS:
  void SlotUpdateTransformStats();
  void UpdateBackgroundProcessState();
  void BackgroundProcessClicked(bool checked);

private:
  void ScheduleUpdateTransformStats();
  void AssetCuratorEvents(const xiiAssetCuratorEvent& e);
  void AssetProcessorEvents(const xiiAssetProcessorEvent& e);
  void ProjectEvents(const xiiToolsProjectEvent& e);

  bool         m_bScheduled         = false;
  QToolButton* m_pBackgroundProcess = nullptr;
};
