/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>

void xiiQtEditorApp::AddRestartRequiredReason(const char* szReason, bool bPrompt)
{
  if (!m_RestartRequiredReasons.Find(szReason).IsValid())
  {
    m_RestartRequiredReasons.Insert(szReason);
    UpdateRequiredActions();
  }

  if (!bPrompt)
    return;

  xiiStringBuilder s;
  s.SetFormat("The editor process must be restarted.\nReason: '{0}'\n\nDo you want to restart now?", szReason);

  if (xiiQtUiServices::MessageBoxQuestion(s, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
  {
    const auto* pProject = xiiToolsProject::GetSingleton();
    auto*       pWindow  = xiiQtContainerWindow::GetContainerWindow();
    if (pProject == nullptr || pWindow == nullptr)
    {
      return;
    }

    m_sRestartProjectFile = pProject->GetProjectFile();
    if (!pWindow->close())
    {
      m_sRestartProjectFile.Clear();
      return;
    }

    QCoreApplication::quit();
  }
}

void xiiQtEditorApp::AddReloadProjectRequiredReason(const char* szReason, bool bPrompt)
{
  if (!m_ReloadProjectRequiredReasons.Find(szReason).IsValid())
  {
    m_ReloadProjectRequiredReasons.Insert(szReason);

    xiiStringBuilder s;
    s.SetFormat("The project must be reloaded.\nReason: '{0}'", szReason);

    if (bPrompt)
      xiiQtUiServices::MessageBoxInformation(s);

    UpdateRequiredActions();
  }
}

void xiiQtEditorApp::UpdateRequiredActions()
{
  Q_EMIT RequiredActionsChanged(!m_RestartRequiredReasons.IsEmpty(), !m_ReloadProjectRequiredReasons.IsEmpty());
}
