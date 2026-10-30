/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/WindowConfigPage.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

namespace
{
  QByteArray SerializeWindow(xiiWindowCreationDescription& desc)
  {
    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            stream(&storage);
    xiiOpenDdlWriter                 ddl;
    ddl.SetOutputStream(&stream);
    desc.SaveToDDL(ddl);
    return QByteArray(reinterpret_cast<const char*>(storage.GetData()), storage.GetStorageSize32());
  }

  xiiResult SaveWindow(xiiWindowCreationDescription& desc, xiiStringView path)
  {
    xiiDeferredFileWriter stream;
    stream.SetOutput(path);
    xiiOpenDdlWriter ddl;
    ddl.SetOutputStream(&stream);
    desc.SaveToDDL(ddl);
    return stream.Close();
  }
} // namespace

xiiQtWindowConfigPage::xiiQtWindowConfigPage(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);
  LoadDescs();

  m_ComboMonitor->addItem("Primary");
  m_ComboMonitor->addItem("Monitor 0");
  m_ComboMonitor->addItem("Monitor 1");
  m_ComboMonitor->addItem("Monitor 2");
  m_ComboMonitor->addItem("Monitor 3");
  m_ComboMonitor->addItem("Monitor 4");
  m_ComboMonitor->addItem("Monitor 5");

  m_ComboMode->addItem("Window (Fixed Resolution)");
  m_ComboMode->addItem("Window (Resizable)");
  m_ComboMode->addItem("Fullscreen (Borderless Wnd)");
  m_ComboMode->addItem("Fullscreen (Fixed Resolution)");

  FillUI(m_Descs[m_uiCurDesc]);

  m_ComboWnd->addItem("Project Default");
  m_ComboWnd->addItem("User Specific (xiiPlayer)");
  m_ComboWnd->setCurrentIndex(0);
  for (xiiUInt32 i = 0; i < 2; ++i)
    m_Committed[i] = SerializeWindow(m_Descs[i]);
  m_bCommittedOverride = m_bOverrideProjectDefault[1];
}

void xiiQtWindowConfigPage::FillUI(const xiiWindowCreationDescription& desc)
{
  m_LineEditTitle->setText(QString::fromUtf8(desc.m_Title.GetData()));
  m_ComboMonitor->setCurrentIndex(xiiMath::Clamp<xiiInt32>(desc.m_iMonitor, -1, 5) + 1);
  m_ComboMode->setCurrentIndex(xiiMath::Clamp<xiiInt32>(desc.m_WindowMode, 0, 3));

  m_SpinResX->setValue(desc.m_Resolution.width);
  m_SpinResY->setValue(desc.m_Resolution.height);

  m_ClipMouseCursor->setCheckState(desc.m_bClipMouseCursor ? Qt::Checked : Qt::Unchecked);
  m_ShowMouseCursor->setCheckState(desc.m_bShowMouseCursor ? Qt::Checked : Qt::Unchecked);

  m_CheckOverrideDefault->setCheckState(m_bOverrideProjectDefault[m_uiCurDesc] ? Qt::Checked : Qt::Unchecked);

  UpdateUI();
}

void xiiQtWindowConfigPage::GrabUI(xiiWindowCreationDescription& desc)
{
  desc.m_Title             = m_LineEditTitle->text().toUtf8().data();
  desc.m_iMonitor          = m_ComboMonitor->currentIndex() - 1;
  desc.m_WindowMode        = (xiiWindowMode::Enum)(m_ComboMode->currentIndex());
  desc.m_Resolution.width  = m_SpinResX->value();
  desc.m_Resolution.height = m_SpinResY->value();
  desc.m_bClipMouseCursor  = m_ClipMouseCursor->isChecked();
  desc.m_bShowMouseCursor  = m_ShowMouseCursor->isChecked();
}

void xiiQtWindowConfigPage::UpdateUI()
{
  const bool bEnable = m_uiCurDesc == 0 || m_bOverrideProjectDefault[m_uiCurDesc];

  m_LineEditTitle->setEnabled(bEnable);
  m_ComboMonitor->setEnabled(bEnable);
  m_ComboMode->setEnabled(bEnable);
  m_SpinResX->setEnabled(bEnable);
  m_SpinResY->setEnabled(bEnable);
  m_ClipMouseCursor->setEnabled(bEnable);
  m_ShowMouseCursor->setEnabled(bEnable);
  m_CheckOverrideDefault->setVisible(m_uiCurDesc != 0);
}

void xiiQtWindowConfigPage::LoadDescs()
{
  xiiStringBuilder sPath;

  {
    sPath = xiiToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/Window.ddl");

    // Opening configuration must not create or overwrite a file.
    m_bCanSave[0] = m_Descs[0].LoadFromDDL(sPath).Succeeded() || !xiiOSFile::ExistsFile(sPath);
    if (!m_bCanSave[0])
      xiiLog::Warning("Unable to load window configuration '{}'; preserving the original file.", sPath);

    m_bOverrideProjectDefault[0] = false;
  }

  {
    sPath = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sPath.AppendPath("RuntimeConfigs/Window.ddl");

    m_bOverrideProjectDefault[1] = m_Descs[1].LoadFromDDL(sPath).Succeeded();
    m_bCanSave[1]                = m_bOverrideProjectDefault[1] || !xiiOSFile::ExistsFile(sPath);
    if (!m_bCanSave[1])
      xiiLog::Warning("Unable to load window configuration '{}'; preserving the original file.", sPath);
  }
}

xiiResult xiiQtWindowConfigPage::SaveDescs()
{
  xiiStringBuilder sPath;

  {
    sPath = xiiToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/Window.ddl");

    const auto current = SerializeWindow(m_Descs[0]);
    if (current != m_Committed[0])
    {
      if (!m_bCanSave[0])
        return XII_FAILURE;
      XII_SUCCEED_OR_RETURN(SaveWindow(m_Descs[0], sPath));
      m_Committed[0] = current;
    }
  }

  {
    sPath = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sPath.AppendPath("RuntimeConfigs/Window.ddl");

    const auto current = SerializeWindow(m_Descs[1]);
    if (m_bOverrideProjectDefault[1] == m_bCommittedOverride && (!m_bCommittedOverride || current == m_Committed[1]))
      return XII_SUCCESS;
    if (!m_bCanSave[1])
      return XII_FAILURE;
    if (m_bOverrideProjectDefault[1])
    {
      XII_SUCCEED_OR_RETURN(SaveWindow(m_Descs[1], sPath));
    }
    else
    {
      if (xiiOSFile::ExistsFile(sPath))
        XII_SUCCEED_OR_RETURN(xiiOSFile::DeleteFile(sPath));
    }
    m_Committed[1]       = current;
    m_bCommittedOverride = m_bOverrideProjectDefault[1];
  }
  return XII_SUCCESS;
}

void xiiQtWindowConfigPage::on_m_ComboWnd_currentIndexChanged(int index)
{
  GrabUI(m_Descs[m_uiCurDesc]);
  m_uiCurDesc = index;

  if (!m_bOverrideProjectDefault[m_uiCurDesc])
  {
    m_Descs[m_uiCurDesc] = m_Descs[0];
  }

  FillUI(m_Descs[m_uiCurDesc]);
}

void xiiQtWindowConfigPage::on_m_CheckOverrideDefault_stateChanged(int state)
{
  m_bOverrideProjectDefault[m_uiCurDesc] = m_CheckOverrideDefault->isChecked();

  UpdateUI();
}

xiiResult xiiQtWindowConfigPage::ApplyChanges()
{
  GrabUI(m_Descs[m_uiCurDesc]);
  return SaveDescs();
}
