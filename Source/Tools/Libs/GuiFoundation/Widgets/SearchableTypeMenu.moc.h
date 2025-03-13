#pragma once

#include <Foundation/Containers/Set.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QObject>

class xiiQtSearchableMenu;
class xiiRTTI;
class QMenu;

class XII_GUIFOUNDATION_DLL xiiQtTypeMenu : public QObject
{
  Q_OBJECT

public:
  void FillMenu(QMenu* pMenu, const xiiRTTI* pBaseType, bool bDerivedTypes, bool bSimpleMenu);

  static xiiDynamicArray<xiiString>* s_pRecentList;
  static bool                        s_bShowInDevelopmentFeatures;

  const xiiRTTI* m_pLastSelectedType = nullptr;

Q_SIGNALS:
  void TypeSelected(QString sTypeName);

protected Q_SLOTS:
  void OnMenuAction();

private:
  QMenu* CreateCategoryMenu(xiiStringView sCategory, xiiMap<xiiString, QMenu*>& existingMenus);
  void   OnMenuAction(const xiiRTTI* pRtti);

  QMenu*                 m_pMenu = nullptr;
  xiiSet<const xiiRTTI*> m_SupportedTypes;
  xiiQtSearchableMenu*   m_pSearchableMenu = nullptr;

  static xiiString s_sLastMenuSearch;
};
