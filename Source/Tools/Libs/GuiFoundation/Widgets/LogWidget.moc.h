/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_LogWidget.h>
#include <QWidget>

class xiiQtLogModel;
class xiiQtSearchWidget;

/// The application wide panel that shows the engine log output and the editor log output
class XII_GUIFOUNDATION_DLL xiiQtLogWidget : public QWidget, public Ui_LogWidget
{
  Q_OBJECT

public:
  xiiQtLogWidget(QWidget* pParent);
  ~xiiQtLogWidget();

  void ShowControls(bool bShow);

  xiiQtLogModel*      GetLog();
  xiiQtSearchWidget*  GetSearchWidget();
  void                SetLogLevel(xiiLogMsgType::Enum logLevel);
  xiiLogMsgType::Enum GetLogLevel() const;

  virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;

  using LogItemContextActionCallback = xiiDelegate<void(const xiiStringView& sLogText)>;
  static bool AddLogItemContextActionCallback(const xiiStringView& sName, const LogItemContextActionCallback& logCallback);
  static bool RemoveLogItemContextActionCallback(const xiiStringView& sName);

private Q_SLOTS:
  void on_ButtonClearLog_clicked();
  void on_Search_textChanged(const QString& text);
  void on_ComboFilter_currentIndexChanged(int index);
  void OnItemDoubleClicked(QModelIndex idx);

private:
  xiiQtLogModel* m_pLog;
  void           ScrollToBottomIfAtEnd(int iNumElements);

  /// List of callbacks invoked when the user double clicks a log message
  static xiiMap<xiiString, LogItemContextActionCallback> s_LogCallbacks;
};
