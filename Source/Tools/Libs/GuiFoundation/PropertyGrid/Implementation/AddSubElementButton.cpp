#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QPushButton>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiString xiiQtAddSubElementButton::s_sLastMenuSearch;
bool      xiiQtAddSubElementButton::s_bShowInDevelopmentFeatures = false;

xiiQtAddSubElementButton::xiiQtAddSubElementButton() :
  xiiQtPropertyWidget()
{
  // Reset base class size policy as we are put in a layout that would cause us to vanish instead.
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText("Add Item");
  m_pButton->setObjectName("Button");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addSpacerItem(new QSpacerItem(0, 0));
  m_pLayout->setStretch(0, 1);
  m_pLayout->addWidget(m_pButton);
  m_pLayout->addSpacerItem(new QSpacerItem(0, 0));
  m_pLayout->setStretch(2, 1);

  m_pMenu = nullptr;
}

void xiiQtAddSubElementButton::OnInit()
{
  if (m_pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
  {
    m_pMenu = new QMenu(m_pButton);
    m_pMenu->setToolTipsVisible(true);
    connect(m_pMenu, &QMenu::aboutToShow, this, &xiiQtAddSubElementButton::onMenuAboutToShow);
    m_pButton->setMenu(m_pMenu);
    m_pButton->setObjectName("Button");
  }

  if (const xiiMaxArraySizeAttribute* pAttr = m_pProp->GetAttributeByType<xiiMaxArraySizeAttribute>())
  {
    m_uiMaxElements = pAttr->GetMaxSize();
  }

  if (const xiiPreventDuplicatesAttribute* pAttr = m_pProp->GetAttributeByType<xiiPreventDuplicatesAttribute>())
  {
    m_bPreventDuplicates = true;
  }

  QMetaObject::connectSlotsByName(this);
}

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

QMenu* xiiQtAddSubElementButton::CreateCategoryMenu(xiiStringView sCategory, xiiMap<xiiString, QMenu*>& existingMenus)
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

  QMenu* pNewMenu          = pParentMenu->addMenu(xiiTranslate(sPath));
  existingMenus[sCategory] = pNewMenu;

  return pNewMenu;
}

void xiiQtAddSubElementButton::onMenuAboutToShow()
{
  if (m_Items.IsEmpty())
    return;

  if (m_pMenu->isEmpty())
  {
    auto pProp = GetProperty();

    if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
    {
      m_SupportedTypes.Clear();
      xiiReflectionUtils::GatherTypesDerivedFromClass(pProp->GetSpecificType(), m_SupportedTypes, false);
    }
    m_SupportedTypes.Insert(pProp->GetSpecificType());

    // remove all types that are marked as hidden
    for (auto it = m_SupportedTypes.GetIterator(); it.IsValid();)
    {
      if (it.Key()->GetAttributeByType<xiiHiddenAttribute>() != nullptr)
      {
        it = m_SupportedTypes.Remove(it);
        continue;
      }

      if (!s_bShowInDevelopmentFeatures)
      {
        if (auto pInDev = it.Key()->GetAttributeByType<xiiInDevelopmentAttribute>())
        {
          it = m_SupportedTypes.Remove(it);
          continue;
        }
      }

      ++it;
    }

    // Make category-sorted array of types
    xiiDynamicArray<const xiiRTTI*> supportedTypes;
    for (const xiiRTTI* pRtti : m_SupportedTypes)
    {
      if (pRtti->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract))
        continue;

      supportedTypes.PushBack(pRtti);
    }
    supportedTypes.Sort(TypeComparer());

    if (!m_bPreventDuplicates && supportedTypes.GetCount() > 10)
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

    xiiStringBuilder tmp;

    // second round: create the actions
    for (const xiiRTTI* pRtti : supportedTypes)
    {
      sIconName.Set(":/TypeIcons/", pRtti->GetTypeName());
      const QIcon actionIcon = xiiQtUiServices::GetCachedIconResource(sIconName.GetData());

      // Determine current menu
      const xiiCategoryAttribute*      pCatA  = pRtti->GetAttributeByType<xiiCategoryAttribute>();
      const xiiInDevelopmentAttribute* pInDev = pRtti->GetAttributeByType<xiiInDevelopmentAttribute>();

      if (m_pSearchableMenu != nullptr)
      {
        xiiStringBuilder sFullPath;
        sFullPath = pCatA ? pCatA->GetCategory() : "";
        sFullPath.AppendPath(pRtti->GetTypeName());

        xiiStringBuilder sDisplayName = xiiTranslate(pRtti->GetTypeName().GetData(tmp));
        if (pInDev)
        {
          sDisplayName.AppendFormat(" [ {} ]", pInDev->GetString());
        }

        m_pSearchableMenu->AddItem(sDisplayName, sFullPath, QVariant::fromValue((void*)pRtti), actionIcon);
      }
      else
      {
        QMenu* pCat = CreateCategoryMenu(pCatA ? pCatA->GetCategory() : nullptr, existingMenus);

        xiiStringBuilder fullName = xiiTranslate(pRtti->GetTypeName().GetData(tmp));

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

        OnAction(pRtti);
        m_pMenu->close();
      });

      connect(m_pSearchableMenu, &xiiQtSearchableMenu::SearchTextChanged, m_pMenu, [this](const QString& sText) {
        s_sLastMenuSearch = sText.toUtf8().data();
      });

      m_pMenu->addAction(m_pSearchableMenu);

      // important to do this last to make sure the search bar gets focus
      m_pSearchableMenu->Finalize(s_sLastMenuSearch.GetData());
    }
  }

  if (m_uiMaxElements > 0) // 0 means unlimited
  {
    QList<QAction*> actions = m_pMenu->actions();

    for (auto& item : m_Items)
    {
      xiiInt32 iCount = 0;
      m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).AssertSuccess();

      if (iCount >= (xiiInt32)m_uiMaxElements)
      {
        if (!m_bNoMoreElementsAllowed)
        {
          m_bNoMoreElementsAllowed = true;

          QAction* pAction = new QAction(QString("Maximum allowed elements in array is %1").arg(m_uiMaxElements));
          m_pMenu->insertAction(actions.isEmpty() ? nullptr : actions[0], pAction);

          for (auto pAct : actions)
          {
            pAct->setEnabled(false);
          }
        }

        return;
      }
    }

    if (m_bNoMoreElementsAllowed)
    {
      for (auto pAct : actions)
      {
        pAct->setEnabled(true);
      }

      m_bNoMoreElementsAllowed = false;
      delete m_pMenu->actions()[0]; // remove the dummy action
    }
  }

  if (m_bPreventDuplicates)
  {
    xiiSet<const xiiRTTI*> UsedTypes;

    for (auto& item : m_Items)
    {
      xiiInt32 iCount = 0;
      m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).AssertSuccess();

      for (xiiInt32 i = 0; i < iCount; ++i)
      {
        xiiUuid guid = m_pObjectAccessor->Get<xiiUuid>(item.m_pObject, m_pProp, i);

        if (guid.IsValid())
        {
          UsedTypes.Insert(m_pObjectAccessor->GetObject(guid)->GetType());
        }
      }

      QList<QAction*> actions = m_pMenu->actions();
      for (auto pAct : actions)
      {
        const xiiRTTI* pRtti = static_cast<const xiiRTTI*>(pAct->property("type").value<void*>());

        pAct->setEnabled(!UsedTypes.Contains(pRtti));
      }
    }
  }
}

void xiiQtAddSubElementButton::on_Button_clicked()
{
  auto pProp = GetProperty();

  if (!pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
  {
    OnAction(pProp->GetSpecificType());
  }
}

void xiiQtAddSubElementButton::OnMenuAction()
{
  const xiiRTTI* pRtti = static_cast<const xiiRTTI*>(sender()->property("type").value<void*>());

  OnAction(pRtti);
}

void xiiQtAddSubElementButton::OnAction(const xiiRTTI* pRtti)
{
  XII_ASSERT_DEV(pRtti != nullptr, "user data retrieval failed");
  xiiVariant index = (xiiInt32)-1;

  if (m_pProp->GetCategory() == xiiPropertyCategory::Map)
  {
    QString text;
    bool    bOk = false;
    while (!bOk)
    {
      text = QInputDialog::getText(this, "Set map key for new element", "Key:", QLineEdit::Normal, text, &bOk);
      if (!bOk)
        return;

      index = text.toUtf8().data();
      for (auto& item : m_Items)
      {
        xiiVariant value;
        xiiStatus  res = m_pObjectAccessor->GetValue(item.m_pObject, m_pProp, value, index);
        if (res.m_Result.Succeeded())
        {
          bOk = false;
          break;
        }
      }
      if (!bOk)
      {
        xiiQtUiServices::GetSingleton()->MessageBoxInformation("The selected key is already used in the selection.");
      }
    }
  }

  m_pObjectAccessor->StartTransaction("Add Element");

  xiiStatus  res;
  const bool bIsValueType = xiiReflectionUtils::IsValueType(m_pProp);
  if (bIsValueType)
  {
    for (auto& item : m_Items)
    {
      res = m_pObjectAccessor->InsertValue(item.m_pObject, m_pProp, xiiReflectionUtils::GetDefaultValue(GetProperty(), index), index);
      if (res.m_Result.Failed())
        break;
    }
  }
  else if (GetProperty()->GetFlags().IsSet(xiiPropertyFlags::Class))
  {
    for (auto& item : m_Items)
    {
      xiiUuid guid;
      res = m_pObjectAccessor->AddObject(item.m_pObject, m_pProp, index, pRtti, guid);
      if (res.m_Result.Failed())
        break;

      xiiHybridArray<xiiPropertySelection, 1> selection;
      selection.PushBack({m_pObjectAccessor->GetObject(guid), xiiVariant()});
      xiiDefaultObjectState defaultState(m_pObjectAccessor, selection);
      defaultState.RevertObject().AssertSuccess();
    }
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}
