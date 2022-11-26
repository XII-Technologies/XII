#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>

void xiiQtEditorApp::AddRestartRequiredReason(const char* szReason)
{
  if (!m_RestartRequiredReasons.Find(szReason).IsValid())
  {
    m_RestartRequiredReasons.Insert(szReason);
    UpdateGlobalStatusBarMessage();
  }

  xiiStringBuilder s;
  s.Format("The editor process must be restarted.\nReason: '{0}'\n\nDo you want to restart now?", szReason);

  if (xiiQtUiServices::MessageBoxQuestion(s, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
  {
    if (xiiToolsProject::CanCloseProject())
    {
      LaunchEditor(xiiToolsProject::GetSingleton()->GetProjectFile(), false);

      QApplication::closeAllWindows();
      return;
    }
  }
}

void xiiQtEditorApp::AddReloadProjectRequiredReason(const char* szReason)
{
  if (!m_ReloadProjectRequiredReasons.Find(szReason).IsValid())
  {
    m_ReloadProjectRequiredReasons.Insert(szReason);

    xiiStringBuilder s;
    s.Format("The project must be reloaded.\nReason: '{0}'", szReason);

    xiiQtUiServices::MessageBoxInformation(s);

    UpdateGlobalStatusBarMessage();
  }
}

void xiiQtEditorApp::UpdateGlobalStatusBarMessage()
{
  xiiStringBuilder sText;

  if (!m_RestartRequiredReasons.IsEmpty())
    sText.Append("Restart the editor to apply changes.   ");

  if (!m_ReloadProjectRequiredReasons.IsEmpty())
    sText.Append("Reload the project to apply changes.   ");

  xiiQtUiServices::ShowGlobalStatusBarMessage(sText);
}
