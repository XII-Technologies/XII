/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qgraphicsitem.h>

xiiQtFileWidget* xiiQtFileWidget::s_pWidget = nullptr;

xiiQtFileWidget::xiiQtFileWidget(ads::CDockManager* pDockManager, QWidget* pParent) :
  ads::CDockWidget(pDockManager, "File Operations", pParent)
{
  s_pWidget = this;

  setIcon(QIcon(":/Icons/Icons/File.svg"));

  setupUi(this);
  setWidget(Frame);

  ResetStats();
}

xiiQtFileWidget::~xiiQtFileWidget() = default;

void xiiQtFileWidget::ResetStats()
{
  m_iMaxID       = 0;
  m_bUpdateTable = true;
  m_FileOps.Clear();
  m_FileOps.Reserve(10000);
  m_LastTableUpdate = xiiTime::MakeFromSeconds(0);

  Table->clear();

  {
    QStringList Headers;
    Headers.append(" # ");
    Headers.append(" Operation ");
    Headers.append(" Duration (ms)");
    Headers.append(" Bytes ");
    Headers.append(" Thread ");
    Headers.append(" File ");

    Table->setColumnCount(static_cast<xiiInt32>(Headers.size()));
    Table->setHorizontalHeaderLabels(Headers);
    Table->horizontalHeader()->show();
  }

  Table->resizeColumnsToContents();
  Table->sortByColumn(0, Qt::DescendingOrder);
}

void xiiQtFileWidget::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (!s_pWidget || sample.m_Records.GetCount() != 1)
    return;
  const auto& row = sample.m_Records[0];
  xiiInt32    id;
  xiiUInt8    operation, mode, threads;
  xiiString   path, destination;
  bool        success;
  xiiUInt64   bytes;
  xiiTime     duration;
  if (!row.Read("id", id) || !row.Read("operation", operation) || !row.Read("mode", mode) || !row.Read("threads", threads) ||
      !row.Read("path", path) || !row.Read("destination", destination) || !row.Read("success", success) || !row.Read("bytes", bytes) ||
      !row.Read("duration", duration) || id <= 0 || duration.IsNegative() || !xiiMath::IsFinite(duration.GetSeconds()) ||
      operation < static_cast<xiiUInt8>(xiiTelemetryFileOperation::Open) || operation > static_cast<xiiUInt8>(xiiTelemetryFileOperation::Casing) ||
      (operation == static_cast<xiiUInt8>(xiiTelemetryFileOperation::Open) && mode != xiiFileOpenMode::Read && mode != xiiFileOpenMode::Write && mode != xiiFileOpenMode::Append))
    return;
  if (s_pWidget->m_FileOps.GetCount() >= 10000 && !s_pWidget->m_FileOps.Contains(id))
  {
    auto oldest = s_pWidget->m_FileOps.GetIterator();
    for (auto it = s_pWidget->m_FileOps.GetIterator(); it.IsValid(); ++it)
      if (it.Key() < oldest.Key())
        oldest = it;
    s_pWidget->m_FileOps.Remove(oldest);
  }
  auto& data                = s_pWidget->m_FileOps[id];
  s_pWidget->m_iMaxID       = xiiMath::Max(s_pWidget->m_iMaxID, id);
  s_pWidget->m_bUpdateTable = true;
  if (data.m_StartTime == xiiTime::MakeZero())
    data.m_StartTime = xiiTime::Now();
  switch (static_cast<xiiTelemetryFileOperation>(operation))
  {
    case xiiTelemetryFileOperation::Open:
      data.m_sFile = path;
      if (mode == xiiFileOpenMode::Read)
        data.m_State = success ? OpenReading : OpenReadingFailed;
      else if (mode == xiiFileOpenMode::Write || mode == xiiFileOpenMode::Append)
        data.m_State = success ? OpenWriting : OpenWritingFailed;
      break;
    case xiiTelemetryFileOperation::Close:
      if (data.m_State == OpenReading)
        data.m_State = ClosedReading;
      else if (data.m_State == OpenWriting)
        data.m_State = ClosedWriting;
      break;
    case xiiTelemetryFileOperation::Read:
      data.m_uiBytesAccessed += bytes;
      if (data.m_State == None)
        data.m_State = OpenReading;
      break;
    case xiiTelemetryFileOperation::Write:
      data.m_uiBytesAccessed += bytes;
      if (data.m_State == None)
        data.m_State = OpenWriting;
      if (!success)
        data.m_State = OpenWritingFailed;
      break;
    case xiiTelemetryFileOperation::Exists:
      data.m_sFile = path;
      data.m_State = success ? FileExists : FileExistsFailed;
      break;
    case xiiTelemetryFileOperation::Delete:
      data.m_sFile = path;
      data.m_State = success ? FileDelete : FileDeleteFailed;
      break;
    case xiiTelemetryFileOperation::CreateDirectory:
      data.m_sFile = path;
      data.m_State = success ? CreateDirs : CreateDirsFailed;
      break;
    case xiiTelemetryFileOperation::Copy:
    {
      xiiStringBuilder text;
      text.SetFormat("'{}' -> '{}'", path, destination);
      data.m_sFile = text;
      data.m_State = success ? FileCopy : FileCopyFailed;
      break;
    }
    case xiiTelemetryFileOperation::Stat:
      data.m_sFile = path;
      data.m_State = success ? FileStat : FileStatFailed;
      break;
    case xiiTelemetryFileOperation::Casing:
      data.m_sFile = path;
      data.m_State = success ? FileCasing : FileCasingFailed;
      break;
    default:
      return;
  }
  data.m_BlockedDuration += duration;
  data.m_uiThreadTypes |= threads & 7;
}


QTableWidgetItem* xiiQtFileWidget::GetStateString(FileOpState State) const
{
  QTableWidgetItem* pItem = new QTableWidgetItem();
  pItem->setTextAlignment(Qt::AlignCenter);

  switch (State)
  {
    case None:
      pItem->setText("Unknown");
      pItem->setForeground(Qt::red);
      break;
    case ClosedReading:
      pItem->setText("Read");
      pItem->setForeground(QColor::fromRgb(110, 60, 185));
      break;
    case ClosedWriting:
      pItem->setText("Write");
      pItem->setForeground(QColor::fromRgb(255, 140, 0));
      break;
    case CreateDirs:
      pItem->setText("MakeDir");
      pItem->setForeground(Qt::darkYellow);
      break;
    case CreateDirsFailed:
      pItem->setText("MakeDir (fail)");
      pItem->setForeground(Qt::red);
      break;
    case FileCopy:
      pItem->setText("Copy");
      pItem->setForeground(QColor::fromRgb(255, 0, 255));
      break;
    case FileCopyFailed:
      pItem->setText("Copy (fail)");
      pItem->setForeground(Qt::red);
      break;
    case FileDelete:
      pItem->setText("Delete");
      pItem->setForeground(Qt::darkYellow);
      break;
    case FileDeleteFailed:
      pItem->setText("Delete (fail)");
      pItem->setForeground(Qt::red);
      break;
    case FileExists:
      pItem->setText("Exists");
      pItem->setForeground(Qt::lightGray);
      break;
    case FileExistsFailed:
      pItem->setText("Exists (not)");
      pItem->setForeground(Qt::red);
      break;
    case OpenReading:
      pItem->setText("Read (Open)");
      pItem->setForeground(QColor::fromRgb(160, 90, 255));
      break;
    case OpenReadingFailed:
      pItem->setText("Read (fail)");
      pItem->setForeground(Qt::red);
      break;
    case OpenWriting:
      pItem->setText("Write (Open)");
      pItem->setForeground(QColor::fromRgb(255, 64, 0));
      break;
    case OpenWritingFailed:
      pItem->setText("Write (fail)");
      pItem->setForeground(Qt::red);
      break;
    case FileStat:
      pItem->setText("Stat");
      pItem->setForeground(QColor::fromRgb(128, 128, 128));
      break;
    case FileStatFailed:
      pItem->setText("Stat (fail)");
      pItem->setForeground(Qt::red);
      break;
    case FileCasing:
      pItem->setText("Casing");
      pItem->setForeground(Qt::cyan);
      break;
    case FileCasingFailed:
      pItem->setText("Casing (fail)");
      pItem->setForeground(Qt::red);
      break;
    default:
      XII_REPORT_FAILURE("Unknown File Operation {0}", (xiiInt32)State);
      break;
  }

  return pItem;
}

void xiiQtFileWidget::UpdateTable()
{
  if (!m_bUpdateTable)
    return;

  if (xiiTime::Now() - m_LastTableUpdate < xiiTime::MakeFromSeconds(0.3))
    return;

  m_LastTableUpdate = xiiTime::Now();

  m_bUpdateTable = false;

  xiiQtScopedUpdatesDisabled _1(Table);

  Table->setSortingEnabled(false);
  Table->clear();

  {
    QStringList Headers;
    Headers.append(" # ");
    Headers.append(" Operation ");
    Headers.append(" Duration (ms) ");
    Headers.append(" Bytes ");
    Headers.append(" Thread ");
    Headers.append(" File ");

    Table->setColumnCount(static_cast<xiiInt32>(Headers.size()));
    Table->setHorizontalHeaderLabels(Headers);
    Table->horizontalHeader()->show();
  }

  const double    fMinDuration   = SpinMinDuration->value();
  const xiiUInt32 uiMMaxElements = SpinLimitToRecent->value();
  xiiString       sFilter        = LineFilterByName->text().toUtf8().data();

  const xiiUInt32 iThread        = ComboThread->currentIndex();
  const xiiUInt8  uiThreadFilter = (iThread == 0) ? 0xFF : (1 << (iThread - 1));

  xiiUInt32 uiRow = 0;
  for (xiiHashTable<xiiUInt32, FileOpData>::Iterator it = m_FileOps.GetIterator(); it.IsValid(); ++it)
  {
    if ((uiThreadFilter & it.Value().m_uiThreadTypes) == 0)
      continue;

    if (it.Value().m_BlockedDuration.GetSeconds() < fMinDuration)
      continue;

    if ((uiMMaxElements > 0) && (m_iMaxID - it.Key() > uiMMaxElements))
      continue;

    if (!sFilter.IsEmpty() && (it.Value().m_sFile.FindSubString_NoCase(sFilter.GetData()) == nullptr))
      continue;

    if (uiRow >= (xiiUInt32)Table->rowCount())
      Table->insertRow(Table->rowCount());

    QTableWidgetItem* pItem;

    pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, QVariant((xiiUInt64)it.Value().m_StartTime.GetMicroseconds()));
    Table->setItem(uiRow, 0, pItem);

    pItem = GetStateString(it.Value().m_State);
    Table->setItem(uiRow, 1, pItem);

    pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, QVariant(it.Value().m_BlockedDuration.GetSeconds() * 1000.0));
    Table->setItem(uiRow, 2, pItem);

    pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, QVariant(it.Value().m_uiBytesAccessed));
    Table->setItem(uiRow, 3, pItem);

    pItem = new QTableWidgetItem();
    pItem->setTextAlignment(Qt::AlignCenter);

    xiiStringBuilder sThread;

    if ((it.Value().m_uiThreadTypes & (1 << 0)) != 0) // Main Thread
      pItem->setForeground(QColor::fromRgb(255, 64, 0));
    else if ((it.Value().m_uiThreadTypes & (1 << 2)) != 0) // Other Thread
      pItem->setForeground(QColor::fromRgb(160, 90, 255));
    else // Task Loading Thread
      pItem->setForeground(QColor::fromRgb(0, 255, 0));

    if ((it.Value().m_uiThreadTypes & (1 << 0)) != 0) // Main Thread
      sThread.Append(" Main ");
    if ((it.Value().m_uiThreadTypes & (1 << 1)) != 0) // Loading Thread
      sThread.Append(" Loading ");
    if ((it.Value().m_uiThreadTypes & (1 << 2)) != 0) // Other Thread
      sThread.Append(" Other ");

    pItem->setData(Qt::DisplayRole, QVariant(sThread.GetData()));
    Table->setItem(uiRow, 4, pItem);

    pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, QVariant(it.Value().m_sFile.GetData()));
    Table->setItem(uiRow, 5, pItem);

    ++uiRow;
  }

  Table->setRowCount(uiRow);
  Table->setSortingEnabled(true);
}

void xiiQtFileWidget::UpdateStats()
{
  if (!m_bUpdateTable)
    return;

  UpdateTable();
}

void xiiQtFileWidget::on_SpinLimitToRecent_valueChanged(int val)
{
  m_bUpdateTable = true;
}

void xiiQtFileWidget::on_SpinMinDuration_valueChanged(double val)
{
  m_bUpdateTable = true;
}

void xiiQtFileWidget::on_LineFilterByName_textChanged()
{
  m_bUpdateTable = true;
}

void xiiQtFileWidget::on_ComboThread_currentIndexChanged(int state)
{
  m_bUpdateTable = true;
}
