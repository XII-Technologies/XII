/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Diagnostics/TelemetryDiscovery.h>
#include <Foundation/Diagnostics/TelemetryProtocol.h>
#include <Foundation/Diagnostics/TelemetryService.h>
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

void xiiQtMainWidget::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (s_pWidget == nullptr)
    return;

  xiiMap<xiiString, xiiVariant> values;
  for (const auto& row : sample.m_Records)
  {
    xiiString   name;
    const auto* value = row.m_Fields.GetValue("value");
    if (row.Read("name", name) && value && name.GetElementCount() <= 512)
      values.Insert(name, *value);
  }
  for (auto it = sample.m_Fields.GetIterator(); it.IsValid(); ++it)
    values.Insert(it.Key(), it.Value());
  const auto        historyLimit = xiiMath::Min(s_pWidget->m_uiMaxStatSamples,
                                                static_cast<xiiUInt32>(64 * 1024 * 1024 / (xiiMath::Max(values.GetCount(), 1U) * sizeof(StatSample))));
  xiiSet<xiiString> retained;
  for (auto it = values.GetIterator(); it.IsValid(); ++it)
  {
    xiiStringBuilder path(it.Key());
    retained.Insert(path);
    for (xiiUInt32 level = 0; level < 64; ++level)
    {
      path.PathParentDirectory();
      path.Trim("/");
      if (path.IsEmpty())
        break;
      retained.Insert(path);
    }
  }
  xiiDynamicArray<xiiString> removed;
  for (auto it = s_pWidget->m_Stats.GetIterator(); it.IsValid(); ++it)
    if (!retained.Contains(it.Key()))
      removed.PushBack(it.Key());
  removed.Sort([](const xiiString& lhs, const xiiString& rhs) { return lhs.GetElementCount() > rhs.GetElementCount(); });
  for (const auto& name : removed)
  {
    auto it = s_pWidget->m_Stats.Find(name);
    delete it.Value().m_pItem;
    delete it.Value().m_pItemFavorite;
    s_pWidget->m_Stats.Remove(it);
  }
  for (auto it = values.GetIterator(); it.IsValid(); ++it)
  {
    auto& stat   = s_pWidget->m_Stats[it.Key()];
    stat.m_Value = it.Value();
    StatSample point;
    point.m_Value        = stat.m_Value.CanConvertTo<double>() ? stat.m_Value.ConvertTo<double>() : 0;
    point.m_AtGlobalTime = xiiTime::MakeFromMicroseconds(sample.m_fTimestampMicroseconds);
    stat.m_History.PushBack(point);
    if (stat.m_History.GetCount() > historyLimit)
      stat.m_History.PopFront(stat.m_History.GetCount() - historyLimit);
    s_pWidget->m_MaxGlobalTime = xiiMath::Max(s_pWidget->m_MaxGlobalTime, point.m_AtGlobalTime);
    if (stat.m_pItem == nullptr)
    {
      stat.m_pItem = s_pWidget->CreateStat(it.Key().GetData(), false);
      if (s_pWidget->m_Favorites.Contains(it.Key()))
        stat.m_pItem->setCheckState(0, Qt::Checked);
    }
    const auto value = stat.m_Value.ConvertTo<xiiString>();
    stat.m_pItem->setData(1, Qt::DisplayRole, value.GetData());
    if (stat.m_pItemFavorite != nullptr)
      stat.m_pItemFavorite->setData(1, Qt::DisplayRole, value.GetData());
  }
}



void xiiQtMainWidget::on_ButtonConnect_clicked()
{
  QSettings     Settings;
  const QString sServer = Settings.value("LastConnection", QLatin1String("local:")).toString();

  xiiDynamicArray<xiiTelemetryInstance> instances;
  xiiTelemetryDiscovery::Enumerate(instances).IgnoreResult();
  QStringList choices;
  for (const auto& instance : instances)
    choices.push_back(QString("%1 (process %2) — local:%3").arg(QString::fromUtf8(instance.m_sApplication.GetData())).arg(instance.m_uiProcessId).arg(QString::fromUtf8(instance.m_sEndpoint.GetData())));
  choices.push_back("Enter endpoint…");
  bool       bOk      = false;
  const auto selected = QInputDialog::getItem(this, "Diagnostics applications", "Available applications", choices, 0, false, &bOk);
  if (!bOk)
    return;
  const auto index = choices.indexOf(selected);
  if (index >= 0 && index < static_cast<int>(instances.GetCount()))
  {
    const auto&      instance = instances[index];
    xiiStringBuilder address("local:", instance.m_sEndpoint);
    if (xiiQtMainWindow::s_pWidget->ConnectDiagnostics(address, instance.m_Instance).Failed())
      xiiLog::Error("Could not connect to the selected application.");
    return;
  }
  const QString sRes = QInputDialog::getText(this, "Host", "Diagnostics endpoint (local:<name>, tls://IP:port, or quic://IP:port)", QLineEdit::Normal, sServer, &bOk);
  if (!bOk)
    return;
  Settings.setValue("LastConnection", sRes);

  if (xiiQtMainWindow::s_pWidget->ConnectDiagnostics(sRes.toUtf8().constData()).Failed())
    xiiLog::Error("Could not start the diagnostics connection.");
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
  const auto& client = xiiQtMainWindow::s_pWidget->GetDiagnosticsClient();
  if (!client.IsReady())
  {
    LabelPing->setText("Ping: N/A");
    LabelStatus->setText("Disconnected");
    LabelServer->setText("Application: N/A");
    return;
  }
  xiiNetworkConnectionInfo connection;
  if (client.GetConnectionInfo(connection).Succeeded())
    LabelPing->setText(QString("Ping: %1 ms").arg(connection.m_Statistics.m_HeartbeatRoundTrip.GetMilliseconds(), 0, 'f', 2));
  LabelStatus->setText("Connected");
  const auto& info = client.GetServerInfo();
  LabelServer->setText(QString("%1 (process %2)").arg(QString::fromUtf8(info.m_sApplication.GetData())).arg(info.m_uiProcessId).toHtmlEscaped());
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
