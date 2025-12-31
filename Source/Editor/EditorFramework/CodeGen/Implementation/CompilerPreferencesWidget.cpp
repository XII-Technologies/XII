#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CompilerPreferencesWidget.moc.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiQtCompilerPreferencesWidget::xiiQtCompilerPreferencesWidget() :
  xiiQtPropertyTypeWidget(true)
{
  m_pCompilerPreset = new QComboBox();
  int counter       = 0;
  for (auto& compiler : xiiCppProject::GetMachineSpecificCompilers())
  {
    m_pCompilerPreset->addItem(xiiMakeQString(compiler.m_sNiceName), counter);
    ++counter;
  }
  connect(m_pCompilerPreset, &QComboBox::currentIndexChanged, this, &xiiQtCompilerPreferencesWidget::on_compiler_preset_changed);

  auto gridLayout = new QGridLayout();
  gridLayout->setColumnStretch(0, 1);
  gridLayout->setColumnStretch(1, 0);
  gridLayout->setColumnMinimumWidth(1, 5);
  gridLayout->setColumnStretch(2, 2);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  gridLayout->setSpacing(0);

  xiiStringBuilder fmt;
  QLabel*          versionText = new QLabel(xiiMakeQString(xiiFmt("This SDK was compiled with {} version {}. Select a compatible compiler.", xiiCppProject::CompilerToString(xiiCppProject::GetSdkCompiler()), xiiCppProject::GetSdkCompilerMajorVersion()).GetText(fmt)));
  versionText->setWordWrap(true);
  gridLayout->addWidget(versionText, 0, 0, 1, 3);

  gridLayout->addWidget(new QLabel("Compiler Preset"), 1, 0);
  gridLayout->addWidget(m_pCompilerPreset, 1, 2);
  m_pGroupLayout->addLayout(gridLayout);
}

xiiQtCompilerPreferencesWidget::~xiiQtCompilerPreferencesWidget() = default;

void xiiQtCompilerPreferencesWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtScopedUpdatesDisabled _(this);

  xiiQtPropertyTypeWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    const auto& selection = m_pTypeWidget->GetSelection();

    XII_ASSERT_DEBUG(selection.GetCount() == 1, "Expected exactly one object");
    auto pObj = selection[0].m_pObject;

    xiiEnum<xiiCompiler> m_Compiler;
    bool                 bIsCustomCompiler;
    xiiString            m_sCCompiler, m_sCppCompiler;

    {
      xiiVariant varCompiler, varIsCustomCompiler, varCCompiler, varCppCompiler;

      m_pObjectAccessor->GetValueByName(pObj, "Compiler", varCompiler).AssertSuccess();
      m_pObjectAccessor->GetValueByName(pObj, "CustomCompiler", varIsCustomCompiler).AssertSuccess();
      m_pObjectAccessor->GetValueByName(pObj, "CCompiler", varCCompiler).AssertSuccess();
      m_pObjectAccessor->GetValueByName(pObj, "CppCompiler", varCppCompiler).AssertSuccess();

      m_Compiler.SetValue(static_cast<xiiCompiler::StorageType>(varCompiler.Get<xiiInt64>()));
      bIsCustomCompiler = varIsCustomCompiler.Get<decltype(bIsCustomCompiler)>();
      m_sCCompiler      = varCCompiler.Get<decltype(m_sCCompiler)>();
      m_sCppCompiler    = varCppCompiler.Get<decltype(m_sCppCompiler)>();
    }

    xiiInt32    selectedIndex            = -1;
    const auto& machineSpecificCompilers = xiiCppProject::GetMachineSpecificCompilers();
    // first look for non custom compilers
    for (xiiUInt32 i = 0; i < machineSpecificCompilers.GetCount(); ++i)
    {
      const auto& curCompiler = machineSpecificCompilers[i];
      if ((curCompiler.m_bIsCustom == false) && (curCompiler.m_Compiler == m_Compiler) && (curCompiler.m_sCCompiler == m_sCCompiler) && (curCompiler.m_sCppCompiler == m_sCppCompiler))
      {
        selectedIndex = static_cast<int>(i);
        break;
      }
    }

    if (selectedIndex == -1)
    {
      // If we didn't find a system default compiler, look for custom compilers next
      for (xiiUInt32 i = 0; i < machineSpecificCompilers.GetCount(); ++i)
      {
        const auto& curCompiler = machineSpecificCompilers[i];
        if (curCompiler.m_bIsCustom == true && curCompiler.m_Compiler == m_Compiler)
        {
          selectedIndex = static_cast<int>(i);
          break;
        }
      }
    }

    if (selectedIndex >= 0)
    {
      m_pCompilerPreset->blockSignals(true);
      m_pCompilerPreset->setCurrentIndex(selectedIndex);
      m_pCompilerPreset->blockSignals(false);
    }
  }
}

void xiiQtCompilerPreferencesWidget::on_compiler_preset_changed(int index)
{
  auto compilerPresets = xiiCppProject::GetMachineSpecificCompilers();

  if (index >= 0 && index < (int)compilerPresets.GetCount())
  {
    const auto& preset = compilerPresets[index];

    const auto& selection = m_pTypeWidget->GetSelection();
    XII_ASSERT_DEV(selection.GetCount() == 1, "This Widget does not support multi selection");

    auto obj = selection[0].m_pObject;
    m_pObjectAccessor->StartTransaction("Change Compiler Preset");
    m_pObjectAccessor->SetValueByName(obj, "Compiler", preset.m_Compiler.GetValue()).AssertSuccess();
    m_pObjectAccessor->SetValueByName(obj, "CustomCompiler", preset.m_bIsCustom).AssertSuccess();
    m_pObjectAccessor->SetValueByName(obj, "CCompiler", preset.m_sCCompiler).AssertSuccess();
    m_pObjectAccessor->SetValueByName(obj, "CppCompiler", preset.m_sCppCompiler).AssertSuccess();
    m_pObjectAccessor->FinishTransaction();
  }
}

void xiiCompilerPreferences_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiCompilerPreferences>();

  auto& typeAccessor = e.m_pObject->GetTypeAccessor();

  if (typeAccessor.GetType() != pRtti)
    return;

  xiiPropertyUiState::Visibility compilerFieldsVisibility = xiiPropertyUiState::Default;

  bool bCustomCompiler = typeAccessor.GetValue("CustomCompiler").Get<bool>();
  if (!bCustomCompiler)
  {
    compilerFieldsVisibility = xiiPropertyUiState::Disabled;
  }
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  auto compiler = typeAccessor.GetValue("Compiler").Get<xiiInt64>();
  if (compiler == xiiCompiler::Vs2026 || compiler == xiiCompiler::Vs2022)
  {
    compilerFieldsVisibility = xiiPropertyUiState::Invisible;
  }
#endif

  auto& props = *e.m_pPropertyStates;

  props["CCompiler"].m_Visibility   = compilerFieldsVisibility;
  props["CppCompiler"].m_Visibility = compilerFieldsVisibility;
#if XII_ENABLED(XII_PLATFORM_LINUX)
  props["RcCompiler"].m_Visibility = xiiPropertyUiState::Invisible;
#else
  props["RcCompiler"].m_Visibility = (compiler == xiiCompiler::Vs2026 || compiler == xiiCompiler::Vs2022) ? xiiPropertyUiState::Invisible : xiiPropertyUiState::Default;
#endif
}
