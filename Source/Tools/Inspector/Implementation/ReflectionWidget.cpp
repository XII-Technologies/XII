/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Diagnostics/TelemetryService.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <qlistwidget.h>

xiiQtReflectionWidget* xiiQtReflectionWidget::s_pWidget = nullptr;

xiiQtReflectionWidget::xiiQtReflectionWidget(ads::CDockManager* pDockManager, QWidget* pParent) :
  ads::CDockWidget(pDockManager, "Reflection Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(ReflectionWidgetFrame);

  setIcon(QIcon(":/Icons/Icons/Type.svg"));

  ResetStats();
}

void xiiQtReflectionWidget::ResetStats()
{
  TypeTree->clear();

  {
    QStringList Headers;
    Headers.append(" Type ");
    Headers.append(" Property ");
    Headers.append(" Size (Bytes) ");
    Headers.append(" Plugin ");

    TypeTree->setColumnCount(static_cast<xiiInt32>(Headers.size()));
    TypeTree->setHeaderLabels(Headers);
    TypeTree->header()->show();
  }
}

void xiiQtReflectionWidget::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (!s_pWidget)
    return;
  s_pWidget->m_Types.Clear();
  s_pWidget->TypeTree->clear();
  for (const auto& row : sample.m_Records)
  {
    xiiString name;
    xiiUInt32 properties;
    TypeData  data;
    if (!row.Read("name", name) || !row.Read("parent", data.m_sParentType) || !row.Read("plugin", data.m_sPlugin) || !row.Read("size", data.m_uiSize) || !row.Read("properties", properties) || properties > 64)
      continue;
    for (xiiUInt32 i = 0; i < properties; ++i)
    {
      PropertyData     property;
      xiiStringBuilder key;
      key.SetFormat("name{}", i);
      if (!row.Read(key, property.m_sPropertyName))
        continue;
      key.SetFormat("category{}", i);
      if (!row.Read(key, property.m_iCategory))
        continue;
      key.SetFormat("type{}", i);
      if (!row.Read(key, property.m_sType))
        continue;
      data.m_Properties.PushBack(std::move(property));
    }
    s_pWidget->m_Types.Insert(name, std::move(data));
  }
  while (s_pWidget->UpdateTree())
  {
  }
  for (int column = 0; column < 4; ++column)
    s_pWidget->TypeTree->resizeColumnToContents(column);
}


bool xiiQtReflectionWidget::UpdateTree()
{
  bool bAddedAny = false;

  for (auto it = m_Types.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pTreeItem == nullptr)
    {

      const auto parent = m_Types.Find(it.Value().m_sParentType);
      if (parent.IsValid() && parent.Value().m_pTreeItem == nullptr)
        continue;
      QTreeWidgetItem* pParent = parent.IsValid() ? parent.Value().m_pTreeItem : nullptr;
      bAddedAny                = true;

      QTreeWidgetItem* pItem = new QTreeWidgetItem();
      it.Value().m_pTreeItem = pItem;

      xiiStringBuilder sText;
      sText.SetFormat("{0}", it.Value().m_uiSize);

      pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      pItem->setText(0, it.Key().GetData());
      pItem->setText(2, sText.GetData());
      pItem->setText(3, it.Value().m_sPlugin.GetData());

      pItem->setIcon(0, QIcon(":/Icons/Icons/Type.svg"));

      if (pParent)
      {
        pParent->addChild(pItem);
        pParent->setExpanded(false);
      }
      else
      {
        TypeTree->addTopLevelItem(pItem);
      }

      for (xiiUInt32 i = 0; i < it.Value().m_Properties.GetCount(); ++i)
      {
        QTreeWidgetItem* pProperty = new QTreeWidgetItem();
        pProperty->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        switch (it.Value().m_Properties[i].m_iCategory)
        {
          case -1:
            pProperty->setText(0, "Message");
            pProperty->setIcon(0, QIcon(":/Icons/Icons/Message.svg"));
            break;
          case xiiPropertyCategory::Member:
            pProperty->setText(0, it.Value().m_Properties[i].m_sType.GetData());
            pProperty->setIcon(0, QIcon(":/Icons/Icons/Member.svg"));
            break;
          case xiiPropertyCategory::Function:
            pProperty->setText(0, "Function");
            pProperty->setIcon(0, QIcon(":/Icons/Icons/Function.svg"));
            break;
          case xiiPropertyCategory::Array:
            pProperty->setText(0, it.Value().m_Properties[i].m_sType.GetData());
            pProperty->setIcon(0, QIcon(":/Icons/Icons/Array.svg"));
            break;
          case xiiPropertyCategory::Set:
            pProperty->setText(0, it.Value().m_Properties[i].m_sType.GetData());
            pProperty->setIcon(0, QIcon(":/Icons/Icons/Set.svg"));
            break;
          case xiiPropertyCategory::Map:
            pProperty->setText(0, it.Value().m_Properties[i].m_sType.GetData());
            pProperty->setIcon(0, QIcon(":/Icons/Icons/Map.svg"));
            break;
        }

        pProperty->setText(1, it.Value().m_Properties[i].m_sPropertyName.GetData());

        pItem->addChild(pProperty);
      }
    }
  }

  return bAddedAny;
}
