#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <QDir>
#include <QMenu>
#include <QSettings>
#include <QStandardPaths>

xiiQtMainWidget* xiiQtMainWidget::s_pWidget = nullptr;

xiiQtMainWidget::xiiQtMainWidget(ads::CDockManager* pDockManager, QWidget* pParent) :
  ads::CDockWidget(pDockManager, "Main", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(MainWidgetFrame);

  this->setFeature(ads::CDockWidget::DockWidgetClosable, false);

  m_uiMaxStatSamples = 20000; // should be enough for 5 minutes of history at 60 Hz

  setContextMenuPolicy(Qt::NoContextMenu);

  TreeStats->setContextMenuPolicy(Qt::CustomContextMenu);

  ResetStats();

  LoadFavorites();

  QSettings Settings;
  Settings.beginGroup("MainWidget");

  splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
  splitter->restoreGeometry(Settings.value("SplitterSize", splitter->saveGeometry()).toByteArray());

  Settings.endGroup();
}

xiiQtMainWidget::~xiiQtMainWidget()
{
  SaveFavorites();
}

void xiiQtMainWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  xiiTelemetryMessage Msg;

  while (xiiTelemetry::RetrieveMessage('STAT', Msg) == XII_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case ' DEL':
      {
        xiiString sStatName;
        Msg.GetReader() >> sStatName;

        xiiMap<xiiString, StatData>::Iterator it = s_pWidget->m_Stats.Find(sStatName);

        if (!it.IsValid())
          break;

        if (it.Value().m_pItem)
          delete it.Value().m_pItem;

        if (it.Value().m_pItemFavorite)
          delete it.Value().m_pItemFavorite;

        s_pWidget->m_Stats.Remove(it);
      }
      break;

      case ' SET':
      {
        xiiString sStatName;
        Msg.GetReader() >> sStatName;

        StatData& sd = s_pWidget->m_Stats[sStatName];

        Msg.GetReader() >> sd.m_Value;

        StatSample ss;
        ss.m_Value = sd.m_Value.ConvertTo<double>();
        Msg.GetReader() >> ss.m_AtGlobalTime;

        sd.m_History.PushBack(ss);

        s_pWidget->m_MaxGlobalTime = xiiMath::Max(s_pWidget->m_MaxGlobalTime, ss.m_AtGlobalTime);

        // remove excess samples
        if (sd.m_History.GetCount() > s_pWidget->m_uiMaxStatSamples)
          sd.m_History.PopFront(sd.m_History.GetCount() - s_pWidget->m_uiMaxStatSamples);

        if (sd.m_pItem == nullptr)
        {
          sd.m_pItem = s_pWidget->CreateStat(sStatName.GetData(), false);

          if (s_pWidget->m_Favorites.Find(sStatName).IsValid())
            sd.m_pItem->setCheckState(0, Qt::Checked);
        }

        const xiiString sValue = sd.m_Value.ConvertTo<xiiString>();
        sd.m_pItem->setData(1, Qt::DisplayRole, sValue.GetData());

        if (sd.m_pItemFavorite)
          sd.m_pItemFavorite->setData(1, Qt::DisplayRole, sValue.GetData());
      }
      break;
    }
  }
}

void xiiQtMainWidget::on_ButtonConnect_clicked()
{
  QSettings     Settings;
  const QString sServer = Settings.value("LastConnection", QLatin1String("localhost:1040")).toString();

  bool    bOk  = false;
  QString sRes = QInputDialog::getText(this, "Host", "Host Name or IP Address:\nDefault is 'localhost:1040'", QLineEdit::Normal, sServer, &bOk);

  if (!bOk)
    return;

  Settings.setValue("LastConnection", sRes);

  if (xiiTelemetry::ConnectToServer(sRes.toUtf8().data()) == XII_SUCCESS)
  {
  }
}

void xiiQtMainWidget::SaveFavorites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir    dir;
  dir.mkpath(sFile);

  sFile.append("/Favourites.stats");

  QFile f(sFile);
  if (!f.open(QIODevice::WriteOnly))
    return;

  QDataStream stream(&f);

  const xiiUInt32 uiNumFavorites = m_Favorites.GetCount();
  stream << uiNumFavorites;

  for (xiiSet<xiiString>::Iterator it = m_Favorites.GetIterator(); it.IsValid(); ++it)
  {
    const QString s = it.Key().GetData();
    stream << s;
  }

  f.close();
}

void xiiQtMainWidget::LoadFavorites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir    dir;
  dir.mkpath(sFile);
  sFile.append("/Favourites.stats");

  QFile f(sFile);
  if (!f.open(QIODevice::ReadOnly))
    return;

  m_Favorites.Clear();

  QDataStream stream(&f);

  xiiUInt32 uiNumFavorites = 0;
  stream >> uiNumFavorites;

  for (xiiUInt32 i = 0; i < uiNumFavorites; ++i)
  {
    QString s;
    stream >> s;

    xiiString xiis = s.toUtf8().data();

    m_Favorites.Insert(xiis);
  }

  f.close();
}

void xiiQtMainWidget::ResetStats()
{
  m_Stats.Clear();
  TreeStats->clear();
  TreeFavorites->clear();
}

void xiiQtMainWidget::UpdateStats()
{
  static bool bWasConnected = false;
  const bool  bIsConnected  = xiiTelemetry::IsConnectedToServer();

  if (bIsConnected)
    LabelPing->setText(QString::fromUtf8("<p>Ping: %1ms</p>").arg((xiiUInt32)xiiTelemetry::GetPingToServer().GetMilliseconds()));

  if (bWasConnected == bIsConnected)
    return;

  bWasConnected = bIsConnected;

  if (!bIsConnected)
  {
    LabelPing->setText("<p>Ping: N/A</p>");
    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#ff0000;\">Not Connected</span></p>");
    LabelServer->setText("<p>Server: N/A</p>");
  }
  else
  {
    xiiStringBuilder tmp;

    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#00aa00;\">Connected</span></p>");
    LabelServer->setText(QString::fromUtf8("<p>Server: %1:%2</p>").arg(xiiTelemetry::GetServerIP().GetData(tmp)).arg(xiiTelemetry::s_uiPort));
  }
}


void xiiQtMainWidget::closeEvent(QCloseEvent* pEvent)
{
  QSettings Settings;

  Settings.beginGroup("MainWidget");

  Settings.setValue("SplitterState", splitter->saveState());
  Settings.setValue("SplitterGeometry", splitter->saveGeometry());

  Settings.endGroup();
}

QTreeWidgetItem* xiiQtMainWidget::CreateStat(xiiStringView sPath, bool bParent)
{
  xiiStringBuilder sCleanPath = sPath;
  if (sCleanPath.EndsWith("/"))
    sCleanPath.Shrink(0, 1);

  xiiMap<xiiString, StatData>::Iterator it = m_Stats.Find(sCleanPath.GetData());

  if (it.IsValid() && it.Value().m_pItem != nullptr)
    return it.Value().m_pItem;

  QTreeWidgetItem* pParent = nullptr;
  StatData&        sd      = m_Stats[sCleanPath.GetData()];

  {
    xiiStringBuilder sParentPath = sCleanPath.GetData();
    sParentPath.PathParentDirectory(1);

    sd.m_pItem = new QTreeWidgetItem();
    sd.m_pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | (bParent ? Qt::NoItemFlags : Qt::ItemIsUserCheckable));
    sd.m_pItem->setData(0, Qt::UserRole, QString(sCleanPath.GetData()));

    if (bParent)
      sd.m_pItem->setIcon(0, QIcon(":/Icons/Icons/StatGroup.svg"));
    else
      sd.m_pItem->setIcon(0, QIcon(":/Icons/Icons/Stat.svg"));

    if (!bParent)
      sd.m_pItem->setCheckState(0, Qt::Unchecked);

    if (!sParentPath.IsEmpty())
    {
      pParent = CreateStat(sParentPath.GetData(), true);
      pParent->addChild(sd.m_pItem);
      pParent->setExpanded(false);
    }
    else
    {
      TreeStats->addTopLevelItem(sd.m_pItem);
    }
  }

  {
    xiiString sFileName = sCleanPath.GetFileName();
    sd.m_pItem->setData(0, Qt::DisplayRole, sFileName.GetData());

    if (pParent)
      pParent->sortChildren(0, Qt::AscendingOrder);
    else
      TreeStats->sortByColumn(0, Qt::AscendingOrder);

    TreeStats->resizeColumnToContents(0);
  }

  return sd.m_pItem;
}

void xiiQtMainWidget::SetFavorite(const xiiString& sStat, bool bFavorite)
{
  StatData& sd = m_Stats[sStat];

  if (bFavorite)
  {
    m_Favorites.Insert(sStat);

    if (!sd.m_pItemFavorite)
    {
      sd.m_pItemFavorite = new QTreeWidgetItem();
      TreeFavorites->addTopLevelItem(sd.m_pItemFavorite);
      sd.m_pItemFavorite->setData(0, Qt::DisplayRole, sStat.GetData());
      sd.m_pItemFavorite->setData(1, Qt::DisplayRole, sd.m_Value.ConvertTo<xiiString>().GetData());
      sd.m_pItemFavorite->setIcon(0, QIcon(":/Icons/Icons/StatFavorite.svg"));

      TreeFavorites->resizeColumnToContents(0);
    }
  }
  else
  {
    if (sd.m_pItemFavorite)
    {
      m_Favorites.Remove(sStat);

      delete sd.m_pItemFavorite;
      sd.m_pItemFavorite = nullptr;
    }
  }
}

void xiiQtMainWidget::on_TreeStats_itemChanged(QTreeWidgetItem* item, int column)
{
  if (column == 0)
  {
    xiiString sPath = item->data(0, Qt::UserRole).toString().toUtf8().data();

    SetFavorite(sPath, (item->checkState(0) == Qt::Checked));
  }
}

void xiiQtMainWidget::on_TreeStats_customContextMenuRequested(const QPoint& p)
{
  if (!TreeStats->currentItem())
    return;

  QMenu mSub;
  mSub.setTitle("Show in");
  mSub.setIcon(QIcon(":/Icons/Icons/StatHistory.svg"));

  QMenu m;
  m.addMenu(&mSub);

  for (xiiInt32 i = 0; i < 10; ++i)
  {
    xiiQtMainWindow::s_pWidget->m_pActionShowStatIn[i]->setText(xiiQtMainWindow::s_pWidget->m_pStatHistoryWidgets[i]->LineName->text());
    mSub.addAction(xiiQtMainWindow::s_pWidget->m_pActionShowStatIn[i]);
  }

  if (TreeStats->currentItem()->childCount() > 0)
    mSub.setEnabled(false);

  m.exec(TreeStats->viewport()->mapToGlobal(p));
}


void xiiQtMainWidget::ShowStatIn(bool)
{
  if (!TreeStats->currentItem())
    return;

  QAction* pAction = (QAction*)sender();

  xiiInt32 iHistoryWidget = 0;
  for (iHistoryWidget = 0; iHistoryWidget < 10; ++iHistoryWidget)
  {
    if (xiiQtMainWindow::s_pWidget->m_pActionShowStatIn[iHistoryWidget] == pAction)
      goto found;
  }

  return;

found:

  xiiString sPath = TreeStats->currentItem()->data(0, Qt::UserRole).toString().toUtf8().data();

  xiiQtMainWindow::s_pWidget->m_pStatHistoryWidgets[iHistoryWidget]->AddStat(sPath);
}
