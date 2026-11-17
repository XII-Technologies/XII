/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Diagnostics/TelemetryService.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qspinbox.h>

class xiiCommandInterpreterInspector : public xiiCommandInterpreter
{
public:
  explicit xiiCommandInterpreterInspector(xiiTelemetryClient& client) : m_Client(client) {}
  void Interpret(xiiCommandInterpreterState& state) override
  {
    xiiNetworkRpcRequest request;
    if (m_Client.Console(state.m_sInput, false, request).Failed())
      xiiQtLogDockWidget::s_pWidget->Log("Console request could not be submitted.");
  }
  void AutoComplete(xiiCommandInterpreterState& state) override
  {
    xiiNetworkRpcRequest request;
    if (m_Client.Console(state.m_sInput, true, request).Failed())
      xiiQtLogDockWidget::s_pWidget->Log("Completion request could not be submitted.");
  }

private:
  xiiTelemetryClient& m_Client;
};

xiiQtCVarsWidget* xiiQtCVarsWidget::s_pWidget = nullptr;

xiiQtCVarsWidget::xiiQtCVarsWidget(ads::CDockManager* pDockManager, QWidget* pParent) :
  ads::CDockWidget(pDockManager, "CVars", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(CVarWidget);

  setIcon(QIcon(":/GuiFoundation/Icons/CVar.svg"));

  connect(CVarWidget, &xiiQtCVarWidget::onBoolChanged, this, &xiiQtCVarsWidget::BoolChanged);
  connect(CVarWidget, &xiiQtCVarWidget::onFloatChanged, this, &xiiQtCVarsWidget::FloatChanged);
  connect(CVarWidget, &xiiQtCVarWidget::onDoubleChanged, this, &xiiQtCVarsWidget::DoubleChanged);
  connect(CVarWidget, &xiiQtCVarWidget::onIntChanged, this, &xiiQtCVarsWidget::IntChanged);
  connect(CVarWidget, &xiiQtCVarWidget::onStringChanged, this, &xiiQtCVarsWidget::StringChanged);

  CVarWidget->GetConsole().SetCommandInterpreter(XII_DEFAULT_NEW(xiiCommandInterpreterInspector, xiiQtMainWindow::s_pWidget->GetDiagnosticsClient()));

  ResetStats();
}

void xiiQtCVarsWidget::ResetStats()
{
  m_CVars.Clear();
  CVarWidget->Clear();
}

void xiiQtCVarsWidget::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (!s_pWidget)
    return;
  xiiMap<xiiString, xiiCVarWidgetData> values;
  bool                                 changed = false;
  for (const auto& row : sample.m_Records)
  {
    xiiString         name;
    xiiCVarWidgetData data;
    if (!row.Read("name", name) || !row.Read("plugin", data.m_sPlugin) || !row.Read("description", data.m_sDescription) || !row.Read("type", data.m_uiType))
      continue;
    bool valid = false;
    switch (data.m_uiType)
    {
      case xiiCVarType::Bool:
        valid = row.Read("value", data.m_bValue);
        break;
      case xiiCVarType::Int:
        valid = row.Read("value", data.m_iValue);
        break;
      case xiiCVarType::Float:
        valid = row.Read("value", data.m_fValue);
        break;
      case xiiCVarType::Double:
        valid = row.Read("value", data.m_dValue);
        break;
      case xiiCVarType::String:
        valid = row.Read("value", data.m_sValue);
        break;
      default:
        break;
    }
    if (!valid)
      continue;
    const auto old = s_pWidget->m_CVars.Find(name);
    changed |= !old.IsValid() || old.Value().m_uiType != data.m_uiType || old.Value().m_sDescription != data.m_sDescription ||
      old.Value().m_sPlugin != data.m_sPlugin || old.Value().m_bValue != data.m_bValue || old.Value().m_iValue != data.m_iValue ||
      old.Value().m_fValue != data.m_fValue || old.Value().m_dValue != data.m_dValue || old.Value().m_sValue != data.m_sValue;
    values.Insert(name, std::move(data));
  }
  changed |= values.GetCount() != s_pWidget->m_CVars.GetCount();
  s_pWidget->m_CVars     = std::move(values);
  const auto permissions = xiiQtMainWindow::s_pWidget->GetDiagnosticsClient().GetServerInfo().m_uiPermissions;
  s_pWidget->CVarWidget->CVarsView->setEditTriggers((permissions & static_cast<xiiUInt8>(xiiTelemetryPermission::Configure)) ? QAbstractItemView::AllEditTriggers : QAbstractItemView::NoEditTriggers);
  s_pWidget->CVarWidget->ConsoleInput->setEnabled((permissions & static_cast<xiiUInt8>(xiiTelemetryPermission::Console)) != 0);
  if (changed)
    s_pWidget->CVarWidget->UpdateCVarUI(s_pWidget->m_CVars);
}

void xiiQtCVarsWidget::ReceiveCommandResult(const xiiNetworkRpcResult& result, const xiiTelemetryCommandResponse& response)
{
  if (!s_pWidget)
    return;
  if (result.m_Error.Failed() || response.m_uiError != 0)
  {
    const auto error = result.m_Error.Failed() ? result.m_Error : xiiNetworkDataError(static_cast<xiiNetworkDataErrorCode>(response.m_uiError));
    xiiQtLogDockWidget::s_pWidget->Log(xiiFmt("Diagnostics command {} failed: {}", result.m_Request.m_uiId, error.GetCodeName()));
  }
  else if (result.m_uiMethod == 2 || result.m_uiMethod == 3)
    s_pWidget->CVarWidget->AddConsoleStrings(xiiStringBuilder(response.m_sOutput));
}

void xiiQtCVarsWidget::SendCVarUpdateToServer(xiiStringView sName, const xiiCVarWidgetData& cvd)
{
  xiiVariant value;
  switch (cvd.m_uiType)
  {
    case xiiCVarType::Bool:
      value = cvd.m_bValue;
      break;
    case xiiCVarType::Int:
      value = cvd.m_iValue;
      break;
    case xiiCVarType::Float:
      value = cvd.m_fValue;
      break;
    case xiiCVarType::Double:
      value = cvd.m_dValue;
      break;
    case xiiCVarType::String:
      value = cvd.m_sValue;
      break;
    default:
      return;
  }
  xiiNetworkRpcRequest request;
  const auto           error = xiiQtMainWindow::s_pWidget->GetDiagnosticsClient().SetCVar(sName, value, request);
  if (error.Failed())
    xiiQtLogDockWidget::s_pWidget->Log(xiiFmt("Configuration request could not be submitted: {}", error.GetCodeName()));
}

void xiiQtCVarsWidget::BoolChanged(xiiStringView sCVar, bool newValue)
{
  auto& cvarData    = m_CVars[sCVar];
  cvarData.m_bValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void xiiQtCVarsWidget::FloatChanged(xiiStringView sCVar, float newValue)
{
  auto& cvarData    = m_CVars[sCVar];
  cvarData.m_fValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void xiiQtCVarsWidget::DoubleChanged(xiiStringView sCVar, double newValue)
{
  auto& cvarData    = m_CVars[sCVar];
  cvarData.m_dValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void xiiQtCVarsWidget::IntChanged(xiiStringView sCVar, int newValue)
{
  auto& cvarData    = m_CVars[sCVar];
  cvarData.m_iValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void xiiQtCVarsWidget::StringChanged(xiiStringView sCVar, xiiStringView sNewValue)
{
  auto& cvarData    = m_CVars[sCVar];
  cvarData.m_sValue = sNewValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}
