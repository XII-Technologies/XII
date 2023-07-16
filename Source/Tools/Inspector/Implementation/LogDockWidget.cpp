#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qlistwidget.h>

xiiQtLogDockWidget* xiiQtLogDockWidget::s_pWidget = nullptr;

xiiQtLogDockWidget::xiiQtLogDockWidget(QWidget* pParent) :
  ads::CDockWidget("Log", pParent)
{
  s_pWidget = this;
  setupUi(this);
  LogWidget->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Log"));

  this->setWidget(LogWidget);
}

void xiiQtLogDockWidget::ResetStats()
{
  LogWidget->GetLog()->Clear();
}

void xiiQtLogDockWidget::Log(const xiiFormatString& text)
{
  xiiStringBuilder tmp;

  xiiLogEntry lm;
  lm.m_sMsg          = text.GetText(tmp);
  lm.m_Type          = xiiLogMsgType::InfoMsg;
  lm.m_uiIndentation = 0;
  LogWidget->GetLog()->AddLogMsg(lm);
}

void xiiQtLogDockWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  xiiTelemetryMessage Msg;

  while (xiiTelemetry::RetrieveMessage(' LOG', Msg) == XII_SUCCESS)
  {
    xiiLogEntry lm;
    xiiInt8     iEventType = 0;

    Msg.GetReader() >> iEventType;
    Msg.GetReader() >> lm.m_uiIndentation;
    Msg.GetReader() >> lm.m_sTag;
    Msg.GetReader() >> lm.m_sMsg;

    if (iEventType == xiiLogMsgType::EndGroup)
      Msg.GetReader() >> lm.m_fSeconds;

    lm.m_Type = (xiiLogMsgType::Enum)iEventType;
    s_pWidget->LogWidget->GetLog()->AddLogMsg(lm);
  }
}
