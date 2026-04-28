/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

bool                        xiiQtTypeMenu::s_bShowInDevelopmentFeatures = false;
xiiDynamicArray<xiiString>* xiiQtTypeMenu::s_pRecentList                = nullptr;

struct TypeComparer
{
  XII_FORCE_INLINE bool Less(const xiiRTTI* a, const xiiRTTI* b) const
  {
    const xiiCategoryAttribute* pCatA = a->GetAttributeByType<xiiCategoryAttribute>();
    const xiiCategoryAttribute* pCatB = b->GetAttributeByType<xiiCategoryAttribute>();
    if (pCatA != nullptr && pCatB == nullptr)
    {
      return true;
    }
    else if (pCatA == nullptr && pCatB != nullptr)
    {
      return false;
    }
    else if (pCatA != nullptr && pCatB != nullptr)
    {
      xiiInt32 iRes = pCatA->GetCategory().Compare(pCatB->GetCategory());
      if (iRes != 0)
      {
        return iRes < 0;
      }
    }

    return a->GetTypeName().Compare(b->GetTypeName()) < 0;
  }
};

xiiString xiiQtTypeMenu::s_sLastMenuSearch;

QMenu* xiiQtTypeMenu::CreateCategoryMenu(xiiStringView sCategory, xiiMap<xiiString, QMenu*>& existingMenus)
{
  if (sCategory.IsEmpty())
    return m_pMenu;

  auto it = existingMenus.Find(sCategory);
  if (it.IsValid())
    return it.Value();

  xiiStringBuilder sPath = sCategory;
  sPath.PathParentDirectory();
  sPath.Trim("/");

  QMenu* pParentMenu = m_pMenu;

  if (!sPath.IsEmpty())
  {
    pParentMenu = CreateCategoryMenu(sPath, existingMenus);
  }

  sPath = sCategory;
  sPath = sPath.GetFileName();

  QMenu* pNewMenu          = pParentMenu->addMenu(xiiMakeQString(xiiTranslate(sPath)));
  existingMenus[sCategory] = pNewMenu;

  return pNewMenu;
}

void xiiQtTypeMenu::OnMenuAction()
{
  const xiiRTTI* pRtti = static_cast<const xiiRTTI*>(sender()->property("type").value<void*>());

  OnMenuAction(pRtti);
}

void xiiQtTypeMenu::OnMenuAction(const xiiRTTI* pRtti)
{
  m_pLastSelectedType = pRtti;

  if (s_pRecentList && !s_pRecentList->Contains(pRtti->GetTypeName()))
  {
    if (s_pRecentList->GetCount() > 32)
    {
      s_pRecentList->RemoveAtAndCopy(0);
    }

    s_pRecentList->PushBack(pRtti->GetTypeName());
  }

  Q_EMIT TypeSelected(xiiMakeQString(pRtti->GetTypeName()));
}

void xiiQtTypeMenu::FillMenu(QMenu* pMenu, const xiiRTTI* pBaseType, bool bDerivedTypes, bool bSimpleMenu)
{
  m_pMenu = pMenu;

  m_SupportedTypes.Clear();
  m_SupportedTypes.Insert(pBaseType);

  if (bDerivedTypes)
  {
    xiiReflectionUtils::GatherTypesDerivedFromClass(pBaseType, m_SupportedTypes);
  }

  // Make category-sorted array of types and skip all abstract, hidden or in development types
  xiiDynamicArray<const xiiRTTI*> supportedTypes;
  for (const xiiRTTI* pRtti : m_SupportedTypes)
  {
    if (pRtti->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract))
      continue;

    if (pRtti->GetAttributeByType<xiiHiddenAttribute>() != nullptr)
      continue;

    if (!s_bShowInDevelopmentFeatures && pRtti->GetAttributeByType<xiiInDevelopmentAttribute>() != nullptr)
      continue;

    supportedTypes.PushBack(pRtti);
  }
  supportedTypes.Sort(TypeComparer());

  if (!bSimpleMenu && supportedTypes.GetCount() > 10)
  {
    // only show a searchable menu when it makes some sense
    // also deactivating entries to prevent duplicates is currently not supported by the searchable menu
    m_pSearchableMenu = new xiiQtSearchableMenu(m_pMenu);
  }

  xiiStringBuilder sIconName;
  xiiStringBuilder sCategory = "";

  xiiMap<xiiString, QMenu*> existingMenus;

  if (m_pSearchableMenu == nullptr)
  {
    // first round: create all sub menus
    for (const xiiRTTI* pRtti : supportedTypes)
    {
      // Determine current menu
      const xiiCategoryAttribute* pCatA = pRtti->GetAttributeByType<xiiCategoryAttribute>();

      if (pCatA)
      {
        CreateCategoryMenu(pCatA->GetCategory(), existingMenus);
      }
    }
  }

  if (m_pSearchableMenu != nullptr)
  {
    // add recently used sub-menu
    if (s_pRecentList)
    {
      xiiStringBuilder sInternalPath, sDisplayName;

      xiiInt32 iToAdd = 8;

      for (auto& sTypeName : *s_pRecentList)
      {
        const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(sTypeName);

        if (pRtti == nullptr)
          continue;

        if (!pRtti->IsDerivedFrom(pBaseType))
          continue;

        sIconName.Set(":/TypeIcons/", pRtti->GetTypeName(), ".svg");

        sInternalPath.Set(" *** RECENT ***/", pRtti->GetTypeName());

        sDisplayName = xiiTranslate(pRtti->GetTypeName());

        const xiiCategoryAttribute* pCatA = pRtti->GetAttributeByType<xiiCategoryAttribute>();
        const xiiColorAttribute*    pColA = pRtti->GetAttributeByType<xiiColorAttribute>();

        xiiColor iconColor = xiiColor::MakeZero();

        if (pColA)
        {
          iconColor = pColA->GetColor();
        }
        else if (pCatA && iconColor == xiiColor::MakeZero())
        {
          iconColor = xiiColorScheme::GetCategoryColor(pCatA->GetCategory(), xiiColorScheme::CategoryColorUsage::MenuEntryIcon);
        }

        const QIcon actionIcon = xiiQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);

        m_pSearchableMenu->AddItem(sDisplayName, sInternalPath, QVariant::fromValue((void*)pRtti), actionIcon);

        if (--iToAdd <= 0)
          break;
      }
    }
  }

  xiiStringBuilder tmp;

  // second round: create the actions
  for (const xiiRTTI* pRtti : supportedTypes)
  {
    sIconName.Set(":/TypeIcons/", pRtti->GetTypeName(), ".svg");

    // Determine current menu
    const xiiCategoryAttribute*      pCatA  = pRtti->GetAttributeByType<xiiCategoryAttribute>();
    const xiiInDevelopmentAttribute* pInDev = pRtti->GetAttributeByType<xiiInDevelopmentAttribute>();
    const xiiColorAttribute*         pColA  = pRtti->GetAttributeByType<xiiColorAttribute>();

    xiiColor iconColor = xiiColor::MakeZero();

    if (pColA)
    {
      iconColor = pColA->GetColor();
    }
    else if (pCatA && iconColor == xiiColor::MakeZero())
    {
      iconColor = xiiColorScheme::GetCategoryColor(pCatA->GetCategory(), xiiColorScheme::CategoryColorUsage::MenuEntryIcon);
    }

    const QIcon actionIcon = xiiQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);


    if (m_pSearchableMenu != nullptr)
    {
      xiiStringBuilder sFullPath;
      sFullPath = pCatA ? pCatA->GetCategory() : "";
      sFullPath.AppendPath(pRtti->GetTypeName());

      xiiStringBuilder sDisplayName = xiiTranslate(pRtti->GetTypeName());
      if (pInDev)
      {
        sDisplayName.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      m_pSearchableMenu->AddItem(sDisplayName, sFullPath, QVariant::fromValue((void*)pRtti), actionIcon);
    }
    else
    {
      QMenu* pCat = CreateCategoryMenu(pCatA ? pCatA->GetCategory() : nullptr, existingMenus);

      xiiStringBuilder fullName = xiiTranslate(pRtti->GetTypeName());

      if (pInDev)
      {
        fullName.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      // Add type action to current menu
      QAction* pAction = new QAction(fullName.GetData(), m_pMenu);
      pAction->setProperty("type", QVariant::fromValue((void*)pRtti));
      XII_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(OnMenuAction())) != nullptr, "connection failed");

      pAction->setIcon(actionIcon);

      pCat->addAction(pAction);
    }
  }

  if (m_pSearchableMenu != nullptr)
  {
    connect(m_pSearchableMenu, &xiiQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant) {
      const xiiRTTI* pRtti = static_cast<const xiiRTTI*>(variant.value<void*>());

      OnMenuAction(pRtti);

      m_pMenu->close();
      //
    });

    connect(m_pSearchableMenu, &xiiQtSearchableMenu::SearchTextChanged, m_pMenu, [this](const QString& sText) { xiiQtTypeMenu::s_sLastMenuSearch = sText.toUtf8().data(); });

    m_pMenu->addAction(m_pSearchableMenu);

    // important to do this last to make sure the search bar gets focus
    m_pSearchableMenu->Finalize(xiiQtTypeMenu::s_sLastMenuSearch.GetData());
  }
}
