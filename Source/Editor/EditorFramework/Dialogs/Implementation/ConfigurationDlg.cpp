/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/AssetProfilesPage.moc.h>
#include <EditorFramework/Dialogs/ConfigurationDlg.moc.h>
#include <EditorFramework/Dialogs/DataDirsPage.moc.h>
#include <EditorFramework/Dialogs/InputConfigPage.moc.h>
#include <EditorFramework/Dialogs/PluginSelectionWidget.moc.h>
#include <EditorFramework/Dialogs/TagsPage.moc.h>
#include <EditorFramework/Dialogs/WindowConfigPage.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/Dialogs/ShortcutEditorPage.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class xiiEditorSettingsDocument : public xiiDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorSettingsDocument, xiiDocument);

public:
  xiiEditorSettingsDocument() : xiiDocument("<settings>", XII_DEFAULT_NEW(xiiDocumentObjectManager)) {}
  xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorSettingsDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  constexpr int s_iBindingRole = Qt::UserRole;
  constexpr int s_iSearchRole  = Qt::UserRole + 1;
  constexpr int s_iPageRole    = Qt::UserRole + 2;
  enum Page
  {
    Input,
    DataDirectories,
    Window,
    Tags,
    Profiles,
    Shortcuts,
    Plugins
  };
  QPointer<xiiQtConfigurationDlg> s_pWindow;

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

xiiQtConfigurationDlg::xiiQtConfigurationDlg(QWidget* pParent, const xiiDocument* pContext, Section section) : QDialog(pParent), m_pContext(pContext)
{
  setupUi(this);
  QFont headingFont = Heading->font();
  headingFont.setPointSize(headingFont.pointSize() + 4);
  headingFont.setBold(true);
  Heading->setFont(headingFont);
  auto* pProject = new QTreeWidgetItem(Tree, {tr("Project Settings")});
  auto* pEditor  = new QTreeWidgetItem(Tree, {tr("Editor Preferences")});
  auto* pPlugins = new QTreeWidgetItem(Tree, {tr("Plugins")});
  pPlugins->setData(0, s_iPageRole, Plugins);
  QString pluginSearch = tr("Plugins extensions editor runtime engine bundles dependencies");
  for (const auto& bundle : xiiQtEditorApp::GetSingleton()->GetPluginBundles().m_Plugins)
    pluginSearch += QLatin1Char(' ') + xiiMakeQString(bundle.Key()) + QLatin1Char(' ') + xiiMakeQString(bundle.Value().m_sDisplayName) + QLatin1Char(' ') + xiiMakeQString(bundle.Value().m_sDescription);
  pPlugins->setData(0, s_iSearchRole, pluginSearch);
  auto addPage = [&](QTreeWidgetItem* pParentItem, const QString& category, const QString& name, int page) {
    auto* pCategory = FindOrAddCategory(Tree, pParentItem, category);
    auto* pItem     = new QTreeWidgetItem(pCategory, {name});
    pItem->setData(0, s_iPageRole, page);
    pItem->setData(0, s_iSearchRole, name + QLatin1Char(' ') + category);
  };
  if (xiiToolsProject::IsProjectOpen())
  {
    addPage(pProject, tr("Input"), tr("Input Actions"), Input);
    addPage(pProject, tr("Assets"), tr("Data Directories"), DataDirectories);
    addPage(pProject, tr("Platform"), tr("Window Configuration"), Window);
    addPage(pProject, tr("General"), tr("Tags"), Tags);
    addPage(pProject, tr("Platform"), tr("Platform and Asset Profiles"), Profiles);
  }
  addPage(pEditor, tr("Interface"), tr("Keyboard Shortcuts"), Shortcuts);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  Tree->setMinimumWidth(240);
  m_pDocument = XII_DEFAULT_NEW(xiiEditorSettingsDocument);
  Properties->SetDocument(m_pDocument);

  xiiDynamicArray<xiiEditorSettings::Settings> settings;
  xiiEditorSettings::GatherSettings(settings, pContext);
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

    // User settings keep their own application/project/document scope. They never become shared project configuration.
    const bool bUserSettings = pType->IsDerivedFrom<xiiEditorSettings>() || entry.m_Domain != xiiEditorSettings::Domain::Project;
    auto*      pParentItem   = bUserSettings ? pEditor : pProject;
    if (entry.m_Domain == xiiEditorSettings::Domain::Project && bUserSettings)
      pParentItem = FindOrAddCategory(Tree, pParentItem, tr("Project (This User)"));
    if (entry.m_Domain == xiiEditorSettings::Domain::Document)
      pParentItem = FindOrAddCategory(Tree, pParentItem, tr("Document"));
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
    QString name = pType->IsDerivedFrom<xiiEditorSettings>() ? xiiMakeQString(entry.m_sName) : xiiMakeQString(xiiTranslate(pType->GetTypeName()));
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
  for (int i = 0; i < Tree->topLevelItemCount(); ++i)
    Tree->topLevelItem(i)->sortChildren(0, Qt::AscendingOrder);
  Tree->expandAll();
  connect(Search, &QLineEdit::textChanged, this, &xiiQtConfigurationDlg::FilterNavigation);
  connect(Tree, &QTreeWidget::itemSelectionChanged, this, &xiiQtConfigurationDlg::SelectionChanged);
  SelectSection(section);
  ButtonApply->setEnabled(true);

  xiiToolsProject::s_Events.AddEventHandler([this](const xiiToolsProjectEvent& event) {
    if (event.m_Type == xiiToolsProjectEvent::Type::ProjectClosing || event.m_Type == xiiToolsProjectEvent::Type::ProjectOpened)
    {
      ReleaseObjects();
      reject();
    }
  },
                                            m_ProjectSubscription);

  xiiEditorSettings::s_SettingsInvalidatedEvent.AddEventHandler([this, pContext](const xiiDocument* pDocument) {
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

xiiQtConfigurationDlg::~xiiQtConfigurationDlg()
{
  ReleaseObjects();
}

void xiiQtConfigurationDlg::ReleaseObjects()
{
  if (m_pDocument == nullptr)
    return;
  // Type widgets unsubscribe through the grid's document during destruction.
  // Destroy them while the draft and its RTTI are still alive, including deferred child widgets.
  delete Properties;
  Properties = nullptr;
  // Pages contain borrowed action descriptors and profile RTTI; release before plugin unload.
  for (const auto& entry : m_Pages)
    delete entry.Value();
  m_Pages.Clear();
  m_ModifiedPages.clear();
  m_Bindings.Clear();
  Tree->clear();
  ButtonApply->setEnabled(false);
  ButtonReset->setEnabled(false);
  XII_DEFAULT_DELETE(m_pDocument);
}

xiiUuid xiiQtConfigurationDlg::NativeToObject(xiiSettings* pSettings)
{
  const auto*             pType = pSettings->GetDynamicRTTI();
  xiiAbstractObjectGraph  graph;
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

void xiiQtConfigurationDlg::SelectionChanged()
{
  if (m_pDocument == nullptr)
    return;
  auto* pItem = Tree->currentItem();
  Pages->setCurrentWidget(EmptyPage);
  Heading->setText(pItem ? pItem->text(0) : tr("Configuration"));
  Description->setText(QString());
  Status->clear();
  ButtonReset->setText(tr("Reset Category"));
  ButtonReset->setEnabled(false);
  if (pItem && pItem->data(0, s_iPageRole).isValid())
  {
    m_pDocument->GetSelectionManager()->Clear();
    const int page = pItem->data(0, s_iPageRole).toInt();
    Pages->setCurrentWidget(CreatePage(page));
    ButtonReset->setText(tr("Revert Page"));
    ButtonReset->setEnabled(true);
    Description->setText(page == Plugins ? tr("Project requirements select bundles for editor, engine process, and runtime. Availability does not mean a module is currently loaded. Apply saves requirements; restart the editor to activate changes.") : tr("Changes remain staged until Apply or OK. Cancel discards edits since the last Apply."));
    if (page == DataDirectories)
      Status->setText(tr("Changing data directories requires reloading the project."));
    if (page == Window)
      Status->setText(tr("Project defaults and explicit per-user Player overrides are stored separately. Changes apply to new runtime windows."));
    if (page == Plugins)
      Status->setText(xiiToolsProject::IsProjectOpen() ? tr("Changes require editor restart. Runtime module copying only enables reloads supported by that plugin.") : tr("Open a project to change plugin requirements. Available bundles are read-only."));
    return;
  }
  EmptyText->setText(xiiToolsProject::IsProjectOpen() ? tr("Select a category on the left.") : tr("Open a project to configure project settings."));
  if (pItem == nullptr || !pItem->data(0, s_iBindingRole).isValid())
  {
    m_pDocument->GetSelectionManager()->Clear();
    ButtonReset->setEnabled(false);
    return;
  }
  const auto guid    = xiiConversionUtils::ConvertStringToUuid(pItem->data(0, s_iBindingRole).toString().toUtf8().constData());
  auto*      pObject = m_pDocument->GetObjectManager()->GetObject(guid);
  Pages->setCurrentWidget(ReflectedPage);
  m_pDocument->GetSelectionManager()->SetSelection(pObject);
  ButtonReset->setEnabled(pObject != nullptr);
  bool bRestart = false;
  if (pObject)
  {
    const auto* pType   = pObject->GetType();
    const auto* pScope  = pType->GetAttributeByType<xiiSettingsScopeAttribute>();
    const bool  bShared = !pType->IsDerivedFrom<xiiEditorSettings>() && pScope && pScope->GetScope() == xiiSettingsScope::Project;
    Description->setText(bShared ? tr("Shared settings stored with the current project. Apply saves changes for this project.") : tr("Preferences for this user. Project and document preferences remain specific to their labeled context."));
    bRestart = pType->GetAttributeByType<xiiSettingsRestartAttribute>() != nullptr;
    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (const auto* pProperty : properties)
      bRestart |= pProperty->GetAttributeByType<xiiSettingsRestartAttribute>() != nullptr;
  }
  Status->setText(bRestart ? tr("Some changes require restart or subsystem reinitialization.") : QString());
}

void xiiQtConfigurationDlg::FilterNavigation(const QString& text)
{
  if (m_pDocument == nullptr)
    return;
  const auto terms = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
  for (QTreeWidgetItemIterator it(Tree); *it; ++it)
  {
    auto* pItem = *it;
    if (!pItem->data(0, s_iBindingRole).isValid() && !pItem->data(0, s_iPageRole).isValid())
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
    if (((*it)->data(0, s_iBindingRole).isValid() || (*it)->data(0, s_iPageRole).isValid()) && !(*it)->isHidden())
      for (auto* pAncestor = (*it)->parent(); pAncestor; pAncestor = pAncestor->parent())
        pAncestor->setHidden(false);
  if (auto it = m_Pages.Find(Plugins); it.IsValid())
    static_cast<xiiQtPluginSelectionWidget*>(it.Value())->Filter(text);
  Tree->clearSelection();
  m_pDocument->GetSelectionManager()->Clear();
  ButtonReset->setEnabled(false);
  if (Pages->currentWidget() != m_Pages.GetValueOrDefault(Plugins, nullptr))
  {
    Pages->setCurrentWidget(EmptyPage);
    EmptyText->setText(text.isEmpty() ? tr("Select a category on the left.") : tr("Select a matching category on the left. No entries means no settings match your search."));
  }
}

xiiResult xiiQtConfigurationDlg::ApplyAllChanges()
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
  if (auto it = m_Pages.Find(Plugins); it.IsValid() && m_ModifiedPages.contains(it.Value()))
  {
    xiiStringBuilder reason;
    if (static_cast<xiiQtPluginSelectionWidget*>(it.Value())->ValidateSelection(reason).Failed())
    {
      Status->setText(xiiMakeQString(reason));
      return XII_FAILURE;
    }
  }
  // Validate every category before saving or notifying any consumer. Failed saves never mutate live objects.
  for (const auto& entry : candidates)
  {
    if (!entry.Key()->HasSameValues(*entry.Value()) && xiiEditorSettings::SaveSettings(entry.Key(), entry.Value()).Failed())
    {
      Status->setText(tr("Unable to save settings. Live values remain unchanged; check the settings file and log."));
      return XII_FAILURE;
    }
  }
  if (ApplyPages().Failed())
  {
    Status->setText(tr("Unable to save this configuration page. Check the log and file permissions. Earlier successful page saves remain committed."));
    return XII_FAILURE;
  }
  for (const auto& entry : candidates)
    XII_SUCCEED_OR_RETURN(entry.Key()->Apply(*entry.Value()));
  Status->setText(tr("Settings applied and saved. Changes marked for restart require restart or reinitialization."));
  return XII_SUCCESS;
}

void xiiQtConfigurationDlg::on_ButtonApply_clicked()
{
  ApplyAllChanges().IgnoreResult();
}

void xiiQtConfigurationDlg::on_ButtonOk_clicked()
{
  if (ApplyAllChanges().Succeeded())
    accept();
}

void xiiQtConfigurationDlg::on_ButtonReset_clicked()
{
  if (m_pDocument == nullptr)
    return;
  if (auto* pItem = Tree->currentItem(); pItem && pItem->data(0, s_iPageRole).isValid())
  {
    const int page = pItem->data(0, s_iPageRole).toInt();
    if (auto it = m_Pages.Find(page); it.IsValid())
    {
      auto* pWidget = it.Value();
      m_ModifiedPages.remove(pWidget);
      m_Pages.Remove(it);
      delete pWidget;
    }
    SelectionChanged();
    return;
  }
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

void xiiQtConfigurationDlg::Show(Section section, const xiiDocument* pContext, xiiStringView sPage)
{
  if (pContext == nullptr)
    if (auto* pContainer = xiiQtContainerWindow::GetContainerWindow())
      if (auto* pWindow = pContainer->GetFocusedDocumentWindow())
        pContext = pWindow->GetDocument();
  // Keep one modeless window; navigating between sections preserves pending edits.
  if (s_pWindow && s_pWindow->m_pDocument == nullptr)
    delete s_pWindow;
  if (!s_pWindow)
  {
    s_pWindow = new xiiQtConfigurationDlg(nullptr, pContext, section);
    s_pWindow->setAttribute(Qt::WA_DeleteOnClose);
  }
  s_pWindow->SelectSection(section, sPage);
  s_pWindow->show();
  s_pWindow->raise();
  s_pWindow->activateWindow();
}

void xiiQtConfigurationDlg::SelectSection(Section section, xiiStringView sPage)
{
  if (m_pDocument == nullptr)
    return;
  Search->clear();
  auto* pItem = Tree->topLevelItem(static_cast<int>(section));
  if (!sPage.IsEmpty())
  {
    for (QTreeWidgetItemIterator it(pItem); *it; ++it)
    {
      if ((*it)->text(0) == xiiMakeQString(sPage))
      {
        pItem = *it;
        break;
      }
    }
  }
  else
  {
    while (pItem && pItem->childCount() > 0)
      pItem = pItem->child(0);
  }
  Tree->setCurrentItem(pItem);
  SelectionChanged();
}

QWidget* xiiQtConfigurationDlg::CreatePage(int iPage)
{
  if (auto it = m_Pages.Find(iPage); it.IsValid())
    return it.Value();
  QWidget* pPage = nullptr;
  switch (iPage)
  {
    case Input:
      pPage = new xiiQtInputConfigPage(Pages);
      connect(static_cast<xiiQtInputConfigPage*>(pPage), &xiiQtInputConfigPage::Changed, this, [this, pPage]() { m_ModifiedPages.insert(pPage); });
      break;
    case DataDirectories:
      pPage = new xiiQtDataDirsPage(Pages);
      break;
    case Window:
      pPage = new xiiQtWindowConfigPage(Pages);
      break;
    case Tags:
      pPage = new xiiQtTagsPage(Pages);
      break;
    case Profiles:
      pPage = new xiiQtAssetProfilesPage(Pages, false);
      break;
    case Shortcuts:
      pPage = new xiiQtShortcutEditorPage(Pages);
      break;
    case Plugins:
    {
      auto* pEditor = xiiQtEditorApp::GetSingleton();
      pEditor->DetectAvailablePluginBundles(xiiOSFile::GetApplicationDirectory());
      if (xiiToolsProject::IsProjectOpen())
      {
        xiiCppSettings cpp;
        if (cpp.Load().Succeeded())
          pEditor->DetectAvailablePluginBundles(xiiCppProject::GetPluginSourceDir(cpp));
      }
      m_Plugins = pEditor->GetPluginBundles();
      if (!xiiToolsProject::IsProjectOpen())
        for (auto& bundle : m_Plugins.m_Plugins)
        {
          bundle.Value().m_bSelected = false;
          bundle.Value().m_bLoadCopy = false;
        }
      auto* pSelection = new xiiQtPluginSelectionWidget(Pages);
      pSelection->SetPluginSet(&m_Plugins);
      pSelection->SetReadOnly(!xiiToolsProject::IsProjectOpen());
      pPage = pSelection;
      break;
    }
    default:
      return EmptyPage;
  }
  Pages->addWidget(pPage);
  m_Pages.Insert(iPage, pPage);
  TrackPageChanges(pPage);
  return pPage;
}

void xiiQtConfigurationDlg::TrackPageChanges(QWidget* pPage)
{
  auto changed = [this, pPage]() {
    m_ModifiedPages.insert(pPage);
  };
  for (auto* pButton : pPage->findChildren<QAbstractButton*>())
    connect(pButton, &QAbstractButton::clicked, this, changed);
  for (auto* pEdit : pPage->findChildren<QLineEdit*>())
    connect(pEdit, &QLineEdit::textEdited, this, changed);
  for (auto* pCombo : pPage->findChildren<QComboBox*>())
    if (pCombo->objectName() != QStringLiteral("PluginCategory"))
      connect(pCombo, &QComboBox::activated, this, changed);
  for (auto* pSpin : pPage->findChildren<QSpinBox*>())
    connect(pSpin, &QSpinBox::valueChanged, this, changed);
  for (auto* pTree : pPage->findChildren<QTreeWidget*>())
    connect(pTree, &QTreeWidget::itemChanged, this, changed);
  for (auto* pList : pPage->findChildren<QListWidget*>())
    connect(pList, &QListWidget::itemChanged, this, changed);
}

xiiResult xiiQtConfigurationDlg::ApplyPages()
{
  bool bProjectChanged = false;
  for (const auto& entry : m_Pages)
  {
    const int page    = entry.Key();
    auto*     pWidget = entry.Value();
    if (!m_ModifiedPages.contains(pWidget) && !(page == Profiles && static_cast<xiiQtAssetProfilesPage*>(pWidget)->HasChanges()))
      continue;
    switch (page)
    {
      case Input:
        XII_SUCCEED_OR_RETURN(static_cast<xiiQtInputConfigPage*>(pWidget)->ApplyChanges());
        break;
      case DataDirectories:
        XII_SUCCEED_OR_RETURN(static_cast<xiiQtDataDirsPage*>(pWidget)->ApplyChanges());
        break;
      case Window:
        XII_SUCCEED_OR_RETURN(static_cast<xiiQtWindowConfigPage*>(pWidget)->ApplyChanges());
        break;
      case Tags:
        XII_SUCCEED_OR_RETURN(static_cast<xiiQtTagsPage*>(pWidget)->ApplyChanges());
        break;
      case Profiles:
        XII_SUCCEED_OR_RETURN(static_cast<xiiQtAssetProfilesPage*>(pWidget)->ApplyChanges());
        break;
      case Shortcuts:
        XII_SUCCEED_OR_RETURN(static_cast<xiiQtShortcutEditorPage*>(pWidget)->ApplyChanges());
        break;
      case Plugins:
      {
        if (!xiiToolsProject::IsProjectOpen())
          return XII_FAILURE;
        auto* pSelection = static_cast<xiiQtPluginSelectionWidget*>(pWidget);
        pSelection->SyncStateToSet();
        auto* pEditor = xiiQtEditorApp::GetSingleton();
        // Discovery may have refreshed metadata while this window was open. Commit only requirements.
        auto selection = pEditor->GetPluginBundles();
        for (const auto& bundle : m_Plugins.m_Plugins)
        {
          auto it = selection.m_Plugins.Find(bundle.Key());
          if (!it.IsValid())
            return XII_FAILURE;
          it.Value().m_bSelected = bundle.Value().m_bSelected;
          it.Value().m_bLoadCopy = bundle.Value().m_bLoadCopy;
        }
        if (!pEditor->GetPluginBundles().IsStateEqual(selection))
        {
          xiiStringBuilder      path(xiiToolsProject::GetSingleton()->GetProjectDirectory(), "/", xiiToolsProject::GetSingleton()->GetManifest().m_sPluginSelection);
          xiiDeferredFileWriter file;
          file.SetOutput(path);
          xiiOpenDdlWriter ddl;
          ddl.SetOutputStream(&file);
          selection.WriteStateToDDL(ddl);
          XII_SUCCEED_OR_RETURN(file.Close());
          pEditor->GetPluginBundles() = selection;
          pEditor->AddRestartRequiredReason("The set of active plugins has changed.", false);
        }
        break;
      }
    }
    bProjectChanged |= page == Input || page == Tags || page == Profiles;
    m_ModifiedPages.remove(pWidget);
  }
  if (bProjectChanged)
    xiiToolsProject::BroadcastConfigChanged();
  return XII_SUCCESS;
}
