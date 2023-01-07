#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qspinbox.h>

class xiiCommandInterpreterInspector : public xiiCommandInterpreter
{
public:
  virtual void Interpret(xiiCommandInterpreterState& inout_State) override
  {
    xiiTelemetryMessage Msg;
    Msg.SetMessageID('CMD', 'EXEC');
    Msg.GetWriter() << inout_State.m_sInput;
    xiiTelemetry::SendToServer(Msg);
  }

  virtual void AutoComplete(xiiCommandInterpreterState& inout_State) override
  {
    xiiTelemetryMessage Msg;
    Msg.SetMessageID('CMD', 'COMP');
    Msg.GetWriter() << inout_State.m_sInput;
    xiiTelemetry::SendToServer(Msg);
  }
};

xiiQtCVarsWidget* xiiQtCVarsWidget::s_pWidget = nullptr;

xiiQtCVarsWidget::xiiQtCVarsWidget(QWidget* parent) :
  ads::CDockWidget("CVars", parent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(CVarWidget);

  connect(CVarWidget, &xiiQtCVarWidget::onBoolChanged, this, &xiiQtCVarsWidget::BoolChanged);
  connect(CVarWidget, &xiiQtCVarWidget::onFloatChanged, this, &xiiQtCVarsWidget::FloatChanged);
  connect(CVarWidget, &xiiQtCVarWidget::onDoubleChanged, this, &xiiQtCVarsWidget::DoubleChanged);
  connect(CVarWidget, &xiiQtCVarWidget::onIntChanged, this, &xiiQtCVarsWidget::IntChanged);
  connect(CVarWidget, &xiiQtCVarWidget::onStringChanged, this, &xiiQtCVarsWidget::StringChanged);

  CVarWidget->GetConsole().SetCommandInterpreter(XII_DEFAULT_NEW(xiiCommandInterpreterInspector));

  ResetStats();
}

void xiiQtCVarsWidget::ResetStats()
{
  m_CVarsBackup = m_CVars;
  m_CVars.Clear();
  CVarWidget->Clear();
}

void xiiQtCVarsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  xiiTelemetryMessage msg;

  bool bUpdateCVarsTable = false;
  bool bFillCVarsTable   = false;

  while (xiiTelemetry::RetrieveMessage('CVAR', msg) == XII_SUCCESS)
  {
    if (msg.GetMessageID() == ' CLR')
    {
      s_pWidget->m_CVars.Clear();
    }

    if (msg.GetMessageID() == 'SYNC')
    {
      for (auto it = s_pWidget->m_CVars.GetIterator(); it.IsValid(); ++it)
      {
        auto var = s_pWidget->m_CVarsBackup.Find(it.Key());

        if (var.IsValid() && it.Value().m_uiType == var.Value().m_uiType)
        {
          it.Value().m_bValue = var.Value().m_bValue;
          it.Value().m_fValue = var.Value().m_fValue;
          it.Value().m_dValue = var.Value().m_dValue;
          it.Value().m_sValue = var.Value().m_sValue;
          it.Value().m_iValue = var.Value().m_iValue;
        }
      }

      s_pWidget->CVarWidget->RebuildCVarUI(s_pWidget->m_CVars);

      s_pWidget->SyncAllCVarsToServer();
    }

    if (msg.GetMessageID() == 'DATA')
    {
      xiiString sName;
      msg.GetReader() >> sName;

      xiiCVarWidgetData& sd = s_pWidget->m_CVars[sName];

      msg.GetReader() >> sd.m_sPlugin;
      msg.GetReader() >> sd.m_uiType;
      msg.GetReader() >> sd.m_sDescription;

      switch (sd.m_uiType)
      {
        case xiiCVarType::Bool:
          msg.GetReader() >> sd.m_bValue;
          break;
        case xiiCVarType::Float:
          msg.GetReader() >> sd.m_fValue;
          break;
        case xiiCVarType::Double:
          msg.GetReader() >> sd.m_dValue;
        case xiiCVarType::Int:
          msg.GetReader() >> sd.m_iValue;
          break;
        case xiiCVarType::String:
          msg.GetReader() >> sd.m_sValue;
          break;
      }

      if (sd.m_bNewEntry)
        bUpdateCVarsTable = true;

      bFillCVarsTable = true;
    }
  }

  if (bUpdateCVarsTable)
    s_pWidget->CVarWidget->RebuildCVarUI(s_pWidget->m_CVars);
  else if (bFillCVarsTable)
    s_pWidget->CVarWidget->UpdateCVarUI(s_pWidget->m_CVars);
}

void xiiQtCVarsWidget::ProcessTelemetryConsole(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  xiiTelemetryMessage msg;
  xiiStringBuilder    tmp;

  while (xiiTelemetry::RetrieveMessage('CMD', msg) == XII_SUCCESS)
  {
    if (msg.GetMessageID() == 'RES')
    {
      msg.GetReader() >> tmp;
      s_pWidget->CVarWidget->AddConsoleStrings(tmp);
    }
  }
}

void xiiQtCVarsWidget::SyncAllCVarsToServer()
{
  for (auto it = m_CVars.GetIterator(); it.IsValid(); ++it)
    SendCVarUpdateToServer(it.Key().GetData(), it.Value());
}

void xiiQtCVarsWidget::SendCVarUpdateToServer(const char* szName, const xiiCVarWidgetData& cvd)
{
  xiiTelemetryMessage Msg;
  Msg.SetMessageID('SVAR', ' SET');
  Msg.GetWriter() << szName;
  Msg.GetWriter() << cvd.m_uiType;

  switch (cvd.m_uiType)
  {
    case xiiCVarType::Bool:
      Msg.GetWriter() << cvd.m_bValue;
      break;

    case xiiCVarType::Float:
      Msg.GetWriter() << cvd.m_fValue;
      break;

    case xiiCVarType::Double:
      Msg.GetWriter() << cvd.m_dValue;
      break;

    case xiiCVarType::Int:
      Msg.GetWriter() << cvd.m_iValue;
      break;

    case xiiCVarType::String:
      Msg.GetWriter() << cvd.m_sValue;
      break;
  }

  xiiTelemetry::SendToServer(Msg);
}

void xiiQtCVarsWidget::BoolChanged(const char* szCVar, bool newValue)
{
  auto& cvarData    = m_CVars[szCVar];
  cvarData.m_bValue = newValue;
  SendCVarUpdateToServer(szCVar, cvarData);
}

void xiiQtCVarsWidget::FloatChanged(const char* szCVar, float newValue)
{
  auto& cvarData    = m_CVars[szCVar];
  cvarData.m_fValue = newValue;
  SendCVarUpdateToServer(szCVar, cvarData);
}

void xiiQtCVarsWidget::DoubleChanged(const char* szCVar, double newValue)
{
  auto& cvarData    = m_CVars[szCVar];
  cvarData.m_dValue = newValue;
  SendCVarUpdateToServer(szCVar, cvarData);
}

void xiiQtCVarsWidget::IntChanged(const char* szCVar, int newValue)
{
  auto& cvarData    = m_CVars[szCVar];
  cvarData.m_iValue = newValue;
  SendCVarUpdateToServer(szCVar, cvarData);
}

void xiiQtCVarsWidget::StringChanged(const char* szCVar, const char* newValue)
{
  auto& cvarData    = m_CVars[szCVar];
  cvarData.m_sValue = newValue;
  SendCVarUpdateToServer(szCVar, cvarData);
}
