#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <GuiFoundation/Models/LogModel.moc.h>

XII_IMPLEMENT_SINGLETON(xiiQtLogPanel);

xiiQtLogPanel::xiiQtLogPanel() :
  xiiQtApplicationPanel("Panel.Log"), m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  setIcon(xiiQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Log.svg"));
  setWindowTitle(xiiMakeQString(xiiTranslate("Panel.Log")));
  setWidget(pDummy);

  EditorLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Editor Log"));
  EngineLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Engine Log"));
  CombinedLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Log"));

  xiiGlobalLog::AddLogWriter(xiiMakeDelegate(&xiiQtLogPanel::LogWriter, this));
  xiiEditorEngineProcessConnection::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtLogPanel::EngineProcessMsgHandler, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    splitter->restoreState(Settings.value("Splitter", splitter->saveState()).toByteArray());
  }
  Settings.endGroup();

  connect(EditorLog->GetLog(), &xiiQtLogModel::NewErrorsOrWarnings, this, &xiiQtLogPanel::OnNewWarningsOrErrors);
  connect(EngineLog->GetLog(), &xiiQtLogModel::NewErrorsOrWarnings, this, &xiiQtLogPanel::OnNewWarningsOrErrors);
  connect(CombinedLog->GetLog(), &xiiQtLogModel::NewErrorsOrWarnings, this, &xiiQtLogPanel::OnNewWarningsOrErrors);

  xiiQtUiServices::GetSingleton()->s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtLogPanel::UiServiceEventHandler, this));

  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
  pPreferences->m_ChangedEvent.AddEventHandler(xiiMakeDelegate(&xiiQtLogPanel::OnPreferenceChange, this));

  m_bCombineLogs = pPreferences->m_bCombinedEditorAndEngineLogs;

  LogWidgets->setCurrentIndex(m_bCombineLogs ? 0 : 1);
}

xiiQtLogPanel::~xiiQtLogPanel()
{
  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
  pPreferences->m_ChangedEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtLogPanel::OnPreferenceChange, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    Settings.setValue("Splitter", splitter->saveState());
  }
  Settings.endGroup();

  xiiGlobalLog::RemoveLogWriter(xiiMakeDelegate(&xiiQtLogPanel::LogWriter, this));
  xiiEditorEngineProcessConnection::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtLogPanel::EngineProcessMsgHandler, this));
  xiiQtUiServices::GetSingleton()->s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtLogPanel::UiServiceEventHandler, this));
}

void xiiQtLogPanel::OnNewWarningsOrErrors(xiiStringView sText, bool bError)
{
  m_uiKnownNumWarnings = EditorLog->GetLog()->GetNumSeriousWarnings() + EditorLog->GetLog()->GetNumWarnings() + EngineLog->GetLog()->GetNumSeriousWarnings() + EngineLog->GetLog()->GetNumWarnings() + CombinedLog->GetLog()->GetNumSeriousWarnings() + CombinedLog->GetLog()->GetNumWarnings();
  m_uiKnownNumErrors = EditorLog->GetLog()->GetNumErrors() + EngineLog->GetLog()->GetNumErrors() + CombinedLog->GetLog()->GetNumErrors();

  xiiQtUiServices::Event::TextType type = xiiQtUiServices::Event::Info;

  xiiUInt32 uiShowNumWarnings = 0;
  xiiUInt32 uiShowNumErrors   = 0;

  if (m_uiKnownNumWarnings > m_uiIgnoreNumWarnings)
  {
    uiShowNumWarnings = m_uiKnownNumWarnings - m_uiIgnoreNumWarnings;
    type              = xiiQtUiServices::Event::Warning;
  }
  else
  {
    m_uiIgnoreNumWarnings = m_uiKnownNumWarnings;
  }

  if (m_uiKnownNumErrors > m_uiIgnoredNumErrors)
  {
    uiShowNumErrors = m_uiKnownNumErrors - m_uiIgnoredNumErrors;
    type            = xiiQtUiServices::Event::Error;
  }
  else
  {
    m_uiIgnoredNumErrors = m_uiKnownNumErrors;
  }

  xiiStringBuilder tmp;
  if (uiShowNumErrors > 0)
  {
    tmp.AppendFormat("{} Errors", uiShowNumErrors);
  }
  if (uiShowNumWarnings > 0)
  {
    tmp.AppendWithSeparator(",", " ");
    tmp.AppendFormat("{} Warnings", uiShowNumWarnings);
  }

  xiiQtUiServices::GetSingleton()->ShowAllDocumentsPermanentStatusBarMessage(tmp, type);

  if (!sText.IsEmpty())
  {
    xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("{}: {}", bError ? "Error" : "Warning", sText), xiiTime::MakeFromSeconds(10));
  }
}

void xiiQtLogPanel::ToolsProjectEventHandler(const xiiToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectClosing:
      CombinedLog->GetLog()->Clear();
      EditorLog->GetLog()->Clear();
      EngineLog->GetLog()->Clear();
      [[fallthrough]];

    case xiiToolsProjectEvent::Type::ProjectOpened:
      setEnabled(e.m_Type == xiiToolsProjectEvent::Type::ProjectOpened);
      break;

    default:
      break;
  }

  xiiQtApplicationPanel::ToolsProjectEventHandler(e);
}

void xiiQtLogPanel::LogWriter(const xiiLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  xiiLogEntry msg(e);

  if (m_bCombineLogs)
    CombinedLog->GetLog()->AddLogMsg(msg);
  else
    EditorLog->GetLog()->AddLogMsg(msg);

  if (msg.m_sTag == "EditorStatus")
  {
    xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt(msg.m_sMsg), xiiTime::MakeFromSeconds(5));
  }
}

void xiiQtLogPanel::EngineProcessMsgHandler(const xiiEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case xiiEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (const xiiLogMsgToEditor* pMsg = xiiDynamicCast<const xiiLogMsgToEditor*>(e.m_pMsg))
      {
        if (m_bCombineLogs)
          CombinedLog->GetLog()->AddLogMsg(pMsg->m_Entry);
        else
          EngineLog->GetLog()->AddLogMsg(pMsg->m_Entry);
      }
    }
    break;

    default:
      return;
  }
}

void xiiQtLogPanel::UiServiceEventHandler(const xiiQtUiServices::Event& e)
{
  if (e.m_Type == xiiQtUiServices::Event::ClickedDocumentPermanentStatusBarText)
  {
    EnsureVisible();

    m_uiIgnoredNumErrors  = m_uiKnownNumErrors;
    m_uiIgnoreNumWarnings = m_uiKnownNumWarnings;

    xiiQtUiServices::GetSingleton()->ShowAllDocumentsPermanentStatusBarMessage(nullptr, xiiQtUiServices::Event::Info);
  }
}

void xiiQtLogPanel::OnPreferenceChange(xiiPreferences* pref)
{
  if (xiiEditorPreferencesUser* pPref = xiiDynamicCast<xiiEditorPreferencesUser*>(pref))
  {
    if (m_bCombineLogs != pPref->m_bCombinedEditorAndEngineLogs)
    {
      m_bCombineLogs = pPref->m_bCombinedEditorAndEngineLogs;

      LogWidgets->setCurrentIndex(m_bCombineLogs ? 0 : 1);
    }
  }
}
