/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Settings/EditorSettings.h>
#include <EditorFramework/Settings/ProjectPreferences.h>
#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <ToolsFoundation/Project/ToolsProject.h>

bool xiiQtExportAndRunDlg::s_bTransformAll    = true;
bool xiiQtExportAndRunDlg::s_bUpdateThumbnail = false;
bool xiiQtExportAndRunDlg::s_bCompileCpp      = true;

static int s_iLastPlayerApp = 0;

xiiQtExportAndRunDlg::xiiQtExportAndRunDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);
  label->setText("Base editor arguments (configuration overrides and session identity are applied at launch):");
  label->setWordWrap(true);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  ToolCombo->addItem("xiiPlayer", "xiiPlayer.exe");
#else
  ToolCombo->addItem("xiiPlayer", "xiiPlayer");
#endif

  xiiProjectPreferencesUser* pPref = xiiEditorSettings::QuerySettings<xiiProjectPreferencesUser>();

  for (const auto& app : pPref->m_PlayerApps)
  {
    xiiStringBuilder name = xiiPathUtils::GetFileName(app);

    ToolCombo->addItem(name.GetData(), QString::fromUtf8(app.GetData()));
  }

  ToolCombo->setCurrentIndex(s_iLastPlayerApp);

  m_CppSettings.Load().IgnoreResult();
  resize(740, 820);
  m_sConfigPath     = xiiStringBuilder(xiiToolsProject::GetSingleton()->GetProjectDirectory(), "/RuntimeConfigs/Player.ddl");
  m_bConfigReadable = m_ProjectConfig.Load(m_sConfigPath).Succeeded();
  auto* form        = new QFormLayout;
  m_pLaunchName     = new QComboBox(this);
  m_pLaunchName->setEditable(true);
  m_pLaunchName->setObjectName("PlayerLaunchConfiguration");
  for (const auto& launch : m_ProjectConfig.m_LaunchConfigurations)
    m_pLaunchName->addItem(QString::fromUtf8(launch.m_sName.GetData()));
  if (m_pLaunchName->count() == 0)
    m_pLaunchName->addItem("Default");
  auto* names = new QHBoxLayout;
  names->addWidget(m_pLaunchName);
  auto* save   = new QPushButton("Save Configuration", this);
  auto* remove = new QPushButton("Delete", this);
  names->addWidget(save);
  names->addWidget(remove);
  form->addRow("Launch configuration", names);
  m_pMainScene = new QLineEdit(QString::fromUtf8(xiiToolsProject::GetSingleton()->GetManifest().m_sDefaultScene.GetData()), this);
  m_pMainScene->setPlaceholderText("Project-relative source scene path; shared with standalone Player");
  form->addRow("Project main scene", m_pMainScene);
  auto* currentMain = new QPushButton("Use Current Scene as Main", this);
  form->addRow("", currentMain);
  connect(currentMain, &QPushButton::clicked, this, [this]() { m_pMainScene->setText(QString::fromUtf8(m_sCurrentSourceScene.GetData())); });
  m_pSceneOverride = new QLineEdit(this);
  m_pSceneOverride->setPlaceholderText("Empty uses the project main scene");
  form->addRow("Startup scene override", m_pSceneOverride);
  m_pUseCurrent = new QCheckBox("Launch the current exported scene (temporary override)", this);
  m_pUseCurrent->setChecked(true);
  form->addRow("", m_pUseCurrent);
  m_pProjectOverride = new QLineEdit(this);
  m_pProjectOverride->setPlaceholderText("Empty uses this project; relative to this project");
  form->addRow("Project override", m_pProjectOverride);
  m_pInstances = new QSpinBox(this);
  m_pInstances->setRange(1, 16);
  m_pInstances->setObjectName("PlayerInstanceCount");
  form->addRow("Instances", m_pInstances);
  m_pRole = new QComboBox(this);
  m_pRole->addItems({"standalone", "server", "client", "server-clients"});
  form->addRow("Application role", m_pRole);
  m_pConnect = new QLineEdit("127.0.0.1", this);
  form->addRow("Connect address", m_pConnect);
  m_pBasePort = new QSpinBox(this);
  m_pBasePort->setRange(0, 65503);
  m_pBasePort->setSpecialValueText("Automatic session block");
  form->addRow("Base port", m_pBasePort);
  m_pArrange = new QCheckBox("Arrange instances side by side", this);
  m_pArrange->setChecked(true);
  form->addRow("", m_pArrange);
  m_pArguments = new QPlainTextEdit(this);
  m_pArguments->setMaximumHeight(60);
  m_pArguments->setPlaceholderText("Additional arguments, e.g. -App.VSync false -App.FrameLimit 60 -show-fps true");
  form->addRow("Arguments", m_pArguments);
  m_pInstanceArguments = new QPlainTextEdit(this);
  m_pInstanceArguments->setMaximumHeight(60);
  m_pInstanceArguments->setPlaceholderText("One command line per instance; e.g. -window-x 0 -width 800");
  form->addRow("Instance overrides", m_pInstanceArguments);
  verticalLayout->insertLayout(0, form);
  auto* policy = new QLabel("Editor instances use private preferences, logs, temporary files and IPC. Closing the editor stops its Players. Roles are passed to application networking plugins.", this);
  policy->setWordWrap(true);
  verticalLayout->insertWidget(1, policy);
  if (!m_bConfigReadable)
  {
    save->setEnabled(false);
    remove->setEnabled(false);
    policy->setText("Cannot read RuntimeConfigs/Player.ddl. The existing file will be preserved; repair it before saving launch settings.");
  }
  connect(m_pLaunchName, &QComboBox::currentIndexChanged, this, [this]() { SelectLaunch(); });
  connect(save, &QPushButton::clicked, this, [this]() { SaveLaunch(); });
  connect(remove, &QPushButton::clicked, this, [this]() {
    const auto name = m_pLaunchName->currentText().toUtf8();
    for (xiiUInt32 i = 0; i < m_ProjectConfig.m_LaunchConfigurations.GetCount(); ++i)
      if (m_ProjectConfig.m_LaunchConfigurations[i].m_sName == name.constData())
      {
        m_ProjectConfig.m_LaunchConfigurations.RemoveAtAndCopy(i);
        if (m_ProjectConfig.Save(m_sConfigPath).Failed())
          QMessageBox::warning(this, "Player Configuration", "Could not save the launch configuration file.");
        else
          m_pLaunchName->removeItem(m_pLaunchName->currentIndex());
        break;
      }
  });
  SelectLaunch();
}

void xiiQtExportAndRunDlg::PullFromUI()
{
  m_Launch.m_sApplication    = ToolCombo->currentData().toString().toUtf8().constData();
  m_Launch.m_sName           = m_pLaunchName->currentText().trimmed().toUtf8().constData();
  m_Launch.m_sScene          = m_pSceneOverride->text().trimmed().toUtf8().constData();
  m_Launch.m_sProject        = m_pProjectOverride->text().trimmed().toUtf8().constData();
  m_Launch.m_uiInstances     = m_pInstances->value();
  m_Launch.m_sRole           = m_pRole->currentText().toUtf8().constData();
  m_Launch.m_sConnect        = m_pConnect->text().toUtf8().constData();
  m_Launch.m_uiBasePort      = m_pBasePort->value();
  m_Launch.m_bArrangeWindows = m_pArrange->isChecked();
  m_Launch.m_Arguments.Clear();
  xiiProcessOptions extra;
  extra.AddCommandLine(m_pArguments->toPlainText().toUtf8().constData());
  for (const auto& arg : extra.m_Arguments)
    m_Launch.m_Arguments.PushBack(arg);
  m_Launch.m_InstanceArguments.Clear();
  const auto lines = m_pInstanceArguments->toPlainText().split('\n');
  if (!m_pInstanceArguments->toPlainText().isEmpty())
    for (const auto& line : lines)
      m_Launch.m_InstanceArguments.PushBack(line.toUtf8().constData());
  m_bUseCurrentScene = m_pUseCurrent->isChecked();
  s_bTransformAll    = TransformAll->isChecked();
  s_bUpdateThumbnail = UpdateThumbnail->isChecked();
  s_iLastPlayerApp   = ToolCombo->currentIndex();
  s_bCompileCpp      = CompileCpp->isChecked();

  xiiProjectPreferencesUser* pPref = xiiEditorSettings::QuerySettings<xiiProjectPreferencesUser>();
  pPref->m_PlayerApps.Clear();

  for (int i = 1; i < ToolCombo->count(); ++i)
  {
    xiiStringBuilder path = ToolCombo->itemData(i).toString().toUtf8().data();
    path.MakeCleanPath();

    pPref->m_PlayerApps.PushBack(path);
  }
}

void xiiQtExportAndRunDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  UpdateThumbnail->setVisible(m_bShowThumbnailCheckbox);
  TransformAll->setChecked(s_bTransformAll);
  UpdateThumbnail->setChecked(s_bUpdateThumbnail);
  PlayerCmdLine->setPlainText(m_sCmdLine.GetData());

  if (!xiiCppProject::ExistsProjectCMakeListsTxt())
  {
    CompileCpp->setEnabled(false);
    CompileCpp->setToolTip("This project doesn't have a C++ plugin.");
    CompileCpp->setChecked(false);
  }
  else
  {
    CompileCpp->setChecked(s_bCompileCpp);
  }
}

void xiiQtExportAndRunDlg::on_ExportOnly_clicked()
{
  PullFromUI();
  m_bRunAfterExport = false;
  accept();
}

void xiiQtExportAndRunDlg::on_ExportAndRun_clicked()
{
  PullFromUI();
  if (m_Launch.Validate().Failed())
  {
    QMessageBox::warning(this, "Player Configuration", "Use a configuration name, 1-16 instances, a valid port block and at most one override line per instance.");
    return;
  }
  if (m_bUseCurrentScene && !m_Launch.m_sProject.IsEmpty())
  {
    QMessageBox::warning(this, "Player Configuration", "Disable the current-scene override when launching another project. Its assets belong to that project.");
    return;
  }
  m_bRunAfterExport = true;
  accept();
}

void xiiQtExportAndRunDlg::on_AddToolButton_clicked()
{
  xiiStringBuilder appDir = xiiOSFile::GetApplicationDirectory();
  appDir.MakeCleanPath();
  static QString sLastPath = appDir.GetData();

  const QString sFile = QFileDialog::getOpenFileName(this, "Select Program", sLastPath, "Applicaation (*.exe)", nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  sLastPath = sFile;

  xiiStringBuilder path = sFile.toUtf8().data();
  path.MakeCleanPath();
  path.TrimWordStart(appDir);
  path.Trim("/", "");

  xiiStringBuilder tmp;
  ToolCombo->addItem(QString::fromUtf8(path.GetFileName().GetData(tmp)), QString::fromUtf8(path.GetData()));
  ToolCombo->setCurrentIndex(ToolCombo->count() - 1);
}

void xiiQtExportAndRunDlg::on_RemoveToolButton_clicked()
{
  ToolCombo->removeItem(ToolCombo->currentIndex());
}

void xiiQtExportAndRunDlg::on_ToolCombo_currentIndexChanged(int idx)
{
  RemoveToolButton->setEnabled(idx != 0);
}

void xiiQtExportAndRunDlg::SelectLaunch()
{
  const auto* launch = m_ProjectConfig.FindLaunch(m_pLaunchName->currentText().toUtf8().constData());
  if (!launch)
    return;
  m_Launch = *launch;
  m_pUseCurrent->setChecked(false);
  m_pSceneOverride->setText(QString::fromUtf8(launch->m_sScene.GetData()));
  m_pProjectOverride->setText(QString::fromUtf8(launch->m_sProject.GetData()));
  m_pInstances->setValue(launch->m_uiInstances);
  m_pRole->setCurrentText(QString::fromUtf8(launch->m_sRole.GetData()));
  m_pConnect->setText(QString::fromUtf8(launch->m_sConnect.GetData()));
  m_pBasePort->setValue(launch->m_uiBasePort);
  m_pArrange->setChecked(launch->m_bArrangeWindows);
  xiiProcessOptions options;
  for (const auto& arg : launch->m_Arguments)
    options.m_Arguments.PushBack(arg);
  xiiStringBuilder command;
  options.BuildCommandLineString(command);
  m_pArguments->setPlainText(QString::fromUtf8(command.GetData()));
  QStringList lines;
  for (const auto& line : launch->m_InstanceArguments)
    lines.push_back(QString::fromUtf8(line.GetData()));
  m_pInstanceArguments->setPlainText(lines.join('\n'));
  const auto app   = QString::fromUtf8(launch->m_sApplication.GetData());
  const int  index = ToolCombo->findData(app);
  if (index >= 0)
    ToolCombo->setCurrentIndex(index);
  else if (app.isEmpty())
    ToolCombo->setCurrentIndex(0);
  else
  {
    ToolCombo->addItem(QFileInfo(app).baseName(), app);
    ToolCombo->setCurrentIndex(ToolCombo->count() - 1);
  }
}

void xiiQtExportAndRunDlg::SaveLaunch()
{
  if (!m_bConfigReadable)
    return;
  PullFromUI();
  if (m_Launch.Validate().Failed())
  {
    QMessageBox::warning(this, "Player Configuration", "Invalid launch settings; check the name, instance count, ports and override lines.");
    return;
  }
  auto manifest             = xiiToolsProject::GetSingleton()->GetManifest();
  manifest.m_sDefaultScene  = m_pMainScene->text().trimmed().toUtf8().constData();
  const auto manifestResult = xiiToolsProject::GetSingleton()->SaveManifest(manifest);
  if (manifestResult.Failed())
  {
    QMessageBox::warning(this, "Player Configuration", QString::fromUtf8(manifestResult.GetMessageString().GetData()));
    return;
  }
  bool found = false;
  for (auto& launch : m_ProjectConfig.m_LaunchConfigurations)
    if (launch.m_sName == m_Launch.m_sName)
    {
      launch = m_Launch;
      found  = true;
      break;
    }
  if (!found)
    m_ProjectConfig.m_LaunchConfigurations.PushBack(m_Launch);
  if (m_ProjectConfig.Save(m_sConfigPath).Failed())
    QMessageBox::warning(this, "Player Configuration", "Could not save RuntimeConfigs/Player.ddl. Check project write access.");
  else if (m_pLaunchName->findText(QString::fromUtf8(m_Launch.m_sName.GetData())) < 0)
    m_pLaunchName->addItem(QString::fromUtf8(m_Launch.m_sName.GetData()));
}
