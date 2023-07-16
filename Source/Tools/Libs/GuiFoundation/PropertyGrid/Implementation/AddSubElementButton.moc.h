#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;
class xiiQtSearchableMenu;

class XII_GUIFOUNDATION_DLL xiiQtAddSubElementButton : public xiiQtPropertyWidget
{
  Q_OBJECT

public:
  xiiQtAddSubElementButton();

  static bool s_bShowInDevelopmentFeatures;

protected:
  virtual void DoPrepareToDie() override {}

private Q_SLOTS:
  void onMenuAboutToShow();
  void on_Button_clicked();
  void OnMenuAction();

private:
  virtual void OnInit() override;
  void         OnAction(const xiiRTTI* pRtti);

  QMenu* CreateCategoryMenu(const char* szCategory, xiiMap<xiiString, QMenu*>& existingMenus);

  QHBoxLayout* m_pLayout;
  QPushButton* m_pButton;

  xiiSet<const xiiRTTI*> m_SupportedTypes;

  bool                 m_bNoMoreElementsAllowed = false;
  QMenu*               m_pMenu                  = nullptr;
  xiiQtSearchableMenu* m_pSearchableMenu        = nullptr;
  xiiUInt32            m_uiMaxElements          = 0; // 0 means unlimited
  bool                 m_bPreventDuplicates     = false;

  // used to remember the last search term entered into the searchable menu
  // this should probably be per 'distinguishable menu', but currently it is just global
  static xiiString s_sLastMenuSearch;
};
