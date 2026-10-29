/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QTreeWidgetItemIterator>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class xiiPreferencesDocument : public xiiDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPreferencesDocument, xiiDocument);

public:
  xiiPreferencesDocument() : xiiDocument("<settings>", XII_DEFAULT_NEW(xiiDocumentObjectManager)) {}
  xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPreferencesDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  constexpr int s_iBindingRole = Qt::UserRole;
  constexpr int s_iSearchRole  = Qt::UserRole + 1;

  QTreeWidgetItem* FindOrAddCategory(QTreeWidget* pTree, QTreeWidgetItem* pParent, const QString& name)
  {
    const int count = pParent ? pParent->childCount() : pTree->topLevelItemCount();
    for (int i = 0; i < count; ++i)
    {
      auto* pChild = pParent ? pParent->child(i) : pTree->topLevelItem(i);
      if (pChild->text(0) == name)
        return pChild;
    }
    return pParent ? new QTreeWidgetItem(pParent, {name}) : new QTreeWidgetItem(pTree, {name});
  }
} // namespace

xiiQtPreferencesDlg::xiiQtPreferencesDlg(QWidget* pParent, const xiiDocument* pContext) : QDialog(pParent)
{
  setupUi(this);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  Tree->setMinimumWidth(240);
  m_pDocument = XII_DEFAULT_NEW(xiiPreferencesDocument);
  Properties->SetDocument(m_pDocument);

  xiiDynamicArray<xiiPreferences::Settings> settings;
  xiiPreferences::GatherSettings(settings, pContext);
  for (const auto& entry : settings)
  {
    const auto*                                    pType = entry.m_pSettings->GetDynamicRTTI();
    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    bool    bVisible = false;
    QString search;
    for (const auto* pProperty : properties)
    {
      if (pProperty->GetCategory() == xiiPropertyCategory::Constant || pProperty->GetAttributeByType<xiiHiddenAttribute>())
        continue;
      bVisible = true;
      search += QString::fromUtf8(pProperty->GetPropertyName().GetStartPointer(), pProperty->GetPropertyName().GetElementCount()) + QLatin1Char(' ');
    }
    if (!bVisible)
      continue;

    const QString scope       = entry.m_Domain == xiiPreferences::Domain::Application ? tr("Application") : entry.m_Domain == xiiPreferences::Domain::Project ? tr("Project") :
                                                                                                                                                                tr("Document");
    auto*         pParentItem = FindOrAddCategory(Tree, nullptr, scope);
    if (entry.m_pDocument)
    {
      const auto path = entry.m_pDocument->GetDocumentPath();
      pParentItem     = FindOrAddCategory(Tree, pParentItem, QString::fromUtf8(path.GetStartPointer(), path.GetElementCount()));
    }
    if (const auto* pCategory = pType->GetAttributeByType<xiiCategoryAttribute>())
    {
      const auto category = pCategory->GetCategory();
      const auto parts    = QString::fromUtf8(category.GetStartPointer(), category.GetElementCount()).split(QLatin1Char('/'), Qt::SkipEmptyParts);
      for (const auto& part : parts)
        pParentItem = FindOrAddCategory(Tree, pParentItem, part);
    }
    QString name = pType->IsDerivedFrom<xiiPreferences>() ? xiiMakeQString(entry.m_sName) : xiiMakeQString(xiiTranslate(pType->GetTypeName()));
    if (const auto* pTitle = pType->GetAttributeByType<xiiTitleAttribute>())
      name = xiiMakeQString(xiiTranslate(pTitle->GetTitle()));
    auto*      pItem = new QTreeWidgetItem(pParentItem, {name});
    const auto guid  = NativeToObject(entry.m_pSettings);
    m_Bindings.Insert(guid, entry.m_pSettings);
    xiiStringBuilder guidString;
    pItem->setData(0, s_iBindingRole, QString::fromUtf8(xiiConversionUtils::ToString(guid, guidString).GetData()));
    pItem->setData(0, s_iSearchRole, search + xiiMakeQString(pType->GetTypeName()));
    pItem->setToolTip(0, tr("Edit reflected settings. Reset and Cancel remain local until Apply or OK."));
  }
  Tree->sortItems(0, Qt::AscendingOrder);
  Tree->expandAll();
  connect(Search, &QLineEdit::textChanged, this, &xiiQtPreferencesDlg::FilterNavigation);
  connect(Tree, &QTreeWidget::itemSelectionChanged, this, &xiiQtPreferencesDlg::SelectionChanged);
  if (m_Bindings.IsEmpty())
    Status->setText(tr("No settings are available in this context."));
  else
  {
    QTreeWidgetItemIterator it(Tree);
    while (*it && !(*it)->data(0, s_iBindingRole).isValid())
      ++it;
    if (*it)
      Tree->setCurrentItem(*it);
  }
  ButtonApply->setEnabled(!m_Bindings.IsEmpty());
  ButtonReset->setEnabled(!m_Bindings.IsEmpty());

  xiiPreferences::s_SettingsInvalidatedEvent.AddEventHandler([this, pContext](const xiiDocument* pDocument) {
    if (pDocument == nullptr || pDocument == pContext)
    {
      ReleaseObjects();
      reject();
    }
  },
                                                             m_InvalidatedSubscription);
  xiiPlugin::Events().AddEventHandler([this](const xiiPluginEvent& event) {
    if (event.m_EventType == xiiPluginEvent::BeforePluginChanges)
    {
      ReleaseObjects();
      reject();
    }
  },
                                      m_PluginSubscription);
}

xiiQtPreferencesDlg::~xiiQtPreferencesDlg()
{
  ReleaseObjects();
}

void xiiQtPreferencesDlg::ReleaseObjects()
{
  if (m_pDocument == nullptr)
    return;
  // Type widgets unsubscribe through the grid's document during destruction.
  // Destroy them while the draft and its RTTI are still alive, including deferred child widgets.
  delete Properties;
  Properties = nullptr;
  m_Bindings.Clear();
  Tree->clear();
  ButtonApply->setEnabled(false);
  ButtonReset->setEnabled(false);
  XII_DEFAULT_DELETE(m_pDocument);
}

xiiUuid xiiQtPreferencesDlg::NativeToObject(xiiSettings* pSettings)
{
  const auto*             pType = pSettings->GetDynamicRTTI();
  xiiAbstractObjectGraph graph;
  xiiRttiConverterContext context;
  xiiRttiConverterWriter  writer(&graph, &context, true, true);
  context.RegisterObject(xiiUuid::MakeUuid(), pType, pSettings);
  auto* pNode   = writer.AddObjectToGraph(pType, pSettings, "root");
  auto* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, nullptr, "Children", -1);
  xiiDocumentObjectConverterReader reader(&graph, m_pDocument->GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  reader.ApplyPropertiesToObject(pNode, pObject);
  return pObject->GetGuid();
}

void xiiQtPreferencesDlg::SelectionChanged()
{
  if (m_pDocument == nullptr)
    return;
  auto* pItem = Tree->currentItem();
  if (pItem == nullptr || !pItem->data(0, s_iBindingRole).isValid())
  {
    m_pDocument->GetSelectionManager()->Clear();
    ButtonReset->setEnabled(false);
    return;
  }
  const auto guid    = xiiConversionUtils::ConvertStringToUuid(pItem->data(0, s_iBindingRole).toString().toUtf8().constData());
  auto*      pObject = m_pDocument->GetObjectManager()->GetObject(guid);
  m_pDocument->GetSelectionManager()->SetSelection(pObject);
  ButtonReset->setEnabled(pObject != nullptr);
  bool bRestart = false;
  if (pObject)
  {
    const auto* pType = pObject->GetType();
    bRestart          = pType->GetAttributeByType<xiiSettingsRestartAttribute>() != nullptr;
    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (const auto* pProperty : properties)
      bRestart |= pProperty->GetAttributeByType<xiiSettingsRestartAttribute>() != nullptr;
  }
  Status->setText(bRestart ? tr("Some changes require restart or subsystem reinitialization.") : QString());
}

void xiiQtPreferencesDlg::FilterNavigation(const QString& text)
{
  if (m_pDocument == nullptr)
    return;
  const auto terms = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
  for (QTreeWidgetItemIterator it(Tree); *it; ++it)
  {
    auto* pItem = *it;
    if (!pItem->data(0, s_iBindingRole).isValid())
    {
      pItem->setHidden(true);
      continue;
    }
    QString searchable = pItem->data(0, s_iSearchRole).toString();
    for (auto* pAncestor = pItem; pAncestor; pAncestor = pAncestor->parent())
      searchable += QLatin1Char(' ') + pAncestor->text(0);
    bool bMatches = true;
    for (const auto& term : terms)
      bMatches &= searchable.contains(term, Qt::CaseInsensitive);
    pItem->setHidden(!bMatches);
  }
  for (QTreeWidgetItemIterator it(Tree); *it; ++it)
    if ((*it)->data(0, s_iBindingRole).isValid() && !(*it)->isHidden())
      for (auto* pAncestor = (*it)->parent(); pAncestor; pAncestor = pAncestor->parent())
        pAncestor->setHidden(false);
  Tree->clearSelection();
  m_pDocument->GetSelectionManager()->Clear();
  ButtonReset->setEnabled(false);
}

xiiResult xiiQtPreferencesDlg::ApplyAllChanges()
{
  if (m_pDocument == nullptr)
    return XII_FAILURE;
  xiiMap<xiiSettings*, xiiSettings*> candidates;
  XII_SCOPE_EXIT(for (auto& entry : candidates) entry.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(entry.Value()));
  for (const auto& binding : m_Bindings)
  {
    auto* pCandidate = xiiReflectionSerializer::Clone(binding.Value());
    candidates.Insert(binding.Value(), pCandidate);
    xiiAbstractObjectGraph graph;
    auto                   filter = [](const xiiDocumentObject*, const xiiAbstractProperty* pProperty) {
      return !pProperty->GetFlags().IsSet(xiiPropertyFlags::ReadOnly);
    };
    xiiDocumentObjectConverterWriter writer(&graph, m_pDocument->GetObjectManager(), filter);
    writer.AddObjectToGraph(m_pDocument->GetObjectManager()->GetObject(binding.Key()), "root");
    if (pCandidate->ReadFromGraph(graph).Failed())
    {
      const auto typeName = pCandidate->GetDynamicRTTI()->GetTypeName();
      Status->setText(tr("Invalid settings in %1. Changes have not been applied.").arg(QString::fromUtf8(typeName.GetStartPointer(), typeName.GetElementCount())));
      return XII_FAILURE;
    }
  }
  // Validate every category before saving or notifying any consumer. Failed saves never mutate live objects.
  for (const auto& entry : candidates)
  {
    if (!entry.Key()->HasSameValues(*entry.Value()) && xiiPreferences::SaveSettings(entry.Key(), entry.Value()).Failed())
    {
      Status->setText(tr("Unable to save settings. Live values remain unchanged; check the settings file and log."));
      return XII_FAILURE;
    }
  }
  for (const auto& entry : candidates)
    XII_SUCCEED_OR_RETURN(entry.Key()->Apply(*entry.Value()));
  Status->setText(tr("Settings applied and saved. Changes marked for restart require restart or reinitialization."));
  return XII_SUCCESS;
}

void xiiQtPreferencesDlg::on_ButtonApply_clicked()
{
  ApplyAllChanges().IgnoreResult();
}

void xiiQtPreferencesDlg::on_ButtonOk_clicked()
{
  if (ApplyAllChanges().Succeeded())
    accept();
}

void xiiQtPreferencesDlg::on_ButtonReset_clicked()
{
  if (m_pDocument == nullptr)
    return;
  const auto* pObject = m_pDocument->GetSelectionManager()->GetCurrentObject();
  if (pObject == nullptr)
    return;
  auto* pDefaults = xiiSettings::Create(pObject->GetType());
  if (pDefaults == nullptr)
    return;
  XII_SCOPE_EXIT(pDefaults->GetDynamicRTTI()->GetAllocator()->Deallocate(pDefaults));
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;
  xiiRttiConverterWriter  writer(&graph, &context, false, true);
  context.RegisterObject(xiiUuid::MakeUuid(), pDefaults->GetDynamicRTTI(), pDefaults);
  auto*                                          pNode = writer.AddObjectToGraph(pDefaults->GetDynamicRTTI(), pDefaults, "root");
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pDefaults->GetDynamicRTTI()->GetAllProperties(properties);
  for (const auto* pProperty : properties)
    if (pProperty->GetAttributeByType<xiiHiddenAttribute>() || pProperty->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
      pNode->RemoveProperty(pProperty->GetPropertyName());
  xiiDocumentObjectConverterReader reader(&graph, m_pDocument->GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  reader.ApplyPropertiesToObject(pNode, const_cast<xiiDocumentObject*>(pObject));
  Properties->ClearSelection();
  m_pDocument->GetSelectionManager()->Clear();
  m_pDocument->GetSelectionManager()->SetSelection(pObject);
}
