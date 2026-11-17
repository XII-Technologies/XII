/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Diagnostics/TelemetryProtocol.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qlistwidget.h>

xiiQtLogDockWidget* xiiQtLogDockWidget::s_pWidget = nullptr;

xiiQtLogDockWidget::xiiQtLogDockWidget(ads::CDockManager* pDockManager, QWidget* pParent) :
  ads::CDockWidget(pDockManager, "Log", pParent)
{
  s_pWidget = this;
  setupUi(this);
  LogWidget->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Log"));

  setIcon(QIcon(":/Icons/Icons/Log.svg"));

  this->setWidget(LogWidget);
}

void xiiQtLogDockWidget::ResetStats()
{
  LogWidget->GetLog()->Clear();
}

void xiiQtLogDockWidget::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (s_pWidget && sample.m_uiDropped != 0)
    s_pWidget->Log(xiiFmt("Diagnostics omitted {} log samples before this entry.", sample.m_uiDropped));
  if (s_pWidget == nullptr)
    return;
  const auto* type        = sample.m_Fields.GetValue("type");
  const auto* indentation = sample.m_Fields.GetValue("indentation");
  const auto* tag         = sample.m_Fields.GetValue("tag");
  const auto* text        = sample.m_Fields.GetValue("text");
  if (type == nullptr || !type->IsA<xiiInt32>() || indentation == nullptr || !indentation->IsA<xiiUInt32>() ||
      tag == nullptr || !tag->IsA<xiiString>() || text == nullptr || !text->IsA<xiiString>())
    return;
  const auto value = type->Get<xiiInt32>();
  if (value < xiiLogMsgType::Flush || value >= xiiLogMsgType::ENUM_COUNT)
    return;
  xiiLogEntry entry;
  entry.m_Type          = static_cast<xiiLogMsgType::Enum>(value);
  entry.m_uiIndentation = static_cast<xiiUInt8>(xiiMath::Min(indentation->Get<xiiUInt32>(), 255U));
  entry.m_sTag          = tag->Get<xiiString>();
  entry.m_sMsg          = text->Get<xiiString>();
  if (const auto* seconds = sample.m_Fields.GetValue("seconds"); seconds != nullptr && seconds->IsA<double>())
    entry.m_fSeconds = seconds->Get<double>();
  s_pWidget->LogWidget->GetLog()->AddLogMsg(entry);
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
