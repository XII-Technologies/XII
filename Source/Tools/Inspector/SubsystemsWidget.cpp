#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>

xiiQtSubsystemsWidget* xiiQtSubsystemsWidget::s_pWidget = nullptr;

xiiQtSubsystemsWidget::xiiQtSubsystemsWidget(QWidget* parent) :
  ads::CDockWidget("Subsystem Widget", parent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(TableSubsystems);

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

  TableSubsystems->setColumnCount(Headers.size());

  TableSubsystems->setHorizontalHeaderLabels(Headers);

  {
    xiiStringBuilder sTemp;
    xiiInt32         iRow = 0;

    for (xiiMap<xiiString, SubsystemData>::Iterator it = m_Subsystems.GetIterator(); it.IsValid(); ++it)
    {
      const SubsystemData& ssd = it.Value();

      QLabel* pIcon = new QLabel();
      pIcon->setPixmap(xiiQtUiServices::GetCachedPixmapResource(":/Icons/Icons/Subsystem.png"));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableSubsystems->setCellWidget(iRow, 0, pIcon);

      sTemp.Format("  {0}  ", it.Key());
      TableSubsystems->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));

      sTemp.Format("  {0}  ", ssd.m_sPlugin);
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

      sTemp.Format("  {0}  ", ssd.m_sDependencies);
      TableSubsystems->setCellWidget(iRow, 4, new QLabel(sTemp.GetData()));

      ++iRow;
    }
  }

  TableSubsystems->resizeColumnsToContents();
}

void xiiQtSubsystemsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  xiiTelemetryMessage Msg;

  while (xiiTelemetry::RetrieveMessage('STRT', Msg) == XII_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case ' CLR':
      {
        s_pWidget->m_Subsystems.Clear();
        s_pWidget->m_bUpdateSubsystems = true;
      }
      break;

      case 'SYST':
      {
        xiiString sGroup, sSystem;

        Msg.GetReader() >> sGroup;
        Msg.GetReader() >> sSystem;

        xiiStringBuilder sFinalName = sGroup.GetData();
        sFinalName.Append("::");
        sFinalName.Append(sSystem.GetData());

        SubsystemData& ssd = s_pWidget->m_Subsystems[sFinalName.GetData()];

        Msg.GetReader() >> ssd.m_sPlugin;

        for (xiiUInt32 i = 0; i < xiiStartupStage::ENUM_COUNT; ++i)
          Msg.GetReader() >> ssd.m_bStartupDone[i];

        xiiUInt8 uiDependencies = 0;
        Msg.GetReader() >> uiDependencies;

        xiiStringBuilder sAllDeps;

        xiiString sDep;
        for (xiiUInt8 i = 0; i < uiDependencies; ++i)
        {
          Msg.GetReader() >> sDep;

          if (sAllDeps.IsEmpty())
            sAllDeps = sDep.GetData();
          else
          {
            sAllDeps.Append(" | ");
            sAllDeps.Append(sDep.GetData());
          }
        }

        ssd.m_sDependencies = sAllDeps.GetData();

        s_pWidget->m_bUpdateSubsystems = true;
      }
      break;
    }
  }
}
