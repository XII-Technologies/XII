/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_ReflectionWidget.h>
#include <ads/DockWidget.h>

class xiiQtReflectionWidget : public ads::CDockWidget, public Ui_ReflectionWidget
{
public:
  Q_OBJECT

public:
  xiiQtReflectionWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);

  static xiiQtReflectionWidget* s_pWidget;

private Q_SLOTS:

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct PropertyData
  {
    xiiString m_sType;
    xiiString m_sPropertyName;
    xiiInt8   m_iCategory;
  };

  struct TypeData
  {
    TypeData() { m_pTreeItem = nullptr; }

    QTreeWidgetItem* m_pTreeItem;

    xiiUInt32 m_uiSize;
    xiiString m_sParentType;
    xiiString m_sPlugin;

    xiiHybridArray<PropertyData, 16> m_Properties;
  };

  bool UpdateTree();

  xiiMap<xiiString, TypeData> m_Types;
};
