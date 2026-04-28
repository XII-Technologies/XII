/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class xiiDynamicStringEnum;
class xiiQtSearchableMenu;

class XII_EDITORFRAMEWORK_DLL xiiQtDynamicStringEnumPropertyWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtDynamicStringEnumPropertyWidget();

protected slots:
  void onMenuAboutToShow();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

  void SetNewValue(xiiStringView sNewValue);

protected:
  QHBoxLayout*                      m_pLayout         = nullptr;
  xiiDynamicStringEnum*             m_pEnum           = nullptr;
  QPushButton*                      m_pButton         = nullptr;
  QMenu*                            m_pMenu           = nullptr;
  xiiQtSearchableMenu*              m_pSearchableMenu = nullptr;
  xiiString                         m_sEnumAttribute;
  static xiiMap<xiiString, QString> s_LastSearch;
};
