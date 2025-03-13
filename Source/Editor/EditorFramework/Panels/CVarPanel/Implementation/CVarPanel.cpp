#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Console/Console.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

XII_IMPLEMENT_SINGLETON(xiiQtCVarPanel);

class xiiCommandInterpreterFwd : public xiiCommandInterpreter
{
public:
  virtual void Interpret(xiiCommandInterpreterState& inout_state) override
  {
    xiiConsoleCmdMsgToEngine msg;
    msg.m_iType    = 0;
    msg.m_sCommand = inout_state.m_sInput;

    xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  virtual void AutoComplete(xiiCommandInterpreterState& inout_state) override
  {
    xiiConsoleCmdMsgToEngine msg;
    msg.m_iType    = 1;
    msg.m_sCommand = inout_state.m_sInput;

    xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
};

xiiQtCVarPanel::xiiQtCVarPanel(ads::CDockManager* pDockManager) :
  xiiQtApplicationPanel(pDockManager, "Panel.CVar"), m_SingletonRegistrar(this)
{
  setIcon(xiiQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/CVar.svg"));
  setWindowTitle(xiiMakeQString(xiiTranslate("Panel.CVar")));
  m_pCVarWidget = new xiiQtCVarWidget(this);
  m_pCVarWidget->layout()->setContentsMargins(0, 0, 0, 0);
  // m_pCVarWidget->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pCVarWidget);

  xiiEditorEngineProcessConnection::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtCVarPanel::EngineProcessMsgHandler, this));

  connect(m_pCVarWidget, &xiiQtCVarWidget::onBoolChanged, this, &xiiQtCVarPanel::BoolChanged);
  connect(m_pCVarWidget, &xiiQtCVarWidget::onDoubleChanged, this, &xiiQtCVarPanel::DoubleChanged);
  connect(m_pCVarWidget, &xiiQtCVarWidget::onFloatChanged, this, &xiiQtCVarPanel::FloatChanged);
  connect(m_pCVarWidget, &xiiQtCVarWidget::onIntChanged, this, &xiiQtCVarPanel::IntChanged);
  connect(m_pCVarWidget, &xiiQtCVarWidget::onStringChanged, this, &xiiQtCVarPanel::StringChanged);

  m_pCVarWidget->GetConsole().SetCommandInterpreter(XII_DEFAULT_NEW(xiiCommandInterpreterFwd));
}

xiiQtCVarPanel::~xiiQtCVarPanel()
{
  xiiEditorEngineProcessConnection::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtCVarPanel::EngineProcessMsgHandler, this));
}

void xiiQtCVarPanel::ToolsProjectEventHandler(const xiiToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectClosing:
      m_EngineCVarState.Clear();
      m_pCVarWidget->Clear();

      [[fallthrough]];

    case xiiToolsProjectEvent::Type::ProjectOpened:
      setEnabled(e.m_Type == xiiToolsProjectEvent::Type::ProjectOpened);
      break;

    default:
      break;
  }

  xiiQtApplicationPanel::ToolsProjectEventHandler(e);
}

void xiiQtCVarPanel::EngineProcessMsgHandler(const xiiEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case xiiEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiCVarMsgToEditor>())
      {
        const xiiCVarMsgToEditor* pMsg = static_cast<const xiiCVarMsgToEditor*>(e.m_pMsg);

        bool  bExisted      = false;
        auto& cvar          = m_EngineCVarState.FindOrAdd(pMsg->m_sName, &bExisted).Value();
        cvar.m_sDescription = pMsg->m_sDescription;
        cvar.m_sPlugin      = pMsg->m_sPlugin;

        switch (pMsg->m_Value.GetType())
        {
          case xiiVariantType::Double:
            cvar.m_uiType = xiiCVarType::Double;
            cvar.m_dValue = pMsg->m_Value.ConvertTo<double>();
            break;
          case xiiVariantType::Float:
            cvar.m_uiType = xiiCVarType::Float;
            cvar.m_fValue = pMsg->m_Value.ConvertTo<float>();
            break;
          case xiiVariantType::Int32:
            cvar.m_uiType = xiiCVarType::Int;
            cvar.m_iValue = pMsg->m_Value.ConvertTo<int>();
            break;
          case xiiVariantType::Bool:
            cvar.m_uiType = xiiCVarType::Bool;
            cvar.m_bValue = pMsg->m_Value.ConvertTo<bool>();
            break;
          case xiiVariantType::String:
            cvar.m_uiType = xiiCVarType::String;
            cvar.m_sValue = pMsg->m_Value.ConvertTo<xiiString>();
            break;
          default:
            break;
        }

        if (!bExisted)
          m_bRebuildUI = true;

        if (!m_bUpdateUI)
        {
          m_bUpdateUI = true;

          // don't do this every single time, otherwise we would spam this during project load
          QTimer::singleShot(100, this, SLOT(UpdateUI()));
        }
      }
      else if (auto pMsg = xiiDynamicCast<const xiiConsoleCmdResultMsgToEditor*>(e.m_pMsg))
      {
        m_sCommandResult.Append(pMsg->m_sResult.GetView());
        m_bUpdateConsole = true;
        QTimer::singleShot(100, this, SLOT(UpdateUI()));
      }
    }
    break;
    default:
      break;
  }
}

void xiiQtCVarPanel::UpdateUI()
{
  if (m_bRebuildUI)
  {
    m_pCVarWidget->RebuildCVarUI(m_EngineCVarState);
  }
  else if (m_bUpdateUI)
  {
    m_pCVarWidget->UpdateCVarUI(m_EngineCVarState);
  }

  if (m_bUpdateConsole)
  {
    m_pCVarWidget->AddConsoleStrings(m_sCommandResult);
  }

  m_sCommandResult.Clear();
  m_bUpdateConsole = false;
  m_bUpdateUI      = false;
  m_bRebuildUI     = false;
}

void xiiQtCVarPanel::BoolChanged(xiiStringView sCVar, bool newValue)
{
  xiiChangeCVarMsgToEngine msg;
  msg.m_sCVarName = sCVar;
  msg.m_NewValue  = newValue;
  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtCVarPanel::DoubleChanged(xiiStringView sCVar, double newValue)
{
  xiiChangeCVarMsgToEngine msg;
  msg.m_sCVarName = sCVar;
  msg.m_NewValue  = newValue;
  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtCVarPanel::FloatChanged(xiiStringView sCVar, float newValue)
{
  xiiChangeCVarMsgToEngine msg;
  msg.m_sCVarName = sCVar;
  msg.m_NewValue  = newValue;
  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtCVarPanel::IntChanged(xiiStringView sCVar, int newValue)
{
  xiiChangeCVarMsgToEngine msg;
  msg.m_sCVarName = sCVar;
  msg.m_NewValue  = newValue;
  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtCVarPanel::StringChanged(xiiStringView sCVar, xiiStringView sNewValue)
{
  xiiChangeCVarMsgToEngine msg;
  msg.m_sCVarName = sCVar;
  msg.m_NewValue  = sNewValue;
  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
