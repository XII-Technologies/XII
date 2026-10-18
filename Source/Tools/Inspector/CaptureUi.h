/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once

#include <QGraphicsView>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QTreeWidget>
#include <QWheelEvent>
#include <cmath>

namespace xiiInspectorUi
{
  inline const char* QueueName(int queue)
  {
    static const char* names[] = {"Graphics", "Async compute", "Async transfer"};
    return queue >= 0 && queue < 3 ? names[queue] : "Unknown queue";
  }

  inline QColor QueueColor(int queue)
  {
    static const QColor colors[] = {QColor("#368bd6"), QColor("#b880dc"), QColor("#43a88e")};
    return queue >= 0 && queue < 3 ? colors[queue] : QColor(Qt::gray);
  }

  inline QString Index(const QJsonValue& value)
  {
    return value.toDouble() == 4294967295.0 ? QString("—") : QString::number(value.toInteger());
  }

  inline QString Indices(const QJsonArray& array)
  {
    QStringList result;
    for (const auto& value : array)
      result << Index(value);
    return result.join(", ");
  }

  class TableItem : public QTreeWidgetItem
  {
  public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem& other) const override
    {
      const int column = treeWidget()->sortColumn();
      bool leftNumeric = false, rightNumeric = false;
      const double left = text(column).toDouble(&leftNumeric), right = other.text(column).toDouble(&rightNumeric);
      return leftNumeric && rightNumeric ? left < right : text(column).localeAwareCompare(other.text(column)) < 0;
    }
  };

  inline QTreeWidget* Table(const QStringList& columns, const char* name)
  {
    auto* table = new QTreeWidget;
    table->setObjectName(name);
    table->setAccessibleName(name);
    table->setHeaderLabels(columns);
    table->setRootIsDecorated(false);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    table->header()->setSectionResizeMode(QHeaderView::Interactive);
    table->header()->setDefaultSectionSize(130);
    table->setUniformRowHeights(true);
    return table;
  }

  inline QTreeWidgetItem* Row(QTreeWidget* table, const QStringList& values, const QString& filter = {})
  {
    auto* item = new TableItem(table, values);
    item->setHidden(!filter.isEmpty() && !values.join(' ').contains(filter, Qt::CaseInsensitive));
    return item;
  }

  class GraphicsView : public QGraphicsView
  {
  public:
    explicit GraphicsView(QGraphicsScene* scene) : QGraphicsView(scene)
    {
      setRenderHint(QPainter::Antialiasing);
      setDragMode(QGraphicsView::ScrollHandDrag);
      setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
      setBackgroundBrush(QColor("#20242b"));
      setToolTip("Drag to pan. Ctrl+wheel to zoom. Use Fit to show the whole capture.");
      setMinimumHeight(180);
    }
    void wheelEvent(QWheelEvent* event) override
    {
      if (event->modifiers().testFlag(Qt::ControlModifier))
      {
        const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        const double next = transform().m11() * factor;
        if (next >= 0.02 && next <= 30)
          scale(factor, factor);
        event->accept();
      }
      else
        QGraphicsView::wheelEvent(event);
    }
  };
}
