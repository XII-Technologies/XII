#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CodeEditorPreferencesWidget.moc.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiQtCodeEditorPreferencesWidget::xiiQtCodeEditorPreferencesWidget() :
  xiiQtPropertyTypeWidget(true)
{
  m_pCodeEditor = new QComboBox();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  m_pCodeEditor->addItem(xiiMakeQString("Visual Studio"), true);
#endif
  m_pCodeEditor->addItem(xiiMakeQString("Custom"), false);

  connect(m_pCodeEditor, &QComboBox::currentIndexChanged, this, &xiiQtCodeEditorPreferencesWidget::on_code_editor_changed);

  auto gridLayout = new QGridLayout();
  gridLayout->setColumnStretch(0, 1);
  gridLayout->setColumnStretch(1, 0);
  gridLayout->setColumnMinimumWidth(1, 5);
  gridLayout->setColumnStretch(2, 2);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  gridLayout->setSpacing(0);

  gridLayout->addWidget(new QLabel("Code Editor"), 1, 0);
  gridLayout->addWidget(m_pCodeEditor, 1, 2);
  m_pGroupLayout->addLayout(gridLayout);
}

xiiQtCodeEditorPreferencesWidget::~xiiQtCodeEditorPreferencesWidget() = default;

void xiiQtCodeEditorPreferencesWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtScopedUpdatesDisabled _(this);

  xiiQtPropertyTypeWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    const auto& selection = m_pTypeWidget->GetSelection();

    XII_ASSERT_DEBUG(selection.GetCount() == 1, "Expected exactly one object");
    auto pObj = selection[0].m_pObject;

    xiiVariant varIsVisualStudio;
    m_pObjectAccessor->GetValueByName(pObj, "IsVisualStudio", varIsVisualStudio).AssertSuccess();
    bool bIsVisualStudio = varIsVisualStudio.Get<decltype(bIsVisualStudio)>();

    m_pCodeEditor->blockSignals(true);
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    m_pCodeEditor->setCurrentIndex(bIsVisualStudio ? 0 : 1);
#else
    m_pCodeEditor->setCurrentIndex(0);
#endif
    m_pCodeEditor->blockSignals(false);
  }
}

void xiiQtCodeEditorPreferencesWidget::on_code_editor_changed(int index)
{
  const auto& selection = m_pTypeWidget->GetSelection();
  XII_ASSERT_DEV(selection.GetCount() == 1, "This Widget does not support multi selection");

  const QVariant variant = m_pCodeEditor->currentData();
  if (variant.toBool())
  {
    auto obj = selection[0].m_pObject;
    m_pObjectAccessor->StartTransaction("Change Code Editor Preset");
    m_pObjectAccessor->SetValueByName(obj, "IsVisualStudio", true).AssertSuccess();
    m_pObjectAccessor->FinishTransaction();
    return;
  }

  auto obj = selection[0].m_pObject;
  m_pObjectAccessor->StartTransaction("Change Code Editor Preset");
  m_pObjectAccessor->SetValueByName(obj, "IsVisualStudio", false).AssertSuccess();

  xiiVariant editorArgs;
  if (m_pObjectAccessor->GetValueByName(obj, "CodeEditorArgs", editorArgs).Succeeded() && editorArgs.Get<xiiString>().IsEmpty())
  {
    m_pObjectAccessor->SetValueByName(obj, "CodeEditorArgs", "{file} {line}").AssertSuccess();
  }
  m_pObjectAccessor->FinishTransaction();
}

void xiiCodeEditorPreferences_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiCodeEditorPreferences>();

  auto& typeAccessor = e.m_pObject->GetTypeAccessor();

  if (typeAccessor.GetType() != pRtti)
    return;

  xiiPropertyUiState::Visibility codeEditorFieldsVisibility = xiiPropertyUiState::Default;

  xiiStatus res;
  if (typeAccessor.GetValue("IsVisualStudio", xiiVariant(), &res).Get<bool>() && res.Succeeded())
  {
    codeEditorFieldsVisibility = xiiPropertyUiState::Invisible;
  }

  auto& props                          = *e.m_pPropertyStates;
  props["CodeEditorPath"].m_Visibility = codeEditorFieldsVisibility;
  props["CodeEditorArgs"].m_Visibility = codeEditorFieldsVisibility;
}
