/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/PluginSelectionWidget.moc.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <QSignalBlocker>

xiiQtPluginSelectionWidget::xiiQtPluginSelectionWidget(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);
  m_pCategory = new QComboBox(this);
  m_pCategory->setObjectName(QStringLiteral("PluginCategory"));
  m_pCategory->addItems({tr("All Plugins"), tr("Editor"), tr("Runtime / Engine"), tr("Editor and Runtime"), tr("Requested"), tr("Missing")});
  delete dummy;
  dummy = nullptr;
  gridLayout->addWidget(m_pCategory, 0, 1);
  connect(m_pCategory, &QComboBox::currentIndexChanged, this, [this]() { Filter(m_Filter); });
  connect(LoadCopy, &QCheckBox::toggled, this, [this](bool checked) {
    if (auto* pItem = PluginsList->currentItem(); pItem && !m_bReadOnly)
      m_States[PluginsList->row(pItem)].m_bLoadCopy = checked;
  });
}

xiiQtPluginSelectionWidget::~xiiQtPluginSelectionWidget() = default;

void xiiQtPluginSelectionWidget::SetPluginSet(xiiPluginBundleSet* pPluginSet)
{
  m_pPluginSet = pPluginSet;
  const QSignalBlocker blocker(PluginsList);
  PluginsList->clear();
  m_States.Clear();

  xiiSet<xiiString> templates;
  xiiStringBuilder  tmp;

  for (auto& plugin : m_pPluginSet->m_Plugins)
  {
    auto& pi = plugin.Value();

    for (const auto& tmp : pi.m_EnabledInTemplates)
    {
      templates.Insert(tmp);
    }

    auto& state       = m_States.ExpandAndGetRef();
    state.m_sID       = plugin.Key();
    state.m_pInfo     = &pi;
    state.m_bSelected = pi.m_bSelected || pi.m_bMandatory;
    state.m_bLoadCopy = pi.m_bLoadCopy;

    tmp = pi.m_sDisplayName;

    QListWidgetItem* pItem = new QListWidgetItem();

    if (pi.m_bMissing)
    {
      tmp.Append(" (missing)");
      pItem->setBackground(QColor::fromRgb(200, 0, 0));
    }

    pItem->setText(tmp.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
    pItem->setCheckState(pi.m_bSelected ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    pItem->setData(Qt::UserRole, plugin.Key().GetData());
    const QString category = pi.m_EditorPlugins.IsEmpty() ? tr("Runtime / Engine") : pi.m_RuntimePlugins.IsEmpty() ? tr("Editor") :
                                                                                                                     tr("Editor and Runtime");
    pItem->setToolTip(category + (pi.m_bMandatory ? tr(" | Required by the editor") : QString()));

    PluginsList->addItem(pItem);
  }

  on_PluginsList_currentItemChanged(nullptr, nullptr);

  Template->clear();
  Template->addItem("<custom>");

  for (const xiiString tmp : templates)
  {
    Template->addItem(tmp.GetData());
  }

  UpdateInternalState();
}

void xiiQtPluginSelectionWidget::SyncStateToSet()
{
  // make sure to pull the latest state from the UI
  on_PluginsList_currentItemChanged(nullptr, PluginsList->currentItem());

  for (auto& state : m_States)
  {
    if (!state.m_pInfo->m_bMandatory)
      state.m_pInfo->m_bSelected = state.m_bSelected;
    state.m_pInfo->m_bLoadCopy = state.m_bLoadCopy;
  }
}

void xiiQtPluginSelectionWidget::SelectTemplate(const char* szTemplate)
{
  Template->setCurrentText(szTemplate);
}

void xiiQtPluginSelectionWidget::on_PluginsList_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
  if (previous)
  {
    auto& state       = m_States[PluginsList->row(previous)];
    state.m_bLoadCopy = LoadCopy->isChecked();
  }

  if (current)
  {
    auto& state = m_States[PluginsList->row(current)];
    const auto& info     = *state.m_pInfo;
    QString     metadata = tr("Bundle: %1\n%2\n\n").arg(xiiMakeQString(state.m_sID), current->toolTip());
    metadata += xiiMakeQString(info.m_sDescription);
    metadata += info.m_bMandatory ? tr("\nRequirement: Mandatory") : state.m_bIsDependency ? tr("\nRequirement: Dependency of a requested bundle") :
      state.m_bSelected                                                                    ? tr("\nRequirement: Requested by this project") :
                                                                                             tr("\nRequirement: Not requested");
    auto append = [&](const QString& label, const auto& values) {
      QStringList names;
      for (const auto& name : values)
        names.push_back(xiiMakeQString(name));
      if (!names.isEmpty())
        metadata += QLatin1Char('\n') + label + names.join(QStringLiteral(", "));
    };
    append(tr("Editor modules: "), info.m_EditorPlugins);
    append(tr("Engine process modules: "), info.m_EditorEnginePlugins);
    append(tr("Runtime modules: "), info.m_RuntimePlugins);
    append(tr("Requires: "), info.m_RequiredBundles);
    DescriptionText->setPlainText(metadata);
    LoadCopy->setChecked(state.m_bLoadCopy);
    LoadCopy->setEnabled(!m_bReadOnly && state.m_bSelected && !info.m_bMandatory && !info.m_RuntimePlugins.IsEmpty() && !info.m_bMissing);
  }
  else
  {
    LoadCopy->setEnabled(false);
    DescriptionText->clear();
  }
}

void xiiQtPluginSelectionWidget::on_PluginsList_itemChanged(QListWidgetItem* item)
{
  auto& state       = m_States[PluginsList->row(item)];
  state.m_bSelected = item->checkState() == Qt::Checked;

  Template->setCurrentIndex(0); // <custom>

  UpdateInternalState();
}

void xiiQtPluginSelectionWidget::on_Template_currentIndexChanged(int index)
{
  if (index <= 0)
    return;

  const xiiString sTemplate = Template->currentText().toUtf8().data();

  for (auto& s : m_States)
  {
    s.m_bSelected = s.m_pInfo->m_bMandatory || s.m_pInfo->m_EnabledInTemplates.Contains(sTemplate);
  }

  UpdateInternalState();
}

void xiiQtPluginSelectionWidget::UpdateInternalState()
{
  const QSignalBlocker blocker(PluginsList);
  for (auto& s : m_States)
  {
    s.m_bIsDependency = false;
  }

  for (auto& s : m_States)
  {
    if (s.m_bSelected)
    {
      ApplyRequired(s.m_pInfo->m_RequiredBundles);
    }
  }

  xiiSet<xiiString, xiiCompareString_NoCase> exclusiveFeatures;

  for (xiiUInt32 row = 0; row < m_States.GetCount(); ++row)
  {
    const auto& pi    = m_States[row];
    auto        pItem = PluginsList->item(row);

    if (pi.m_bSelected || pi.m_bIsDependency)
    {
      for (const auto& feat : pi.m_pInfo->m_ExclusiveFeatures)
      {
        exclusiveFeatures.Insert(feat);
      }
    }

    if ((pi.m_bIsDependency && !pi.m_bSelected) || pi.m_pInfo->m_bMandatory)
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    else
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);

    pItem->setCheckState(pi.m_bSelected || pi.m_bIsDependency ? Qt::Checked : Qt::Unchecked);
  }

  for (xiiUInt32 row = 0; row < m_States.GetCount(); ++row)
  {
    const auto& pi    = m_States[row];
    auto        pItem = PluginsList->item(row);

    if (pi.m_bSelected || pi.m_bIsDependency)
      continue;

    for (const auto& feat : pi.m_pInfo->m_ExclusiveFeatures)
    {
      if (exclusiveFeatures.Contains(feat))
      {
        goto disable;
      }
    }

    continue;

  disable:
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
  }

  if (m_bReadOnly)
    for (int row = 0; row < PluginsList->count(); ++row)
      PluginsList->item(row)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  Filter(m_Filter);
}

void xiiQtPluginSelectionWidget::ApplyRequired(xiiArrayPtr<xiiString> required)
{
  for (const auto& reqName : required)
  {
    for (auto& s : m_States)
    {
      if (s.m_sID.IsEqual_NoCase(reqName))
      {
        if (!s.m_bIsDependency)
        {
          s.m_bIsDependency = true;
          ApplyRequired(s.m_pInfo->m_RequiredBundles);
        }

        s.m_bIsDependency = true;
        break;
      }
    }
  }
}

void xiiQtPluginSelectionWidget::Filter(const QString& text)
{
  m_Filter               = text;
  const auto terms       = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
  int        iFirstMatch = -1;
  for (xiiUInt32 i = 0; i < m_States.GetCount(); ++i)
  {
    const auto&   state      = m_States[i];
    auto*         pItem      = PluginsList->item(i);
    const QString searchable = pItem->text() + pItem->toolTip() + xiiMakeQString(state.m_sID) + xiiMakeQString(state.m_pInfo->m_sDescription);
    bool          matches    = true;
    for (const auto& term : terms)
      matches &= searchable.contains(term, Qt::CaseInsensitive);
    switch (m_pCategory->currentIndex())
    {
      case 1:
        matches &= !state.m_pInfo->m_EditorPlugins.IsEmpty() && state.m_pInfo->m_RuntimePlugins.IsEmpty();
        break;
      case 2:
        matches &= state.m_pInfo->m_EditorPlugins.IsEmpty();
        break;
      case 3:
        matches &= !state.m_pInfo->m_EditorPlugins.IsEmpty() && !state.m_pInfo->m_RuntimePlugins.IsEmpty();
        break;
      case 4:
        matches &= state.m_bSelected || state.m_bIsDependency;
        break;
      case 5:
        matches &= state.m_pInfo->m_bMissing;
        break;
    }
    pItem->setHidden(!matches);
    if (matches && iFirstMatch < 0)
      iFirstMatch = static_cast<int>(i);
  }
  if (auto* pCurrent = PluginsList->currentItem(); pCurrent == nullptr || pCurrent->isHidden())
    PluginsList->setCurrentRow(iFirstMatch);
  // Update may run while list signals are blocked; never leave hidden entries editable.
  on_PluginsList_currentItemChanged(PluginsList->currentItem(), nullptr);
}

void xiiQtPluginSelectionWidget::SetReadOnly(bool bReadOnly)
{
  m_bReadOnly = bReadOnly;
  Template->setEnabled(!bReadOnly);
  LoadCopy->setEnabled(!bReadOnly && PluginsList->currentItem() != nullptr);
  UpdateInternalState();
}

xiiResult xiiQtPluginSelectionWidget::ValidateSelection(xiiStringBuilder& out_reason) const
{
  xiiSet<xiiString, xiiCompareString_NoCase> features;
  for (const auto& state : m_States)
  {
    if (!state.m_bSelected && !state.m_bIsDependency)
      continue;
    if (state.m_pInfo->m_bMissing)
    {
      out_reason.SetFormat("Plugin bundle '{}' is unavailable. Install it or remove its requirement.", state.m_sID);
      return XII_FAILURE;
    }
    for (const auto& required : state.m_pInfo->m_RequiredBundles)
    {
      if (!m_pPluginSet->m_Plugins.Contains(required))
      {
        out_reason.SetFormat("Plugin bundle '{}' requires unavailable bundle '{}'.", state.m_sID, required);
        return XII_FAILURE;
      }
    }
    for (const auto& feature : state.m_pInfo->m_ExclusiveFeatures)
    {
      if (features.Contains(feature))
      {
        out_reason.SetFormat("Selected plugin bundles conflict for feature '{}'.", feature);
        return XII_FAILURE;
      }
      features.Insert(feature);
    }
  }
  return XII_SUCCESS;
}
