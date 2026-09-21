/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Time/Time.h>

#include <QToolBar>
#include <QWidget>

class QMouseEvent;
class QPushButton;
class QLineEdit;

class XII_GUIFOUNDATION_DLL xiiQtTimeScrubberWidget : public QWidget
{
  Q_OBJECT

public:
  explicit xiiQtTimeScrubberWidget(QWidget* pParent);
  ~xiiQtTimeScrubberWidget();

  /// Sets the duration in 'ticks'. There are 4800 ticks per second.
  void SetDuration(xiiUInt64 uiNumTicks);

  /// Sets the duration.
  void SetDuration(xiiTime time);

  /// Sets the current position in 'ticks'. There are 4800 ticks per second.
  void SetScrubberPosition(xiiUInt64 uiTick);

  /// Sets the current position.
  void SetScrubberPosition(xiiTime time);

Q_SIGNALS:
  void ScrubberPosChangedEvent(xiiUInt64 uiNewScrubberTickPos);

private:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  void         SetScrubberPosFromPixelCoord(xiiInt32 x);

  xiiUInt64 m_uiDurationTicks = 0;
  xiiTime   m_Duration;
  xiiUInt64 m_uiScrubberTickPos     = 0;
  double    m_fNormScrubberPosition = 0.0;
  bool      m_bDragging             = false;
};

class XII_GUIFOUNDATION_DLL xiiQtTimeScrubberToolbar : public QToolBar
{
  Q_OBJECT

public:
  explicit xiiQtTimeScrubberToolbar(QWidget* pParent);

  /// Sets the duration in 'ticks'. There are 4800 ticks per second.
  void SetDuration(xiiUInt64 uiNumTicks);

  /// Sets the current position in 'ticks'. There are 4800 ticks per second.
  void SetScrubberPosition(xiiUInt64 uiTick);

  void SetButtonState(bool bPlaying, bool bRepeatEnabled);

Q_SIGNALS:
  void ScrubberPosChangedEvent(xiiUInt64 uiNewScrubberTickPos);
  void PlayPauseEvent();
  void RepeatEvent();
  void DurationChangedEvent(double fDuration);
  void AdjustDurationEvent();

private:
  xiiQtTimeScrubberWidget* m_pScrubber             = nullptr;
  QPushButton*             m_pPlayButton           = nullptr;
  QPushButton*             m_pRepeatButton         = nullptr;
  QLineEdit*               m_pDuration             = nullptr;
  QPushButton*             m_pAdjustDurationButton = nullptr;
};
