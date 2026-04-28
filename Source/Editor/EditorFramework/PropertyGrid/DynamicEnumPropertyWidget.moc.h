/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class xiiDynamicEnum;
class xiiQtSearchableMenu;

class XII_EDITORFRAMEWORK_DLL xiiQtDynamicEnumPropertyWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtDynamicEnumPropertyWidget();

protected slots:
  void onMenuAboutToShow();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  QHBoxLayout*                      m_pLayout         = nullptr;
  xiiDynamicEnum*                   m_pEnum           = nullptr;
  QPushButton*                      m_pButton         = nullptr;
  QMenu*                            m_pMenu           = nullptr;
  xiiQtSearchableMenu*              m_pSearchableMenu = nullptr;
  xiiString                         m_sEnumAttribute;
  static xiiMap<xiiString, QString> s_LastSearch;
};
