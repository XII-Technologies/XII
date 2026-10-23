/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QPointer>
#include <QProgressBar>
#include <QString>
#include <QSvgRenderer>
#include <QWidget>

class QScreen;

/// Lightweight startup presentation. GUI-thread only; no event pumping or application ownership.
/// Uses a SplashScreen window, scalable artwork, native text, and a local (never overall) work count.
class XII_EDITORFRAMEWORK_DLL xiiQtEditorSplashScreen final : public QWidget
{
public:
  explicit xiiQtEditorSplashScreen(QScreen* pScreen = nullptr);
  void SetStage(const QString& sStage, const QString& sDetail = {}, xiiUInt32 uiCompleted = 0, xiiUInt32 uiTotal = 0);
  void SetEditorWindow(QWidget* pWindow);

protected:
  void paintEvent(QPaintEvent* pEvent) override;
  void mousePressEvent(QMouseEvent* pEvent) override;
  void keyPressEvent(QKeyEvent* pEvent) override;
  bool eventFilter(QObject* pObject, QEvent* pEvent) override;

private:
  friend struct xiiEditorSplashScreenTest;
  QSvgRenderer m_Logo;
  QProgressBar m_Progress;
  QString      m_sStage;
  QString      m_sDetail;
  QString      m_sBuild;
  QString      m_sRevision;
};
