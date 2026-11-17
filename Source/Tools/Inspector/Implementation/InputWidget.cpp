/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Diagnostics/TelemetryService.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qlistwidget.h>

xiiQtInputWidget* xiiQtInputWidget::s_pWidget = nullptr;

xiiQtInputWidget::xiiQtInputWidget(ads::CDockManager* pDockManager, QWidget* pParent) :
  ads::CDockWidget(pDockManager, "Input Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(InputWidgetFrame);

  setIcon(QIcon(":/Icons/Icons/InputActions.svg"));

  ResetStats();
}

void xiiQtInputWidget::ResetStats()
{
  ClearSlots();
  ClearActions();
}

void xiiQtInputWidget::ClearSlots()
{
  m_InputSlots.Clear();
  TableInputSlots->clear();

  {
    QStringList Headers;
    Headers.append("");
    Headers.append(" Slot ");
    Headers.append(" State ");
    Headers.append(" Value ");
    Headers.append(" Dead Zone ");
    Headers.append(" Flags (Binary) ");

    TableInputSlots->setColumnCount(static_cast<xiiInt32>(Headers.size()));

    TableInputSlots->setHorizontalHeaderLabels(Headers);
    TableInputSlots->horizontalHeader()->show();
  }
}

void xiiQtInputWidget::ClearActions()
{
  m_InputActions.Clear();
  TableInputActions->clear();

  {
    QStringList Headers;
    Headers.append("");
    Headers.append(" Action ");
    Headers.append(" State ");
    Headers.append(" Value ");

    for (xiiInt32 slot = 0; slot < xiiInputActionConfig::MaxInputSlotAlternatives; ++slot)
      Headers.append(QString(" Slot %1 ").arg(slot + 1));

    TableInputActions->setColumnCount(static_cast<xiiInt32>(Headers.size()));

    TableInputActions->setHorizontalHeaderLabels(Headers);
    TableInputActions->horizontalHeader()->show();
  }
}

void xiiQtInputWidget::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (!s_pWidget)
    return;
  s_pWidget->m_InputSlots.Clear();
  s_pWidget->m_InputActions.Clear();
  for (const auto& row : sample.m_Records)
  {
    xiiUInt8  kind, state;
    xiiString name;
    float     value;
    if (!row.Read("kind", kind) || !row.Read("state", state) || !row.Read("name", name) || !row.Read("value", value) || state > xiiKeyState::Released)
      continue;
    if (kind == 0)
    {
      SlotData  data;
      xiiUInt32 flags;
      if (!row.Read("flags", flags) || !row.Read("deadZone", data.m_fDeadZone))
        continue;
      data.m_uiSlotFlags = static_cast<decltype(data.m_uiSlotFlags)>(flags);
      data.m_KeyState    = static_cast<xiiKeyState::Enum>(state);
      data.m_fValue      = value;
      s_pWidget->m_InputSlots.Insert(name, data);
    }
    else if (kind == 1)
    {
      xiiString  set;
      ActionData data;
      if (!row.Read("set", set) || !row.Read("timeScaling", data.m_bUseTimeScaling))
        continue;
      data.m_KeyState = static_cast<xiiKeyState::Enum>(state);
      data.m_fValue   = value;
      bool valid      = true;
      for (xiiUInt32 i = 0; i < xiiInputActionConfig::MaxInputSlotAlternatives; ++i)
      {
        xiiStringBuilder key;
        key.SetFormat("trigger{}", i);
        valid &= row.Read(key, data.m_sTrigger[i]);
        key.SetFormat("scale{}", i);
        valid &= row.Read(key, data.m_fTriggerScaling[i]);
      }
      if (!valid)
        continue;
      xiiStringBuilder key(set, "::", name);
      s_pWidget->m_InputActions.Insert(key, data);
    }
  }
  s_pWidget->UpdateSlotTable(true);
  s_pWidget->UpdateActionTable(true);
}


void xiiQtInputWidget::UpdateSlotTable(bool bRecreate)
{
  xiiQtScopedUpdatesDisabled _1(TableInputSlots);

  if (bRecreate)
  {
    TableInputSlots->clear();
    TableInputSlots->setRowCount(m_InputSlots.GetCount());

    QStringList Headers;
    Headers.append("");
    Headers.append(" Slot ");
    Headers.append(" State ");
    Headers.append(" Value ");
    Headers.append(" Dead Zone ");
    Headers.append(" Flags (Binary) ");

    TableInputSlots->setColumnCount(static_cast<xiiInt32>(Headers.size()));

    TableInputSlots->setHorizontalHeaderLabels(Headers);
    TableInputSlots->horizontalHeader()->show();

    xiiStringBuilder sTemp;

    xiiInt32 iRow = 0;
    for (xiiMap<xiiString, SlotData>::Iterator it = m_InputSlots.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      sTemp.SetFormat("  {0}  ", it.Key());

      QLabel* pIcon = new QLabel();
      QIcon   icon  = xiiQtUiServices::GetCachedIconResource(":/Icons/Icons/InputSlots.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableInputSlots->setCellWidget(iRow, 0, pIcon);

      TableInputSlots->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));
      TableInputSlots->setCellWidget(iRow, 2, new QLabel("????????????"));
      TableInputSlots->setCellWidget(iRow, 3, new QLabel("????????"));
      TableInputSlots->setCellWidget(iRow, 4, new QLabel("????????"));
      TableInputSlots->setCellWidget(iRow, 5, new QLabel("????????????????"));

      const xiiUInt32 uiFlags = it.Value().m_uiSlotFlags;

      // Flags
      {
        xiiStringBuilder sFlags;
        sFlags.SetPrintf("  %16b  ", uiFlags);

        QLabel* pFlags = (QLabel*)TableInputSlots->cellWidget(iRow, 5);
        pFlags->setAlignment(Qt::AlignRight);
        pFlags->setText(sFlags.GetData());
      }

      // Flags Tooltip
      {
        // in VS 2012 at least the snprintf fails when "yes" and "no" are passed directly, instead of as const char* variables
        const char* szYes = "<b>yes</b>";
        const char* szNo  = "no";

        xiiStringBuilder tt("<p>");
        tt.AppendFormat("ReportsRelativeValues: {0}<br>", (uiFlags & xiiInputSlotFlags::ReportsRelativeValues) ? szYes : szNo);
        tt.AppendFormat("ValueBinaryZeroOrOne: {0}<br>", (uiFlags & xiiInputSlotFlags::ValueBinaryZeroOrOne) ? szYes : szNo);
        tt.AppendFormat("ValueRangeZeroToOne: {0}<br>", (uiFlags & xiiInputSlotFlags::ValueRangeZeroToOne) ? szYes : szNo);
        tt.AppendFormat("ValueRangeZeroToInf: {0}<br>", (uiFlags & xiiInputSlotFlags::ValueRangeZeroToInf) ? szYes : szNo);
        tt.AppendFormat("Pressable: {0}<br>", (uiFlags & xiiInputSlotFlags::Pressable) ? szYes : szNo);
        tt.AppendFormat("Holdable: {0}<br>", (uiFlags & xiiInputSlotFlags::Holdable) ? szYes : szNo);
        tt.AppendFormat("HalfAxis: {0}<br>", (uiFlags & xiiInputSlotFlags::HalfAxis) ? szYes : szNo);
        tt.AppendFormat("FullAxis: {0}<br>", (uiFlags & xiiInputSlotFlags::FullAxis) ? szYes : szNo);
        tt.AppendFormat("RequiresDeadZone: {0}<br>", (uiFlags & xiiInputSlotFlags::RequiresDeadZone) ? szYes : szNo);
        tt.AppendFormat("ValuesAreNonContinuous: {0}<br>", (uiFlags & xiiInputSlotFlags::ValuesAreNonContinuous) ? szYes : szNo);
        tt.AppendFormat("ActivationDependsOnOthers: {0}<br>", (uiFlags & xiiInputSlotFlags::ActivationDependsOnOthers) ? szYes : szNo);
        tt.AppendFormat("NeverTimeScale: {0}<br>", (uiFlags & xiiInputSlotFlags::NeverTimeScale) ? szYes : szNo);
        tt.Append("</p>");

        TableInputSlots->cellWidget(iRow, 5)->setToolTip(tt.GetData());
      }

      ++iRow;
    }

    TableInputSlots->resizeColumnsToContents();
  }

  {
    xiiStringBuilder sTemp;

    xiiInt32 iRow = 0;
    for (xiiMap<xiiString, SlotData>::Iterator it = m_InputSlots.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pState = (QLabel*)TableInputSlots->cellWidget(iRow, 2);
      pState->setAlignment(Qt::AlignHCenter);

      switch (it.Value().m_KeyState)
      {
        case xiiKeyState::Down:
          pState->setText("Down");
          break;
        case xiiKeyState::Pressed:
          pState->setText("Pressed");
          break;
        case xiiKeyState::Released:
          pState->setText("Released");
          break;
        case xiiKeyState::Up:
          pState->setText("");
          break;
      }

      // Value
      {
        QLabel* pValue = (QLabel*)TableInputSlots->cellWidget(iRow, 3);
        pValue->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fValue == 0.0f)
          pValue->setText("");
        else
        {
          sTemp.SetFormat(" {0} ", xiiArgF(it.Value().m_fValue, 4));
          pValue->setText(sTemp.GetData());
        }
      }

      // Dead-zone
      {
        QLabel* pDeadZone = (QLabel*)TableInputSlots->cellWidget(iRow, 4);
        pDeadZone->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fDeadZone == 0.0f)
          pDeadZone->setText("");
        else
          pDeadZone->setText(QString::number(it.Value().m_fDeadZone, 'f', 2));
      }

      ++iRow;
    }
  }
}

void xiiQtInputWidget::UpdateActionTable(bool bRecreate)
{
  xiiQtScopedUpdatesDisabled _1(TableInputActions);

  if (bRecreate)
  {
    TableInputActions->clear();
    TableInputActions->setRowCount(m_InputActions.GetCount());

    QStringList Headers;
    Headers.append("");
    Headers.append(" Action ");
    Headers.append(" State ");
    Headers.append(" Value ");

    for (xiiInt32 slot = 0; slot < xiiInputActionConfig::MaxInputSlotAlternatives; ++slot)
      Headers.append(QString(" Slot %1 ").arg(slot + 1));

    TableInputActions->setColumnCount(static_cast<xiiInt32>(Headers.size()));

    TableInputActions->setHorizontalHeaderLabels(Headers);
    TableInputActions->horizontalHeader()->show();

    xiiStringBuilder sTemp;

    xiiInt32 iRow = 0;
    for (xiiMap<xiiString, ActionData>::Iterator it = m_InputActions.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      sTemp.SetFormat("  {0}  ", it.Key());

      QLabel* pIcon = new QLabel();
      QIcon   icon  = xiiQtUiServices::GetCachedIconResource(":/Icons/Icons/InputActions.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableInputActions->setCellWidget(iRow, 0, pIcon);

      TableInputActions->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));
      TableInputActions->setCellWidget(iRow, 2, new QLabel("????????????"));
      TableInputActions->setCellWidget(iRow, 3, new QLabel("????????????????????????"));
      TableInputActions->setCellWidget(iRow, 4, new QLabel(""));
      TableInputActions->setCellWidget(iRow, 5, new QLabel(""));
      TableInputActions->setCellWidget(iRow, 6, new QLabel(""));

      // Trigger Slots

      for (xiiInt32 slot = 0; slot < xiiInputActionConfig::MaxInputSlotAlternatives; ++slot)
      {
        if (it.Value().m_sTrigger[slot].IsEmpty())
          sTemp = "  ";
        else
          sTemp.SetFormat("  [Scale: {0}] {1}  ", xiiArgF(it.Value().m_fTriggerScaling[slot], 2), it.Value().m_sTrigger[slot]);

        QLabel* pValue = (QLabel*)TableInputActions->cellWidget(iRow, 4 + slot);
        pValue->setText(sTemp.GetData());
      }

      ++iRow;
    }

    TableInputActions->resizeColumnsToContents();
  }

  {
    xiiStringBuilder sTemp;

    xiiInt32 iRow = 0;
    for (xiiMap<xiiString, ActionData>::Iterator it = m_InputActions.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pState = (QLabel*)TableInputActions->cellWidget(iRow, 2);
      pState->setAlignment(Qt::AlignHCenter);

      switch (it.Value().m_KeyState)
      {
        case xiiKeyState::Down:
          pState->setText("Down");
          break;
        case xiiKeyState::Pressed:
          pState->setText("Pressed");
          break;
        case xiiKeyState::Released:
          pState->setText("Released");
          break;
        case xiiKeyState::Up:
          pState->setText("");
          break;
      }

      // Value
      {
        QLabel* pValue = (QLabel*)TableInputActions->cellWidget(iRow, 3);
        pValue->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fValue == 0.0f)
          pValue->setText("");
        else
        {
          if (it.Value().m_bUseTimeScaling)
            sTemp.SetFormat(" {0} (Time-Scaled) ", xiiArgF(it.Value().m_fValue, 4));
          else
            sTemp.SetFormat(" {0} (Absolute) ", xiiArgF(it.Value().m_fValue, 4));

          pValue->setText(sTemp.GetData());
        }
      }


      ++iRow;
    }
  }
}

void xiiQtInputWidget::on_ButtonClearSlots_clicked()
{
  ClearSlots();
}

void xiiQtInputWidget::on_ButtonClearActions_clicked()
{
  ClearActions();
}
