/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

class QHBoxLayout;

class XII_EDITORFRAMEWORK_DLL xiiQtRttiTypeStringPropertyWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtRttiTypeStringPropertyWidget();

protected Q_SLOTS:
  void onMenuAboutToShow();
  void OnTypeSelected(QString sTypeName);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  QPushButton* m_pButton = nullptr;
  QHBoxLayout* m_pLayout = nullptr;
  QMenu*       m_pMenu   = nullptr;

  xiiQtTypeMenu m_TypeMenu;
};
