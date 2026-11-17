/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Diagnostics/TelemetryService.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>

xiiQtSubsystemsWidget* xiiQtSubsystemsWidget::s_pWidget = nullptr;

xiiQtSubsystemsWidget::xiiQtSubsystemsWidget(ads::CDockManager* pDockManager, QWidget* pParent) :
  ads::CDockWidget(pDockManager, "Subsystem Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(TableSubsystems);

  setIcon(QIcon(":/Icons/Icons/Subsystem.svg"));

  ResetStats();
}

void xiiQtSubsystemsWidget::ResetStats()
{
  m_bUpdateSubsystems = true;
  m_Subsystems.Clear();
}

void xiiQtSubsystemsWidget::UpdateStats()
{
  UpdateSubSystems();
}

void xiiQtSubsystemsWidget::UpdateSubSystems()
{
  if (!m_bUpdateSubsystems)
    return;

  m_bUpdateSubsystems = false;

  xiiQtScopedUpdatesDisabled _1(TableSubsystems);

  TableSubsystems->clear();

  TableSubsystems->setRowCount(m_Subsystems.GetCount());

  QStringList Headers;
  Headers.append("");
  Headers.append(" SubSystem ");
  Headers.append(" Plugin ");
  Headers.append(" Startup Done ");
  Headers.append(" Dependencies ");

  TableSubsystems->setColumnCount(static_cast<xiiInt32>(Headers.size()));

  TableSubsystems->setHorizontalHeaderLabels(Headers);

  {
    xiiStringBuilder sTemp;
    xiiInt32         iRow = 0;

    for (xiiMap<xiiString, SubsystemData>::Iterator it = m_Subsystems.GetIterator(); it.IsValid(); ++it)
    {
      const SubsystemData& ssd = it.Value();

      QLabel* pIcon = new QLabel();
      QIcon   icon  = xiiQtUiServices::GetCachedIconResource(":/Icons/Icons/Subsystem.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableSubsystems->setCellWidget(iRow, 0, pIcon);

      sTemp.SetFormat("  {0}  ", it.Key());
      TableSubsystems->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));

      sTemp.SetFormat("  {0}  ", ssd.m_sPlugin);
      TableSubsystems->setCellWidget(iRow, 2, new QLabel(sTemp.GetData()));

      if (ssd.m_bStartupDone[xiiStartupStage::HighLevelSystems])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#00aa00;\">  Engine  </span></p>"));
      else if (ssd.m_bStartupDone[xiiStartupStage::CoreSystems])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#5555ff;\">  Core  </span></p>"));
      else if (ssd.m_bStartupDone[xiiStartupStage::BaseSystems])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#cece00;\">  Base  </span></p>"));
      else
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#ff0000;\">Not Initialized</span></p>"));

      ((QLabel*)TableSubsystems->cellWidget(iRow, 3))->setAlignment(Qt::AlignHCenter);

      sTemp.SetFormat("  {0}  ", ssd.m_sDependencies);
      TableSubsystems->setCellWidget(iRow, 4, new QLabel(sTemp.GetData()));

      ++iRow;
    }
  }

  TableSubsystems->resizeColumnsToContents();
}

void xiiQtSubsystemsWidget::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (!s_pWidget)
    return;
  s_pWidget->m_Subsystems.Clear();
  for (const auto& row : sample.m_Records)
  {
    xiiString     name, group;
    xiiUInt32     stages;
    SubsystemData data;
    if (!row.Read("name", name) || !row.Read("group", group) || !row.Read("plugin", data.m_sPlugin) || !row.Read("stages", stages) || !row.Read("dependencies", data.m_sDependencies))
      continue;
    for (xiiUInt32 i = 0; i < xiiStartupStage::ENUM_COUNT; ++i)
      data.m_bStartupDone[i] = (stages & (1U << i)) != 0;
    xiiStringBuilder key(group, "::", name);
    s_pWidget->m_Subsystems.Insert(key, data);
  }
  s_pWidget->m_bUpdateSubsystems = true;
}
