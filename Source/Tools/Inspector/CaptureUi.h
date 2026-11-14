/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <QGraphicsView>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QTreeWidget>
#include <QWheelEvent>
#include <cmath>

namespace xiiInspectorUi
{
  inline const char* QueueName(int queue)
  {
    static const char* names[] = {"Graphics Queue 0", "Compute Queue 1", "Transfer Queue 2"};
    return queue >= 0 && queue < 3 ? names[queue] : "Unknown queue";
  }

  inline QString GraphLabel(const QMap<quint64, QString>& graphs, quint64 id)
  {
    const auto name       = graphs.value(id, "Render graph");
    int        duplicates = 0;
    int        ordinal    = 0;
    for (auto it = graphs.begin(); it != graphs.end(); ++it)
    {
      if (it.value() == name)
      {
        ++duplicates;
        if (it.key() <= id)
        {
          ++ordinal;
        }
      }
    }
    return duplicates > 1 ? name + QString(" · Graph %1").arg(ordinal) : name;
  }

  inline QString ResourceName(const QJsonObject& resource, int index)
  {
    const auto name = resource["name"].toString();
    return name.isEmpty() ? QString("%1 %2").arg(resource["texture"].toBool() ? "Texture" : "Buffer").arg(index + 1) : name;
  }

  inline QString Bytes(quint64 bytes)
  {
    if (bytes >= 1024 * 1024)
    {
      return QString::number(double(bytes) / (1024 * 1024), 'f', 2) + " MiB";
    }
    if (bytes >= 1024)
    {
      return QString::number(double(bytes) / 1024, 'f', 2) + " KiB";
    }
    return QString::number(bytes) + " B";
  }

  inline QColor QueueColor(int queue)
  {
    static const QColor colors[] = {QColor("#368bd6"), QColor("#b880dc"), QColor("#43a88e")};
    return queue >= 0 && queue < 3 ? colors[queue] : QColor(Qt::gray);
  }

  inline QString Index(const QJsonValue& value)
  {
    return value.toDouble() == 4294967295.0 ? QString("-") : QString::number(value.toInteger());
  }

  inline QString Indices(const QJsonArray& array)
  {
    QStringList result;
    for (const auto& value : array)
    {
      result << Index(value);
    }
    return result.join(", ");
  }

  class TableItem : public QTreeWidgetItem
  {
  public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem& other) const override
    {
      const int    column       = treeWidget()->sortColumn();
      bool         leftNumeric  = false;
      bool         rightNumeric = false;
      const double left         = text(column).toDouble(&leftNumeric);
      const double right        = other.text(column).toDouble(&rightNumeric);
      if (leftNumeric != rightNumeric)
      {
        return !leftNumeric;
      }
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
      setAlignment(Qt::AlignLeft | Qt::AlignTop);
      setToolTip("Drag to pan. Ctrl+wheel to zoom. Use Fit to show the whole capture.");
      setMinimumHeight(180);
    }
    void wheelEvent(QWheelEvent* event) override
    {
      if (event->modifiers().testFlag(Qt::ControlModifier))
      {
        const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        const double next   = transform().m11() * factor;
        if (next >= 0.02 && next <= 30)
        {
          scale(factor, factor);
        }
        event->accept();
      }
      else
      {
        QGraphicsView::wheelEvent(event);
      }
    }
  };
} // namespace xiiInspectorUi
