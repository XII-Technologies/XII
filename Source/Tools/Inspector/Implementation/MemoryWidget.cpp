/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Diagnostics/TelemetryService.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <QGraphicsPathItem>
#include <QGraphicsView>
#include <QMenu>

xiiQtMemoryWidget* xiiQtMemoryWidget::s_pWidget = nullptr;

namespace MemoryWidgetDetail
{
  static QColor s_Colors[xiiQtMemoryWidget::s_uiMaxColors] = {
    QColor(255, 106, 0),   // orange
    QColor(182, 255, 0),   // lime green
    QColor(255, 0, 255),   // pink
    QColor(0, 148, 255),   // light blue
    QColor(255, 0, 0),     // red
    QColor(0, 255, 255),   // turquoise
    QColor(217, 128, 255), // purple
    QColor(128, 147, 255), // dark blue
    QColor(164, 128, 255), // lilac
  };
}

void FormatSize(xiiStringBuilder& s, xiiStringView sPrefix, xiiUInt64 uiSize)
{
  if (uiSize < 1024)
    s.SetFormat("{0}{1} Bytes", sPrefix, uiSize);
  else if (uiSize < 1024 * 1024)
    s.SetFormat("{0}{1} KB", sPrefix, xiiArgF(uiSize / 1024.0, 1));
  else if (uiSize < 1024 * 1024 * 1024)
    s.SetFormat("{0}{1} MB", sPrefix, xiiArgF(uiSize / 1024.0 / 1024.0, 2));
  else
    s.SetFormat("{0}{1} GB", sPrefix, xiiArgF(uiSize / 1024.0 / 1024.0 / 1024.0, 2));
}

xiiQtMemoryWidget::xiiQtMemoryWidget(ads::CDockManager* pDockManager, QWidget* pParent) :
  ads::CDockWidget(pDockManager, "Memory Widget", pParent)
{
  s_pWidget = this;

  setIcon(QIcon(":/Icons/Icons/Memory.svg"));

  setupUi(this);
  setWidget(MemoryWidgetFrame);

  {
    xiiQtScopedUpdatesDisabled _1(ComboTimeframe);

    ComboTimeframe->addItem("Timeframe: 10 seconds");
    ComboTimeframe->addItem("Timeframe: 30 seconds");
    ComboTimeframe->addItem("Timeframe: 1 minute");
    ComboTimeframe->addItem("Timeframe: 2 minutes");
    ComboTimeframe->addItem("Timeframe: 5 minutes");
    ComboTimeframe->addItem("Timeframe: 10 minutes");
    ComboTimeframe->setCurrentIndex(2);
  }

  m_pPathMax = m_Scene.addPath(QPainterPath(), QPen(QBrush(QColor(255, 255, 255)), 0));

  for (xiiUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i] = m_Scene.addPath(QPainterPath(), QPen(QBrush(MemoryWidgetDetail::s_Colors[i]), 0));

  QTransform t = UsedMemoryView->transform();
  t.scale(1, -1);
  UsedMemoryView->setTransform(t);

  UsedMemoryView->setScene(&m_Scene);

  UsedMemoryView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  UsedMemoryView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  UsedMemoryView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  // UsedMemoryView->setMaximumHeight(100);

  ListAllocators->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ListAllocators, &QTreeView::customContextMenuRequested, this, &xiiQtMemoryWidget::CustomContextMenuRequested);

  ResetStats();
}

void xiiQtMemoryWidget::ResetStats()
{
  m_AllocatorData.Clear();

  m_uiMaxSamples       = 3000;
  m_uiDisplaySamples   = 5 * 60; // 5 samples per second, 60 seconds
  m_uiColorsUsed       = 1;
  m_bAllocatorsChanged = true;

  m_Accu = AllocatorData();

  ListAllocators->clear();
}

void xiiQtMemoryWidget::UpdateStats()
{
  if (!isVisible())
    return;

  if (!xiiQtMainWindow::s_pWidget->GetDiagnosticsClient().IsReady())
  {
    ListAllocators->setEnabled(false);

    LabelNumAllocs->setText(QString::fromUtf8("Alloc Counter: N/A"));
    LabelNumLiveAllocs->setText(QString::fromUtf8("Live Allocs: N/A"));

    LabelCurMemory->setText(QString::fromUtf8("Cur: N/A"));
    LabelMaxMemory->setText(QString::fromUtf8("Max: N/A"));
    LabelMinMemory->setText(QString::fromUtf8("Min: N/A"));

    return;
  }

  ListAllocators->setEnabled(true);

  if (m_bAllocatorsChanged)
  {
    m_bAllocatorsChanged = false;

    xiiQtScopedUpdatesDisabled _1(ListAllocators);
    xiiQtScopedBlockSignals    _2(ListAllocators);

    ListAllocators->clear();

    {
      m_Accu.m_pTreeItem = new QTreeWidgetItem();
      m_Accu.m_pTreeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      m_Accu.m_pTreeItem->setCheckState(0, m_Accu.m_bDisplay ? Qt::Checked : Qt::Unchecked);
      m_Accu.m_pTreeItem->setData(0, Qt::UserRole, xiiUInt32(xiiInvalidIndex));

      m_Accu.m_pTreeItem->setText(0, "<Accumulated>");
      m_Accu.m_pTreeItem->setForeground(0, QColor(255, 255, 255));

      ListAllocators->addTopLevelItem(m_Accu.m_pTreeItem);
    }

    for (auto it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      auto             pParentData = m_AllocatorData.Find(it.Value().m_uiParentId);
      QTreeWidgetItem* pParent     = pParentData.IsValid() ? pParentData.Value().m_pTreeItem : m_Accu.m_pTreeItem;

      QTreeWidgetItem* pItem = new QTreeWidgetItem(pParent);
      pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      pItem->setCheckState(0, it.Value().m_bDisplay ? Qt::Checked : Qt::Unchecked);
      pItem->setData(0, Qt::UserRole, it.Key());

      pItem->setText(0, it.Value().m_sName.GetData());
      pItem->setForeground(0, MemoryWidgetDetail::s_Colors[it.Value().m_uiColor % s_uiMaxColors]);

      it.Value().m_pTreeItem = pItem;
    }

    ListAllocators->expandAll();
  }

  // once a second update the display of the allocators in the list
  if (xiiTime::Now() - m_LastUpdatedAllocatorList > xiiTime::MakeFromSeconds(1))
  {
    m_LastUpdatedAllocatorList = xiiTime::Now();

    for (auto it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      if (!it.Value().m_pTreeItem || it.Value().m_UsedMemory.IsEmpty())
        continue;

      xiiStringBuilder sSize;
      FormatSize(sSize, "", it.Value().m_UsedMemory.PeekBack());

      xiiStringBuilder sMaxSize;
      FormatSize(sMaxSize, "", it.Value().m_uiMaxUsedMemory);

      xiiStringBuilder sText = it.Value().m_sName;
      sText.AppendFormat(" [{0}]", sSize);

      xiiStringBuilder sTooltip;
      sTooltip.SetFormat("<p>Allocator: <b>{}</b><br>Current Memory Used: <b>{}</b><br>Max Memory Used: <b>{}</b><br>Live Allocations: <b>{}</b><br>Allocations: <b>{}</b><br>Deallocations: <b>{}</b><br>", it.Value().m_sName, sSize.GetData(), sMaxSize.GetData(), it.Value().m_uiLiveAllocs, it.Value().m_uiAllocs, it.Value().m_uiDeallocs);

      it.Value().m_pTreeItem->setText(0, sText.GetData());
      it.Value().m_pTreeItem->setToolTip(0, sTooltip.GetData());
    }

    if (m_Accu.m_pTreeItem && !m_Accu.m_UsedMemory.IsEmpty())
    {
      xiiStringBuilder sSize;
      FormatSize(sSize, "", m_Accu.m_UsedMemory.PeekBack());

      xiiStringBuilder sMaxSize;
      FormatSize(sMaxSize, "", m_Accu.m_uiMaxUsedMemory);

      xiiStringBuilder sText = "<Accumulated>";
      sText.AppendFormat(" [{0}]", sSize);

      xiiStringBuilder sTooltip;
      sTooltip.SetFormat("<p>Current Memory Used: <b>{0}</b><br>Max Memory Used: <b>{1}</b><br>Live Allocations: <b>{2}</b><br>Allocations: <b>{3}</b><br>Deallocations: <b>{4}</b><br>", sSize.GetData(), sMaxSize.GetData(), m_Accu.m_uiLiveAllocs, m_Accu.m_uiAllocs, m_Accu.m_uiDeallocs);

      m_Accu.m_pTreeItem->setText(0, sText.GetData());
      m_Accu.m_pTreeItem->setToolTip(0, sTooltip.GetData());
    }
  }

  if (xiiTime::Now() - s_pWidget->m_LastUsedMemoryStored > xiiTime::MakeFromMilliseconds(200))
  {
    m_LastUsedMemoryStored = xiiTime::Now();

    for (auto it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      // sometimes no data arrives in time (game is too slow)
      // in this case simply assume the stats have not changed
      if (!it.Value().m_bReceivedData && !it.Value().m_UsedMemory.IsEmpty())
        it.Value().m_uiMaxUsedMemoryRecently = it.Value().m_UsedMemory.PeekBack();

      it.Value().m_UsedMemory.PushBack(it.Value().m_uiMaxUsedMemoryRecently);

      it.Value().m_uiMaxUsedMemoryRecently = 0;
      it.Value().m_bReceivedData           = false;
    }
  }
  else
    return;

  xiiUInt64 uiUsedMemory    = 0;
  xiiUInt64 uiLiveAllocs    = 0;
  xiiUInt64 uiAllocs        = 0;
  xiiUInt64 uiDeallocs      = 0;
  xiiUInt64 uiMinUsedMemory = 0xFFFFFFFFFFFFFFFFull;
  xiiUInt64 uiMaxUsedMemory = 0;

  {
    m_Accu.m_UsedMemory.SetCount(m_uiDisplaySamples);

    for (xiiUInt32 i = 0; i < m_uiDisplaySamples; ++i)
      m_Accu.m_UsedMemory[i] = 0;

    m_Accu.m_uiAllocs        = 0;
    m_Accu.m_uiDeallocs      = 0;
    m_Accu.m_uiLiveAllocs    = 0;
    m_Accu.m_uiMaxUsedMemory = 0;
  }

  for (auto it = s_pWidget->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_UsedMemory.IsEmpty() || !it.Value().m_bDisplay)
      continue;

    uiUsedMemory += it.Value().m_UsedMemory.PeekBack();
    uiLiveAllocs += it.Value().m_uiLiveAllocs;
    uiAllocs += it.Value().m_uiAllocs;
    uiDeallocs += it.Value().m_uiDeallocs;

    if (it.Value().m_UsedMemory.GetCount() > m_uiMaxSamples)
      it.Value().m_UsedMemory.PopFront(it.Value().m_UsedMemory.GetCount() - m_uiMaxSamples);

    const xiiUInt32 uiSampleCount = xiiMath::Min(m_uiDisplaySamples, it.Value().m_UsedMemory.GetCount());
    const xiiUInt32 uiFirstSample = it.Value().m_UsedMemory.GetCount() - uiSampleCount;
    const xiiUInt32 uiStartPos    = m_uiDisplaySamples - uiSampleCount;
    const bool      bTopLevel     = it.Value().m_uiParentId == xiiInvalidIndex;

    if (bTopLevel)
    {
      m_Accu.m_uiAllocs += it.Value().m_uiAllocs;
      m_Accu.m_uiDeallocs += it.Value().m_uiDeallocs;
      m_Accu.m_uiLiveAllocs += it.Value().m_uiLiveAllocs;
      m_Accu.m_uiMaxUsedMemory += it.Value().m_uiMaxUsedMemory;
    }

    for (xiiUInt32 i = 0; i < uiSampleCount; ++i)
    {
      const xiiUInt64 uiBytes = it.Value().m_UsedMemory[uiFirstSample + i];
      uiMinUsedMemory         = xiiMath::Min(uiMinUsedMemory, uiBytes);
      uiMaxUsedMemory         = xiiMath::Max(uiMaxUsedMemory, uiBytes);

      if (bTopLevel)
      {
        m_Accu.m_UsedMemory[uiStartPos + i] += uiBytes;
      }
    }
  }

  if (m_Accu.m_bDisplay)
  {
    for (xiiUInt64 uiBytes : m_Accu.m_UsedMemory)
    {
      uiMinUsedMemory = xiiMath::Min(uiMinUsedMemory, uiBytes);
      uiMaxUsedMemory = xiiMath::Max(uiMaxUsedMemory, uiBytes);
    }
  }

  uiMinUsedMemory = xiiMath::Min(uiMinUsedMemory, uiMaxUsedMemory);
  if (uiMinUsedMemory > 0)
  {
    uiMinUsedMemory = xiiMath::PowerOfTwo_Floor(uiMinUsedMemory);
  }
  if (uiMaxUsedMemory > 0 && uiMaxUsedMemory <= (xiiUInt64{1} << 63))
  {
    uiMaxUsedMemory = xiiMath::PowerOfTwo_Ceil(uiMaxUsedMemory);
  }

  // Qt uses integer geometry when fitting the view. Keep byte counts out of scene coordinates.
  const double fRange        = static_cast<double>(xiiMath::Max(uiMaxUsedMemory - uiMinUsedMemory, xiiUInt64{1}));
  const auto   appendSamples = [this, uiMinUsedMemory, fRange](QPainterPath& ref_path, const xiiDeque<xiiUInt64>& samples) {
    const xiiUInt32 uiSampleCount = xiiMath::Min(m_uiDisplaySamples, samples.GetCount());
    const xiiUInt32 uiFirstSample = samples.GetCount() - uiSampleCount;
    const xiiUInt32 uiStartPos    = m_uiDisplaySamples - uiSampleCount;

    for (xiiUInt32 i = 0; i < uiSampleCount; ++i)
    {
      const double  fHeight = static_cast<double>(samples[uiFirstSample + i] - uiMinUsedMemory) / fRange;
      const QPointF point(uiStartPos + i, fHeight);
      if (i == 0)
      {
        ref_path.moveTo(point);
      }
      else
      {
        ref_path.lineTo(point);
      }
    }
  };

  QPainterPath accumulatedPath;
  if (m_Accu.m_bDisplay)
  {
    appendSamples(accumulatedPath, m_Accu.m_UsedMemory);
  }
  m_pPathMax->setPath(accumulatedPath);

  QPainterPath allocatorPaths[s_uiMaxColors];
  for (const auto& entry : m_AllocatorData)
  {
    const auto& data = entry.Value();
    if (data.m_bDisplay)
    {
      appendSamples(allocatorPaths[data.m_uiColor % s_uiMaxColors], data.m_UsedMemory);
    }
  }

  for (xiiUInt32 i = 0; i < s_uiMaxColors; ++i)
  {
    m_pPath[i]->setPath(allocatorPaths[i]);
  }

  {
    const QRectF plotRect(0, 0, m_uiDisplaySamples - 1, 1);
    UsedMemoryView->setSceneRect(plotRect);
    UsedMemoryView->fitInView(plotRect, Qt::IgnoreAspectRatio);

    xiiStringBuilder s;

    FormatSize(s, "Min: ", uiMinUsedMemory);
    LabelMinMemory->setText(QString::fromUtf8(s.GetData()));

    s.SetFormat("<p>Recent Minimum Memory Usage:<br>{0} GB<br>{1} MB<br>{2} KB<br>{3} Byte</p>", xiiArgF(uiMinUsedMemory / 1024.0 / 1024.0 / 1024.0, 2), xiiArgF(uiMinUsedMemory / 1024.0 / 1024.0, 2), xiiArgF(uiMinUsedMemory / 1024.0, 2), uiMinUsedMemory);
    LabelMinMemory->setToolTip(QString::fromUtf8(s.GetData()));

    FormatSize(s, "Max: ", uiMaxUsedMemory);
    LabelMaxMemory->setText(QString::fromUtf8(s.GetData()));

    s.SetFormat("<p>Recent Maximum Memory Usage:<br>{0} GB<br>{1} MB<br>{2} KB<br>{3} Byte</p>", xiiArgF(uiMaxUsedMemory / 1024.0 / 1024.0 / 1024.0, 2), xiiArgF(uiMaxUsedMemory / 1024.0 / 1024.0, 2), xiiArgF(uiMaxUsedMemory / 1024.0, 2), uiMaxUsedMemory);
    LabelMaxMemory->setToolTip(QString::fromUtf8(s.GetData()));

    const xiiUInt64 uiCurUsedMemory = uiUsedMemory;

    FormatSize(s, "Sum: ", uiCurUsedMemory);
    LabelCurMemory->setText(QString::fromUtf8(s.GetData()));

    s.SetFormat("<p>Current Memory Usage:<br>{0} GB<br>{1} MB<br>{2} KB<br>{3} Byte</p>", xiiArgF(uiCurUsedMemory / 1024.0 / 1024.0 / 1024.0, 2), xiiArgF(uiCurUsedMemory / 1024.0 / 1024.0, 2), xiiArgF(uiCurUsedMemory / 1024.0, 2), uiCurUsedMemory);
    LabelCurMemory->setToolTip(QString::fromUtf8(s.GetData()));

    s.SetFormat("Allocs: {0}", uiLiveAllocs);
    LabelNumLiveAllocs->setText(QString::fromUtf8(s.GetData()));

    s.SetFormat("Counter: {0} / {1}", uiAllocs, uiDeallocs);
    LabelNumAllocs->setText(QString::fromUtf8(s.GetData()));

    s.SetFormat("<p>Allocations: <b>{0}</b><br>Deallocations: <b>{1}</b></p>", uiAllocs, uiDeallocs);
    LabelNumAllocs->setToolTip(QString::fromUtf8(s.GetData()));
  }
}

void xiiQtMemoryWidget::CustomContextMenuRequested(const QPoint& pos)
{
  QModelIndex CurrentIndex = ListAllocators->currentIndex();

  QMenu m;
  if (CurrentIndex.isValid())
  {
    m.addAction(actionEnableOnlyThis);
    m.addSeparator();
  }

  m.addAction(actionEnableAll);
  m.addAction(actionDisableAll);

  m.exec(ListAllocators->viewport()->mapToGlobal(pos));
}

void xiiQtMemoryWidget::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (!s_pWidget)
    return;
  for (auto it = s_pWidget->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    it.Value().m_bStillInUse = false;
  for (const auto& row : sample.m_Records)
  {
    xiiUInt32 id, parent;
    xiiString name;
    xiiUInt64 allocations, deallocations, bytes;
    if (!row.Read("id", id) || !row.Read("parent", parent) || !row.Read("name", name) || !row.Read("allocations", allocations) ||
        !row.Read("deallocations", deallocations) || !row.Read("bytes", bytes) || deallocations > allocations)
      continue;
    auto& data         = s_pWidget->m_AllocatorData[id];
    data.m_bStillInUse = data.m_bReceivedData = true;
    data.m_sName                              = name;
    data.m_uiParentId                         = parent;
    if (data.m_uiColor == 0xFF)
    {
      data.m_uiColor                  = s_pWidget->m_uiColorsUsed++;
      s_pWidget->m_bAllocatorsChanged = true;
    }
    data.m_uiAllocs                = allocations;
    data.m_uiDeallocs              = deallocations;
    data.m_uiLiveAllocs            = allocations - deallocations;
    data.m_uiMaxUsedMemoryRecently = xiiMath::Max(data.m_uiMaxUsedMemoryRecently, bytes);
    data.m_uiMaxUsedMemory         = xiiMath::Max(data.m_uiMaxUsedMemory, bytes);
  }
  for (auto it = s_pWidget->m_AllocatorData.GetIterator(); it.IsValid();)
  {
    if (!it.Value().m_bStillInUse)
    {
      s_pWidget->m_bAllocatorsChanged = true;
      it                              = s_pWidget->m_AllocatorData.Remove(it);
    }
    else
      ++it;
  }
}


void xiiQtMemoryWidget::on_ListAllocators_itemChanged(QTreeWidgetItem* item)
{
  if (item->data(0, Qt::UserRole).toUInt() == xiiInvalidIndex)
  {
    m_Accu.m_bDisplay = (item->checkState(0) == Qt::Checked);
    return;
  }

  m_AllocatorData[item->data(0, Qt::UserRole).toUInt()].m_bDisplay = (item->checkState(0) == Qt::Checked);
}

void xiiQtMemoryWidget::on_ComboTimeframe_currentIndexChanged(int index)
{
  const xiiUInt32 uiSeconds[] = {
    10,
    30,
    60 * 1,
    60 * 2,
    60 * 5,
    60 * 10,
  };

  m_uiDisplaySamples = 5 * uiSeconds[index]; // 5 samples per second
}

void xiiQtMemoryWidget::on_actionEnableOnlyThis_triggered(bool)
{
  on_actionDisableAll_triggered(false);

  m_AllocatorData[ListAllocators->currentItem()->data(0, Qt::UserRole).toUInt()].m_bDisplay = true;
}

void xiiQtMemoryWidget::on_actionEnableAll_triggered(bool)
{
  m_bAllocatorsChanged = true;

  for (auto it : m_AllocatorData)
  {
    it.Value().m_bDisplay = true;
  }
}

void xiiQtMemoryWidget::on_actionDisableAll_triggered(bool)
{
  m_bAllocatorsChanged = true;

  for (auto it : m_AllocatorData)
  {
    it.Value().m_bDisplay = false;
  }
}
