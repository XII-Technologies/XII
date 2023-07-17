#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/TagSetPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/Document/Document.h>

#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <QLayout>
#include <QScrollArea>

xiiRttiMappedObjectFactory<xiiQtPropertyWidget> xiiQtPropertyGridWidget::s_Factory;

static xiiQtPropertyWidget* StandardTypeCreator(const xiiRTTI* pRtti)
{
  XII_ASSERT_DEV(pRtti->GetTypeFlags().IsSet(xiiTypeFlags::StandardType), "This function is only valid for StandardType properties, regardless of category");

  if (pRtti == xiiGetStaticRTTI<xiiVariant>())
  {
    return new xiiQtVariantPropertyWidget();
  }

  switch (pRtti->GetVariantType())
  {
    case xiiVariant::Type::Bool:
      return new xiiQtPropertyEditorCheckboxWidget();

    case xiiVariant::Type::Time:
      return new xiiQtPropertyEditorTimeWidget();

    case xiiVariant::Type::Float:
    case xiiVariant::Type::Double:
      return new xiiQtPropertyEditorDoubleSpinboxWidget(1);

    case xiiVariant::Type::Vector2:
      return new xiiQtPropertyEditorDoubleSpinboxWidget(2);

    case xiiVariant::Type::Vector3:
      return new xiiQtPropertyEditorDoubleSpinboxWidget(3);

    case xiiVariant::Type::Vector4:
      return new xiiQtPropertyEditorDoubleSpinboxWidget(4);

    case xiiVariant::Type::Vector2I:
      return new xiiQtPropertyEditorIntSpinboxWidget(2, -2147483645, 2147483645);

    case xiiVariant::Type::Vector3I:
      return new xiiQtPropertyEditorIntSpinboxWidget(3, -2147483645, 2147483645);

    case xiiVariant::Type::Vector4I:
      return new xiiQtPropertyEditorIntSpinboxWidget(4, -2147483645, 2147483645);

    case xiiVariant::Type::Vector2U:
      return new xiiQtPropertyEditorIntSpinboxWidget(2, 0, 2147483645);

    case xiiVariant::Type::Vector3U:
      return new xiiQtPropertyEditorIntSpinboxWidget(3, 0, 2147483645);

    case xiiVariant::Type::Vector4U:
      return new xiiQtPropertyEditorIntSpinboxWidget(4, 0, 2147483645);

    case xiiVariant::Type::Quaternion:
      return new xiiQtPropertyEditorQuaternionWidget();

    case xiiVariant::Type::Int8:
      return new xiiQtPropertyEditorIntSpinboxWidget(1, -127, 127);

    case xiiVariant::Type::UInt8:
      return new xiiQtPropertyEditorIntSpinboxWidget(1, 0, 255);

    case xiiVariant::Type::Int16:
      return new xiiQtPropertyEditorIntSpinboxWidget(1, -32767, 32767);

    case xiiVariant::Type::UInt16:
      return new xiiQtPropertyEditorIntSpinboxWidget(1, 0, 65535);

    case xiiVariant::Type::Int32:
    case xiiVariant::Type::Int64:
      return new xiiQtPropertyEditorIntSpinboxWidget(1, -2147483645, 2147483645);

    case xiiVariant::Type::UInt32:
    case xiiVariant::Type::UInt64:
      return new xiiQtPropertyEditorIntSpinboxWidget(1, 0, 2147483645);

    case xiiVariant::Type::String:
      return new xiiQtPropertyEditorLineEditWidget();

    case xiiVariant::Type::Color:
    case xiiVariant::Type::ColorGamma:
      return new xiiQtPropertyEditorColorWidget();

    case xiiVariant::Type::Angle:
      return new xiiQtPropertyEditorAngleWidget();


    default:
      XII_REPORT_FAILURE("No default property widget available for type: {0}", pRtti->GetTypeName());
      return nullptr;
  }
}

static xiiQtPropertyWidget* EnumCreator(const xiiRTTI* pRtti)
{
  return new xiiQtPropertyEditorEnumWidget();
}

static xiiQtPropertyWidget* BitflagsCreator(const xiiRTTI* pRtti)
{
  return new xiiQtPropertyEditorBitflagsWidget();
}

static xiiQtPropertyWidget* TagSetCreator(const xiiRTTI* pRtti)
{
  return new xiiQtPropertyEditorTagSetWidget();
}

static xiiQtPropertyWidget* VarianceTypeCreator(const xiiRTTI* pRtti)
{
  return new xiiQtVarianceTypeWidget();
}

static xiiQtPropertyWidget* Curve1DTypeCreator(const xiiRTTI* pRtti)
{
  return new xiiQtPropertyEditorCurve1DWidget();
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyGrid)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<bool>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<float>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<double>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVec2>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVec3>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVec4>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVec2I32>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVec3I32>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVec4I32>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVec2U32>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVec3U32>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVec4U32>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiQuat>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiInt8>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiUInt8>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiInt16>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiUInt16>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiInt32>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiUInt32>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiInt64>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiUInt64>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiConstCharPtr>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiString>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiTime>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiColor>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiColorGammaUB>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiAngle>(), StandardTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVariant>(), StandardTypeCreator);

    // \todo: xiiMat3, xiiMat4, xiiTransform, xiiUuid, xiiVariant
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiEnumBase>(), EnumCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiBitflagsBase>(), BitflagsCreator);

    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiTagSetWidgetAttribute>(), TagSetCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVarianceTypeBaseFloat>(), VarianceTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVarianceTypeBaseDouble>(), VarianceTypeCreator);
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiSingleCurveData>(), Curve1DTypeCreator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<bool>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<float>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<double>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVec2>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVec3>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVec4>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVec2I32>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVec3I32>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVec4I32>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVec2U32>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVec3U32>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVec4U32>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiQuat>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiInt8>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiUInt8>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiInt16>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiUInt16>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiInt32>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiUInt32>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiInt64>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiUInt64>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiConstCharPtr>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiString>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiTime>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiColor>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiColorGammaUB>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiAngle>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVariant>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiEnumBase>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiBitflagsBase>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiTagSetWidgetAttribute>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVarianceTypeBaseFloat>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVarianceTypeBaseDouble>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiSingleCurveData>());
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiRttiMappedObjectFactory<xiiQtPropertyWidget>& xiiQtPropertyGridWidget::GetFactory()
{
  return s_Factory;
}

xiiQtPropertyGridWidget::xiiQtPropertyGridWidget(QWidget* pParent, xiiDocument* pDocument, bool bBindToSelectionManager) :
  QWidget(pParent)
{
  m_pDocument = nullptr;

  m_pScroll = new QScrollArea(this);
  m_pScroll->setContentsMargins(0, 0, 0, 0);

  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setSpacing(0);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
  m_pLayout->addWidget(m_pScroll);

  m_pContent = new QWidget(this);
  m_pScroll->setWidget(m_pContent);
  m_pScroll->setWidgetResizable(true);
  m_pContent->setBackgroundRole(QPalette::ColorRole::Window);
  m_pContent->setAutoFillBackground(true);

  m_pContentLayout = new QVBoxLayout(m_pContent);
  m_pContentLayout->setSpacing(1);
  m_pContentLayout->setContentsMargins(0, 0, 0, 0);
  m_pContent->setLayout(m_pContentLayout);

  m_pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  m_pContentLayout->addSpacerItem(m_pSpacer);

  m_pTypeWidget = nullptr;

  s_Factory.m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::FactoryEventHandler, this));
  xiiPhantomRttiManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::TypeEventHandler, this));

  SetDocument(pDocument, bBindToSelectionManager);
}

xiiQtPropertyGridWidget::~xiiQtPropertyGridWidget()
{
  s_Factory.m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::FactoryEventHandler, this));
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::TypeEventHandler, this));

  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::SelectionEventHandler, this));
  }
}


void xiiQtPropertyGridWidget::SetDocument(xiiDocument* pDocument, bool bBindToSelectionManager)
{
  m_bBindToSelectionManager = bBindToSelectionManager;
  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::SelectionEventHandler, this));
  }

  m_pDocument = pDocument;

  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyGridWidget::SelectionEventHandler, this));
  }
}

void xiiQtPropertyGridWidget::ClearSelection()
{
  if (m_pTypeWidget)
  {
    m_pContentLayout->removeWidget(m_pTypeWidget);
    m_pTypeWidget->hide();

    m_pTypeWidget->PrepareToDie();

    m_pTypeWidget->deleteLater();
    m_pTypeWidget = nullptr;
  }

  m_Selection.Clear();
}

void xiiQtPropertyGridWidget::SetSelectionIncludeExcludeProperties(xiiStringView sIncludeProperties /*= {}*/, xiiStringView sExcludeProperties /*= {}*/)
{
  m_sSelectionIncludeProperties = sIncludeProperties;
  m_sSelectionExcludeProperties = sExcludeProperties;
}

void xiiQtPropertyGridWidget::SetSelection(const xiiDeque<const xiiDocumentObject*>& selection)
{
  xiiQtScopedUpdatesDisabled _(this);

  ClearSelection();

  m_Selection = selection;

  if (m_Selection.IsEmpty())
    return;

  {
    xiiHybridArray<xiiPropertySelection, 8> Items;
    Items.Reserve(m_Selection.GetCount());

    for (const auto* sel : m_Selection)
    {
      xiiPropertySelection s;
      s.m_pObject = sel;

      Items.PushBack(s);
    }

    const xiiRTTI* pCommonType = xiiQtPropertyWidget::GetCommonBaseType(Items);
    m_pTypeWidget              = new xiiQtTypeWidget(m_pContent, this, GetObjectAccessor(), pCommonType, m_sSelectionIncludeProperties, m_sSelectionExcludeProperties);
    m_pTypeWidget->SetSelection(Items);

    m_pContentLayout->insertWidget(0, m_pTypeWidget, 0);
  }
}

const xiiDocument* xiiQtPropertyGridWidget::GetDocument() const
{
  return m_pDocument;
}

const xiiDocumentObjectManager* xiiQtPropertyGridWidget::GetObjectManager() const
{
  return m_pDocument->GetObjectManager();
}

xiiCommandHistory* xiiQtPropertyGridWidget::GetCommandHistory() const
{
  return m_pDocument->GetCommandHistory();
}


xiiObjectAccessorBase* xiiQtPropertyGridWidget::GetObjectAccessor() const
{
  return m_pDocument->GetObjectAccessor();
}

xiiQtPropertyWidget* xiiQtPropertyGridWidget::CreateMemberPropertyWidget(const xiiAbstractProperty* pProp)
{
  // Try to create a registered widget for an existing xiiTypeWidgetAttribute.
  const xiiTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<xiiTypeWidgetAttribute>();
  if (pAttrib != nullptr)
  {
    xiiQtPropertyWidget* pWidget = xiiQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
    if (pWidget != nullptr)
      return pWidget;
  }

  // Try to create a registered widget for the given property type.
  xiiQtPropertyWidget* pWidget = xiiQtPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
  if (pWidget != nullptr)
    return pWidget;

  return new xiiQtUnsupportedPropertyWidget("No property grid widget registered");
}

xiiQtPropertyWidget* xiiQtPropertyGridWidget::CreatePropertyWidget(const xiiAbstractProperty* pProp)
{
  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      // Try to create a registered widget for an existing xiiTypeWidgetAttribute.
      const xiiTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<xiiTypeWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        xiiQtPropertyWidget* pWidget = xiiQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          return new xiiQtPropertyPointerWidget();
        else
          return new xiiQtUnsupportedPropertyWidget("Pointer: Use xiiPropertyFlags::PointerOwner or provide derived xiiTypeWidgetAttribute");
      }
      else
      {
        xiiQtPropertyWidget* pWidget = xiiQtPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
        if (pWidget != nullptr)
          return pWidget;

        if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
        {
          // Member struct / class
          return new xiiQtPropertyTypeWidget(true);
        }
      }
    }
    break;
    case xiiPropertyCategory::Set:
    case xiiPropertyCategory::Array:
    case xiiPropertyCategory::Map:
    {
      // Try to create a registered container widget for an existing xiiContainerWidgetAttribute.
      const xiiContainerWidgetAttribute* pAttrib = pProp->GetAttributeByType<xiiContainerWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        xiiQtPropertyWidget* pWidget = xiiQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      // Fallback to default container widgets.
      const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp);
      if (bIsValueType)
      {
        return new xiiQtPropertyStandardTypeContainerWidget();
      }
      else
      {
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
        {
          return new xiiQtUnsupportedPropertyWidget("Pointer: Use xiiPropertyFlags::PointerOwner or provide derived xiiContainerWidgetAttribute");
        }

        return new xiiQtPropertyTypeContainerWidget();
      }
    }
    break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return new xiiQtUnsupportedPropertyWidget();
}

void xiiQtPropertyGridWidget::SetCollapseState(xiiQtGroupBoxBase* pBox)
{
  xiiUInt32 uiHash     = GetGroupBoxHash(pBox);
  bool      bCollapsed = false;
  auto      it         = m_CollapseState.Find(uiHash);
  if (it.IsValid())
    bCollapsed = it.Value();

  pBox->SetCollapseState(bCollapsed);
}

void xiiQtPropertyGridWidget::OnCollapseStateChanged(bool bCollapsed)
{
  xiiQtGroupBoxBase* pBox   = qobject_cast<xiiQtGroupBoxBase*>(sender());
  xiiUInt32          uiHash = GetGroupBoxHash(pBox);
  m_CollapseState[uiHash]   = pBox->GetCollapseState();
}

void xiiQtPropertyGridWidget::ObjectAccessorChangeEventHandler(const xiiObjectAccessorChangeEvent& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

void xiiQtPropertyGridWidget::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  // TODO: even when not binding to the selection manager we need to test whether our selection is still valid.
  if (!m_bBindToSelectionManager)
    return;

  switch (e.m_Type)
  {
    case xiiSelectionManagerEvent::Type::SelectionCleared:
    {
      ClearSelection();
    }
    break;
    case xiiSelectionManagerEvent::Type::SelectionSet:
    case xiiSelectionManagerEvent::Type::ObjectAdded:
    case xiiSelectionManagerEvent::Type::ObjectRemoved:
    {
      SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
    }
    break;
  }
}

void xiiQtPropertyGridWidget::FactoryEventHandler(const xiiRttiMappedObjectFactory<xiiQtPropertyWidget>::Event& e)
{
  if (m_bBindToSelectionManager)
    SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
  else
  {
    xiiDeque<const xiiDocumentObject*> selection = m_Selection;
    SetSelection(selection);
  }
}

void xiiQtPropertyGridWidget::TypeEventHandler(const xiiPhantomRttiManagerEvent& e)
{
  // Adding types cannot affect the property grid content.
  if (e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeAdded)
    return;

  XII_PROFILE_SCOPE("TypeEventHandler");
  if (m_bBindToSelectionManager)
    SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
  else
  {
    xiiDeque<const xiiDocumentObject*> selection = m_Selection;
    SetSelection(selection);
  }
}

xiiUInt32 xiiQtPropertyGridWidget::GetGroupBoxHash(xiiQtGroupBoxBase* pBox) const
{
  xiiUInt32 uiHash = 0;

  QWidget* pCur = pBox;
  while (pCur != nullptr && pCur != this)
  {
    xiiQtGroupBoxBase* pCurBox = qobject_cast<xiiQtGroupBoxBase*>(pCur);
    if (pCurBox != nullptr)
    {
      const QByteArray name = pCurBox->GetTitle().toUtf8().data();
      uiHash += xiiHashingUtils::xxHash32(name, name.length());
    }
    pCur = pCur->parentWidget();
  }
  return uiHash;
}
