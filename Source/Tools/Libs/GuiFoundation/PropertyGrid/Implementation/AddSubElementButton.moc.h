/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;

/// Used by container widgets to add new elements to the container.
class XII_GUIFOUNDATION_DLL xiiQtAddSubElementButton : public xiiQtPropertyWidget
{
  Q_OBJECT

public:
  /// Constructor
  /// \param containerCategory The type of container. Only Map, Set and Array are supported.
  xiiQtAddSubElementButton(xiiEnum<xiiPropertyCategory> containerCategory);

protected:
  virtual void DoPrepareToDie() override {}

private Q_SLOTS:
  void onMenuAboutToShow();
  void on_Button_clicked();
  void OnTypeSelected(QString sTypeName);

private:
  virtual void OnInit() override;
  void         OnAction(const xiiRTTI* pRtti);

  QHBoxLayout* m_pLayout;
  QPushButton* m_pButton;

  xiiQtTypeMenu m_TypeMenu;

  bool                         m_bNoMoreElementsAllowed = false;
  QMenu*                       m_pMenu                  = nullptr;
  xiiUInt32                    m_uiMaxElements          = 0; // 0 means unlimited
  bool                         m_bPreventDuplicates     = false;
  xiiEnum<xiiPropertyCategory> m_ContainerCategory;
};
