#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVisualScriptVariableType, 1)
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Bool),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Byte),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Int),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Int64),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Float),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Double),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Color),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Vector3),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Quaternion),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Transform),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Time),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Angle),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::String),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::HashedString),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::GameObject),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Component),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::TypedPointer),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Variant),
  // XII_ENUM_CONSTANT(xiiVisualScriptVariableType::Resource), // Not yet supported in the editor
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

static_assert(xiiVisualScriptVariableType::Variant == xiiVisualScriptDataType::Variant);
static_assert(xiiVisualScriptVariableType::Resource == xiiVisualScriptDataType::Resource);

///////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVisualScriptVariableCategory, 1)
  XII_ENUM_CONSTANT(xiiVisualScriptVariableCategory::Member),
  XII_ENUM_CONSTANT(xiiVisualScriptVariableCategory::Array),
  // XII_ENUM_CONSTANT(xiiVisualScriptVariableCategory::Map), // Maps are not supported yet
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
xiiPropertyCategory::Enum xiiVisualScriptVariableCategory::GetPropertyCategory(Enum category)
{
  switch (category)
  {
    case Member:
      return xiiPropertyCategory::Member;
    case Array:
      return xiiPropertyCategory::Array;
    case Map:
      return xiiPropertyCategory::Map;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return xiiPropertyCategory::Member;
  }
}

///////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVisualScriptVariableTypeDeclaration, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVisualScriptVariableTypeDeclaration>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Type", xiiVisualScriptVariableType, m_Type),
    XII_ENUM_MEMBER_PROPERTY("Category", xiiVisualScriptVariableCategory, m_Category),
    XII_MEMBER_PROPERTY("Public", m_bPublic),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiVisualScriptVariableTypeDeclaration);
// clang-format on

xiiVisualScriptDataType::Enum xiiVisualScriptVariableTypeDeclaration::GetDataType() const
{
  if (m_Category == xiiVisualScriptVariableCategory::Array)
  {
    return xiiVisualScriptDataType::Array;
  }
  else if (m_Category == xiiVisualScriptVariableCategory::Map)
  {
    return xiiVisualScriptDataType::Map;
  }

  return static_cast<xiiVisualScriptDataType::Enum>(m_Type.GetValue());
}

void operator<<(xiiStreamWriter& inout_stream, const xiiVisualScriptVariableTypeDeclaration& value)
{
  inout_stream << value.m_Type;
  inout_stream << value.m_Category;
  inout_stream << value.m_bPublic;
}

void operator>>(xiiStreamReader& inout_stream, xiiVisualScriptVariableTypeDeclaration& value)
{
  inout_stream >> value.m_Type;
  inout_stream >> value.m_Category;
  inout_stream >> value.m_bPublic;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptVariableAttribute, 1, xiiRTTIDefaultAllocator<xiiVisualScriptVariableAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptVariableWidget::xiiQtVisualScriptVariableWidget()  = default;
xiiQtVisualScriptVariableWidget::~xiiQtVisualScriptVariableWidget() = default;

void xiiQtVisualScriptVariableWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtVariantPropertyWidget::InternalSetValue(value);

  bool bEnableTypeSelection = true;
  for (const auto& item : m_Items)
  {
    xiiVariant typeDeclVar;
    if (m_pObjectAccessor->GetValueByName(item.m_pObject, "Type", typeDeclVar, item.m_Index).Failed())
      break;

    if (typeDeclVar.GetReflectedType() != xiiGetStaticRTTI<xiiVisualScriptVariableTypeDeclaration>())
      break;

    auto typeDecl = typeDeclVar.Get<xiiVisualScriptVariableTypeDeclaration>();
    if (typeDecl.m_Type != xiiVisualScriptVariableType::Variant || typeDecl.m_Category != xiiVisualScriptVariableCategory::Member)
    {
      bEnableTypeSelection = false;
      break;
    }
  }

  EnableTypeSelection(bEnableTypeSelection);
}

xiiResult xiiQtVisualScriptVariableWidget::GetVariantTypeDisplayName(xiiVariantType::Enum type, xiiStringBuilder& out_sName) const
{
  if (type == xiiVariantType::Int8 || type == xiiVariantType::Int16 || type == xiiVariantType::UInt16 || type == xiiVariantType::UInt32 || type == xiiVariantType::UInt64 || type == xiiVariantType::StringView || type == xiiVariantType::TempHashedString)
    return XII_FAILURE;

  xiiVisualScriptDataType::Enum dataType = xiiVisualScriptDataType::FromVariantType(type);
  if (type != xiiVariantType::Invalid && dataType == xiiVisualScriptDataType::Invalid)
    return XII_FAILURE;

  const xiiRTTI* pVisualScriptDataType = xiiGetStaticRTTI<xiiVisualScriptDataType>();
  if (xiiReflectionUtils::EnumerationToString(pVisualScriptDataType, dataType, out_sName) == false)
    return XII_FAILURE;

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptVariableTypeDeclarationWidget::xiiQtVisualScriptVariableTypeDeclarationWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 4);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  m_pTypeList = new QComboBox(this);
  m_pTypeList->installEventFilter(this);
  m_pLayout->addWidget(m_pTypeList);

  m_pCategoryList = new QMenu(this);

  m_pCategoryButton = new QPushButton(this);
  m_pCategoryButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  m_pCategoryButton->installEventFilter(this);
  m_pCategoryButton->setMenu(m_pCategoryList);
  m_pLayout->addWidget(m_pCategoryButton);

  m_pVisibilityButton = new QPushButton(this);
  m_pVisibilityButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  QIcon icon;
  icon.addFile(QString::fromUtf8(":/EditorFramework/Icons/ObjectsHidden.svg"), QSize(), QIcon::Normal, QIcon::Off);
  icon.addFile(QString::fromUtf8(":/EditorFramework/Icons/ObjectsVisible.svg"), QSize(), QIcon::Normal, QIcon::On);
  m_pVisibilityButton->setIcon(icon);
  m_pVisibilityButton->setCheckable(true);
  m_pVisibilityButton->setToolTip(QCoreApplication::translate("VisualScriptVariable", "Make variable public or private", nullptr));
  m_pLayout->addWidget(m_pVisibilityButton);
}

xiiQtVisualScriptVariableTypeDeclarationWidget::~xiiQtVisualScriptVariableTypeDeclarationWidget() = default;

void xiiQtVisualScriptVariableTypeDeclarationWidget::OnInit()
{
  xiiHybridArray<xiiReflectionUtils::EnumKeyValuePair, 16> enumValues;

  {
    xiiReflectionUtils::GetEnumKeysAndValues(xiiGetStaticRTTI<xiiVisualScriptVariableType>(), enumValues);
    for (auto& val : enumValues)
    {
      m_pTypeList->addItem(xiiMakeQString(xiiTranslate(val.m_sKey)), val.m_iValue);
    }

    connect(m_pTypeList, &QComboBox::currentIndexChanged, [this](int iIndex) { ChangeType(); });
  }

  {
    QActionGroup* pActionGroup = new QActionGroup(this);
    xiiReflectionUtils::GetEnumKeysAndValues(xiiGetStaticRTTI<xiiVisualScriptVariableCategory>(), enumValues);

    QString sIcons[] = {
      ":/EditorPluginVisualScript/Icons/VariableCategoryMember.svg",
      ":/EditorPluginVisualScript/Icons/VariableCategoryArray.svg",
      ":/EditorPluginVisualScript/Icons/VariableCategoryMap.svg"};

    XII_ASSERT_DEV(XII_ARRAY_SIZE(sIcons) >= enumValues.GetCount(), "Need exactly one icon per category");

    for (xiiUInt32 i = 0; i < enumValues.GetCount(); ++i)
    {
      QIcon icon;
      icon.addFile(sIcons[i]);

      QAction* pAction = new QAction(icon, xiiMakeQString(xiiTranslate(enumValues[i].m_sKey)), this);
      pAction->setCheckable(true);

      connect(pAction, &QAction::triggered, [this]() { ChangeType(); });

      pActionGroup->addAction(pAction);
      m_pCategoryList->addAction(pAction);
    }
  }

  connect(m_pVisibilityButton, &QPushButton::toggled, [this](bool) { ChangeType(); });
}

void xiiQtVisualScriptVariableTypeDeclarationWidget::InternalSetValue(const xiiVariant& value)
{
  auto typeDecl = value.Get<xiiVisualScriptVariableTypeDeclaration>();

  {
    xiiQtScopedBlockSignals bs(m_pTypeList);
    for (int i = 0; i < m_pTypeList->count(); ++i)
    {
      if (m_pTypeList->itemData(i).toInt() == typeDecl.m_Type)
      {
        m_pTypeList->setCurrentIndex(i);
        break;
      }
    }
  }

  {
    xiiQtScopedBlockSignals bs(m_pCategoryList);
    QAction*                pAction = m_pCategoryList->actions()[typeDecl.m_Category];
    pAction->setChecked(true);

    m_pCategoryButton->setIcon(pAction->icon());
  }

  {
    xiiQtScopedBlockSignals bs(m_pVisibilityButton);
    m_pVisibilityButton->setChecked(typeDecl.m_bPublic);
  }
}

void xiiQtVisualScriptVariableTypeDeclarationWidget::ChangeType()
{
  QAction*  pAction    = nullptr;
  xiiUInt32 uiCategory = 0;
  for (auto action : m_pCategoryList->actions())
  {
    if (action->isChecked())
    {
      pAction = action;
      break;
    }
    ++uiCategory;
  }
  XII_ASSERT_DEV(pAction != nullptr, "No category selected");

  m_pObjectAccessor->StartTransaction("Change variable type");

  for (const auto& item : m_Items)
  {
    xiiVisualScriptVariableTypeDeclaration typeDecl;

    typeDecl.m_Type     = static_cast<xiiVisualScriptVariableType::Enum>(m_pTypeList->currentData().toInt());
    typeDecl.m_Category = static_cast<xiiVisualScriptVariableCategory::Enum>(uiCategory);
    typeDecl.m_bPublic  = m_pVisibilityButton->isChecked();

    m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, typeDecl, item.m_Index).AssertSuccess();
  }

  m_pObjectAccessor->FinishTransaction();

  m_pCategoryButton->setIcon(pAction->icon());
}

///////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVisualScriptVariable, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiVisualScriptVariable>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Type", m_TypeDecl),
    XII_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new xiiDefaultValueAttribute(0), new xiiVisualScriptVariableAttribute()),
    XII_MEMBER_PROPERTY("ClampRange", m_bClampRange),
    XII_MEMBER_PROPERTY("MinValue", m_fMinValue),
    XII_MEMBER_PROPERTY("MaxValue", m_fMaxValue)->AddAttributes(new xiiDefaultValueAttribute(1)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

// static
void xiiVisualScriptVariable::ConvertDefaultValue(xiiVariant& inout_defaultValue, xiiVisualScriptVariableTypeDeclaration targetTypeDecl)
{
  auto ConvertOrSetToDefault = [](xiiVariant& v, xiiVisualScriptDataType::Enum targetType) {
    if (targetType == xiiVisualScriptDataType::Variant)
      return;

    auto variantTargetType = xiiVisualScriptDataType::GetVariantType(targetType);
    if (variantTargetType == xiiVariantType::Invalid || variantTargetType == xiiVariantType::TypedObject || variantTargetType == xiiVariantType::TypedPointer)
    {
      v = xiiVariant();
      return;
    }

    xiiResult res = XII_SUCCESS;
    v             = v.ConvertTo(variantTargetType, &res);
    if (res.Failed())
    {
      v = xiiReflectionUtils::GetDefaultVariantFromType(xiiVisualScriptDataType::GetRtti(targetType));
    }
  };

  auto targetType = static_cast<xiiVisualScriptDataType::Enum>(targetTypeDecl.m_Type.GetValue());
  if (targetTypeDecl.m_Category == xiiVisualScriptVariableCategory::Array)
  {
    if (inout_defaultValue.IsA<xiiVariantArray>())
    {
      xiiVariantArray a = inout_defaultValue.Get<xiiVariantArray>();
      for (auto& v : a)
      {
        ConvertOrSetToDefault(v, targetType);
      }
      inout_defaultValue = a;
    }
    else
    {
      xiiVariantArray a;
      ConvertOrSetToDefault(inout_defaultValue, targetType);
      a.PushBack(inout_defaultValue);
      inout_defaultValue = a;
    }
  }
  else if (targetTypeDecl.m_Category == xiiVisualScriptVariableCategory::Map)
  {
    if (inout_defaultValue.IsA<xiiVariantDictionary>())
    {
      xiiVariantDictionary d = inout_defaultValue.Get<xiiVariantDictionary>();
      for (auto it = d.GetIterator(); it.IsValid(); it.Next())
      {
        xiiVariant v = it.Value();
        ConvertOrSetToDefault(v, targetType);
        d[it.Key()] = v;
      }
      inout_defaultValue = d;
    }
    else
    {
      xiiVariantDictionary d;
      ConvertOrSetToDefault(inout_defaultValue, targetType);
      d["Key"]           = inout_defaultValue;
      inout_defaultValue = d;
    }
  }
  else
  {
    ConvertOrSetToDefault(inout_defaultValue, targetType);
  }
}

/////////////////////////////////////////////////////////////////////////////

static xiiQtPropertyWidget* VisualScriptVariableTypeCreator(const xiiRTTI* pRtti)
{
  return new xiiQtVisualScriptVariableWidget();
}

static xiiQtPropertyWidget* VisualScriptVariableTypeDeclarationCreator(const xiiRTTI* pRtti)
{
  return new xiiQtVisualScriptVariableTypeDeclarationWidget();
}

void xiiVisualScriptVariable_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiVisualScriptVariable>();

  auto& typeAccessor = e.m_pObject->GetTypeAccessor();

  if (typeAccessor.GetType() != pRtti)
    return;

  auto       typeDecl            = typeAccessor.GetValue("Type").Get<xiiVisualScriptVariableTypeDeclaration>();
  const bool bIsPublicNumberType = typeDecl.m_bPublic && xiiVisualScriptDataType::IsNumber(static_cast<xiiVisualScriptDataType::Enum>(typeDecl.m_Type.GetValue()));

  auto& props = *e.m_pPropertyStates;

  auto clampRangeVisibility        = bIsPublicNumberType ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  props["ClampRange"].m_Visibility = clampRangeVisibility;
  props["MinValue"].m_Visibility   = clampRangeVisibility;
  props["MaxValue"].m_Visibility   = clampRangeVisibility;
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, VisualScriptVariable)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVisualScriptVariableAttribute>(), VisualScriptVariableTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVisualScriptVariableTypeDeclaration>(), VisualScriptVariableTypeDeclarationCreator);

    xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiVisualScriptVariable_PropertyMetaStateEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVisualScriptVariableAttribute>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVisualScriptVariableTypeDeclaration>());

    xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiVisualScriptVariable_PropertyMetaStateEventHandler);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

///////////////////////////////////////////////////////////////////////////

class xiiVisualScriptVariablePatch_1_2 : public xiiGraphPatch
{
public:
  xiiVisualScriptVariablePatch_1_2() : xiiGraphPatch("xiiVisualScriptVariable", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pDefaultValue = pNode->FindProperty("DefaultValue");
    auto* pExpose       = pNode->FindProperty("Expose");

    if (pDefaultValue && pExpose)
    {
      xiiVisualScriptVariableTypeDeclaration typeDecl;

      xiiVariantType::Enum variantType = pDefaultValue->m_Value.GetType();
      typeDecl.m_Type                  = static_cast<xiiVisualScriptVariableType::Enum>(xiiVisualScriptDataType::FromVariantType(variantType));
      typeDecl.m_Category              = xiiVisualScriptVariableCategory::Member;

      if (variantType == xiiVariantType::VariantArray)
      {
        typeDecl.m_Type     = xiiVisualScriptVariableType::Variant;
        typeDecl.m_Category = xiiVisualScriptVariableCategory::Array;
      }
      else if (variantType == xiiVariantType::VariantDictionary)
      {
        typeDecl.m_Type     = xiiVisualScriptVariableType::Variant;
        typeDecl.m_Category = xiiVisualScriptVariableCategory::Map;
      }

      typeDecl.m_bPublic = pExpose->m_Value.ConvertTo<bool>();

      pNode->AddProperty("Type", typeDecl);
    }
  }
};

xiiVisualScriptVariablePatch_1_2 g_xiiVisualScriptVariablePatch_1_2;

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVisualScriptExpressionDataType, 1)
  XII_ENUM_CONSTANT(xiiVisualScriptExpressionDataType::Int),
  XII_ENUM_CONSTANT(xiiVisualScriptExpressionDataType::Float),
  XII_ENUM_CONSTANT(xiiVisualScriptExpressionDataType::Vector3),
  XII_ENUM_CONSTANT(xiiVisualScriptExpressionDataType::Color),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVisualScriptExpressionVariable, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVisualScriptExpressionVariable>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiVisualScriptExpressionDataType, m_Type),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptDataType::Enum xiiVisualScriptExpressionDataType::GetVisualScriptDataType(Enum dataType)
{
  switch (dataType)
  {
    case Int:
      return xiiVisualScriptDataType::Int;
    case Float:
      return xiiVisualScriptDataType::Float;
    case Vector3:
      return xiiVisualScriptDataType::Vector3;
    case Color:
      return xiiVisualScriptDataType::Color;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return xiiVisualScriptDataType::Invalid;
}
