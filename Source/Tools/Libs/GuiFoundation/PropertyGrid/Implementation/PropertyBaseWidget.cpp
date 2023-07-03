#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/ElementGroupButton.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <GuiFoundation/Widgets/InlinedGroupBox.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <QClipboard>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QScrollArea>
#include <QStringBuilder>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiPropertyClipboard, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiPropertyClipboard>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("m_Type", m_Type),
    XII_MEMBER_PROPERTY("m_Value", m_Value),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

/// *** BASE ***
xiiQtPropertyWidget::xiiQtPropertyWidget() :
  QWidget(nullptr)
{
  m_bUndead    = false;
  m_bIsDefault = true;
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

xiiQtPropertyWidget::~xiiQtPropertyWidget() = default;

void xiiQtPropertyWidget::Init(xiiQtPropertyGridWidget* pGrid, xiiObjectAccessorBase* pObjectAccessor, const xiiRTTI* pType, const xiiAbstractProperty* pProp)
{
  m_pGrid           = pGrid;
  m_pObjectAccessor = pObjectAccessor;
  m_pType           = pType;
  m_pProp           = pProp;
  XII_ASSERT_DEBUG(m_pGrid && m_pObjectAccessor && m_pType && m_pProp, "");

  if (pProp->GetAttributeByType<xiiReadOnlyAttribute>() != nullptr || pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
    setEnabled(false);

  OnInit();
}

void xiiQtPropertyWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  m_Items = items;
}

const char* xiiQtPropertyWidget::GetLabel(xiiStringBuilder& ref_sTmp) const
{
  ref_sTmp.Set(m_pType->GetTypeName(), "::", m_pProp->GetPropertyName());
  return ref_sTmp;
}

void xiiQtPropertyWidget::ExtendContextMenu(QMenu& m)
{
  m.setToolTipsVisible(true);
  // revert
  {
    QAction* pRevert = m.addAction("Revert to Default");
    pRevert->setEnabled(!m_bIsDefault);
    connect(pRevert, &QAction::triggered, this, [this]() {
      m_pObjectAccessor->StartTransaction("Revert to Default");

      switch (m_pProp->GetCategory())
      {
        case xiiPropertyCategory::Enum::Array:
        case xiiPropertyCategory::Enum::Set:
        case xiiPropertyCategory::Enum::Map:
        {

          xiiStatus res = xiiStatus(XII_SUCCESS);
          if (!m_Items[0].m_Index.IsValid())
          {
            // Revert container
            xiiDefaultContainerState defaultState(m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
            res = defaultState.RevertContainer();
          }
          else
          {
            const bool bIsValueType = xiiReflectionUtils::IsValueType(m_pProp) || m_pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags);
            if (bIsValueType)
            {
              // Revert container value type element
              xiiDefaultContainerState defaultState(m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
              res = defaultState.RevertElement({});
            }
            else
            {
              // Revert objects pointed to by the object type element
              xiiHybridArray<xiiPropertySelection, 8> ResolvedObjects;
              for (const auto& item : m_Items)
              {
                xiiUuid ObjectGuid = m_pObjectAccessor->Get<xiiUuid>(item.m_pObject, m_pProp, item.m_Index);
                if (ObjectGuid.IsValid())
                {
                  ResolvedObjects.PushBack({m_pObjectAccessor->GetObject(ObjectGuid), xiiVariant()});
                }
              }
              xiiDefaultObjectState defaultState(m_pObjectAccessor, ResolvedObjects);
              res = defaultState.RevertObject();
            }
          }
          if (res.Failed())
          {
            res.LogFailure();
            m_pObjectAccessor->CancelTransaction();
            return;
          }
        }
        break;
        default:
        {
          // Revert object member property
          xiiDefaultObjectState defaultState(m_pObjectAccessor, m_Items);
          xiiStatus res = defaultState.RevertProperty(m_pProp);
          if (res.Failed())
          {
            res.LogFailure();
            m_pObjectAccessor->CancelTransaction();
            return;
          }
        }
        break;
      }
      m_pObjectAccessor->FinishTransaction();
    });
  }

  const char* szMimeType = "application/xiiEditor.Property";
  bool        bValueType = xiiReflectionUtils::IsValueType(m_pProp) || m_pProp->GetFlags().IsAnySet(xiiPropertyFlags::Bitflags | xiiPropertyFlags::IsEnum);
  // Copy
  {
    xiiVariant commonValue = GetCommonValue(m_Items, m_pProp);
    QAction*   pCopy       = m.addAction("Copy Value");
    if (!bValueType)
    {
      pCopy->setEnabled(false);
      pCopy->setToolTip("Not a value type");
    }
    else if (!commonValue.IsValid())
    {
      pCopy->setEnabled(false);
      pCopy->setToolTip("No common value in selection");
    }

    connect(pCopy, &QAction::triggered, this, [this, szMimeType, commonValue]() {
      xiiPropertyClipboard content;
      content.m_Type = m_pProp->GetSpecificType()->GetTypeName();
      content.m_Value = commonValue;

      // Serialize
      xiiContiguousMemoryStreamStorage streamStorage;
      xiiMemoryStreamWriter memoryWriter(&streamStorage);
      xiiReflectionSerializer::WriteObjectToDDL(memoryWriter, xiiGetStaticRTTI<xiiPropertyClipboard>(), &content);
      memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

      // Write to clipboard
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      QByteArray encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize32());

      mimeData->setData(szMimeType, encodedData);
      mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
      clipboard->setMimeData(mimeData);
    });
  }

  // Paste
  {
    QAction* pPaste = m.addAction("Paste Value");

    QClipboard* clipboard = QApplication::clipboard();
    auto        mimedata  = clipboard->mimeData();

    if (!bValueType)
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("Not a value type");
    }
    else if (!isEnabled())
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("Property is read only");
    }
    else if (!mimedata->hasFormat(szMimeType))
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("No property in clipboard");
    }
    else
    {
      QByteArray               ba = mimedata->data(szMimeType);
      xiiRawMemoryStreamReader memoryReader(ba.data(), ba.count());

      xiiPropertyClipboard content;
      xiiReflectionSerializer::ReadObjectPropertiesFromDDL(memoryReader, *xiiGetStaticRTTI<xiiPropertyClipboard>(), &content);

      const bool      bIsArray              = m_pProp->GetCategory() == xiiPropertyCategory::Array || m_pProp->GetCategory() == xiiPropertyCategory::Set;
      const xiiRTTI*  pClipboardType        = xiiRTTI::FindTypeByName(content.m_Type);
      const bool      bIsEnumeration        = pClipboardType && (pClipboardType->IsDerivedFrom<xiiEnumBase>() || pClipboardType->IsDerivedFrom<xiiBitflagsBase>() || m_pProp->GetSpecificType()->IsDerivedFrom<xiiEnumBase>() || m_pProp->GetSpecificType()->IsDerivedFrom<xiiBitflagsBase>());
      const bool      bEnumerationMissmatch = bIsEnumeration ? pClipboardType != m_pProp->GetSpecificType() : false;
      const xiiResult clamped               = xiiReflectionUtils::ClampValue(content.m_Value, m_pProp->GetAttributeByType<xiiClampValueAttribute>());

      if (content.m_Value.IsA<xiiVariantArray>() != bIsArray)
      {
        pPaste->setEnabled(false);
        xiiStringBuilder sTemp;
        sTemp.Format("Cannot convert clipboard and property content between arrays and members.");
        pPaste->setToolTip(sTemp.GetData());
      }
      else if (bEnumerationMissmatch || !content.m_Value.CanConvertTo(m_pProp->GetSpecificType()->GetVariantType()) && content.m_Type != m_pProp->GetSpecificType()->GetTypeName())
      {
        pPaste->setEnabled(false);
        xiiStringBuilder sTemp;
        sTemp.Format("Cannot convert clipboard of type '{}' to property of type '{}'", content.m_Type, m_pProp->GetSpecificType()->GetTypeName());
        pPaste->setToolTip(sTemp.GetData());
      }
      else if (clamped.Failed())
      {
        pPaste->setEnabled(false);
        xiiStringBuilder sTemp;
        sTemp.Format("The member property '{}' has a xiiClampValueAttribute but xiiReflectionUtils::ClampValue failed.", m_pProp->GetPropertyName());
      }

      connect(pPaste, &QAction::triggered, this, [this, content]() {
        m_pObjectAccessor->StartTransaction("Paste Value");
        if (content.m_Value.IsA<xiiVariantArray>())
        {
          const xiiVariantArray& values = content.m_Value.Get<xiiVariantArray>();
          for (const xiiPropertySelection& sel : m_Items)
          {
            if (m_pObjectAccessor->Clear(sel.m_pObject, m_pProp->GetPropertyName()).Failed())
            {
              m_pObjectAccessor->CancelTransaction();
              return;
            }
            for (const xiiVariant& val : values)
            {
              if (m_pObjectAccessor->InsertValue(sel.m_pObject, m_pProp, val, -1).Failed())
              {
                m_pObjectAccessor->CancelTransaction();
                return;
              }
            }
          }
        }
        else
        {
          for (const xiiPropertySelection& sel : m_Items)
          {
            if (m_pObjectAccessor->SetValue(sel.m_pObject, m_pProp, content.m_Value, sel.m_Index).Failed())
            {
              m_pObjectAccessor->CancelTransaction();
              return;
            }
          }
        }

        m_pObjectAccessor->FinishTransaction();
      });
    }
  }

  // copy internal name
  {
    auto lambda = [this]() {
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData*  mimeData  = new QMimeData();
      mimeData->setText(m_pProp->GetPropertyName());
      clipboard->setMimeData(mimeData);

      xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(
        xiiFmt("Copied Property Name: {}", m_pProp->GetPropertyName()), xiiTime::Seconds(5));
    };

    QAction* pAction = m.addAction("Copy Internal Property Name:");
    connect(pAction, &QAction::triggered, this, lambda);

    QAction* pAction2 = m.addAction(m_pProp->GetPropertyName());
    connect(pAction2, &QAction::triggered, this, lambda);
  }
}

const xiiRTTI* xiiQtPropertyWidget::GetCommonBaseType(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  const xiiRTTI* pSubtype = nullptr;

  for (const auto& item : items)
  {
    const auto& accessor = item.m_pObject->GetTypeAccessor();

    if (pSubtype == nullptr)
      pSubtype = accessor.GetType();
    else
    {
      pSubtype = xiiReflectionUtils::GetCommonBaseType(pSubtype, accessor.GetType());
    }
  }

  return pSubtype;
}

QColor xiiQtPropertyWidget::SetPaletteBackgroundColor(xiiColorGammaUB inputColor, QPalette& ref_palette)
{
  QColor qColor = qApp->palette().color(QPalette::Window);
  if (inputColor.a != 0)
  {
    const xiiColor paletteColorLinear = qtToXIIColor(qColor);
    const xiiColor inputColorLinear   = inputColor;

    xiiColor blendedColor = xiiMath::Lerp(paletteColorLinear, inputColorLinear, inputColorLinear.a);
    blendedColor.a        = 1.0f;
    qColor                = xiiToQtColor(blendedColor);
  }

  ref_palette.setBrush(QPalette::Window, QBrush(qColor, Qt::SolidPattern));
  return qColor;
}

bool xiiQtPropertyWidget::GetCommonVariantSubType(const xiiHybridArray<xiiPropertySelection, 8>& items, const xiiAbstractProperty* pProperty, xiiVariantType::Enum& out_type)
{
  bool bFirst = true;
  // check if we have multiple values
  for (const auto& item : items)
  {
    if (bFirst)
    {
      bFirst = false;
      xiiVariant value;
      m_pObjectAccessor->GetValue(item.m_pObject, pProperty, value, item.m_Index).AssertSuccess();
      out_type = value.GetType();
    }
    else
    {
      xiiVariant valueNext;
      m_pObjectAccessor->GetValue(item.m_pObject, pProperty, valueNext, item.m_Index).AssertSuccess();
      if (valueNext.GetType() != out_type)
      {
        out_type = xiiVariantType::Invalid;
        return false;
      }
    }
  }
  return true;
}

xiiVariant xiiQtPropertyWidget::GetCommonValue(const xiiHybridArray<xiiPropertySelection, 8>& items, const xiiAbstractProperty* pProperty)
{
  if (!items[0].m_Index.IsValid() && (m_pProp->GetCategory() == xiiPropertyCategory::Array || m_pProp->GetCategory() == xiiPropertyCategory::Set))
  {
    xiiVariantArray values;
    // check if we have multiple values
    for (xiiUInt32 i = 0; i < items.GetCount(); i++)
    {
      const auto& item = items[i];
      if (i == 0)
      {
        m_pObjectAccessor->GetValues(item.m_pObject, pProperty, values).AssertSuccess();
      }
      else
      {
        xiiVariantArray valuesNext;
        m_pObjectAccessor->GetValues(item.m_pObject, pProperty, valuesNext).AssertSuccess();
        if (values != valuesNext)
        {
          return xiiVariant();
        }
      }
    }
    return values;
  }
  else
  {
    xiiVariant value;
    // check if we have multiple values
    for (const auto& item : items)
    {
      if (!value.IsValid())
      {
        m_pObjectAccessor->GetValue(item.m_pObject, pProperty, value, item.m_Index).AssertSuccess();
      }
      else
      {
        xiiVariant valueNext;
        m_pObjectAccessor->GetValue(item.m_pObject, pProperty, valueNext, item.m_Index).AssertSuccess();
        if (value != valueNext)
        {
          value = xiiVariant();
          break;
        }
      }
    }
    return value;
  }
}

void xiiQtPropertyWidget::PrepareToDie()
{
  XII_ASSERT_DEBUG(!m_bUndead, "Object has already been marked for cleanup");

  m_bUndead = true;

  DoPrepareToDie();
}


void xiiQtPropertyWidget::OnCustomContextMenu(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  ExtendContextMenu(m);
  m_pGrid->ExtendContextMenu(m, m_Items, m_pProp);

  m.exec(pt); // pt is already in global space, because we fixed that
}

void xiiQtPropertyWidget::Broadcast(xiiPropertyEvent::Type type)
{
  xiiPropertyEvent ed;
  ed.m_Type      = type;
  ed.m_pProperty = m_pProp;
  PropertyChangedHandler(ed);
}

void xiiQtPropertyWidget::PropertyChangedHandler(const xiiPropertyEvent& ed)
{
  if (m_bUndead)
    return;


  switch (ed.m_Type)
  {
    case xiiPropertyEvent::Type::SingleValueChanged:
    {
      xiiStringBuilder sTemp;
      sTemp.Format("Change Property '{0}'", xiiTranslate(ed.m_pProperty->GetPropertyName()));
      m_pObjectAccessor->StartTransaction(sTemp);

      xiiStatus res;
      for (const auto& sel : *ed.m_pItems)
      {
        res = m_pObjectAccessor->SetValue(sel.m_pObject, ed.m_pProperty, ed.m_Value, sel.m_Index);
        if (res.m_Result.Failed())
          break;
      }

      if (res.m_Result.Failed())
        m_pObjectAccessor->CancelTransaction();
      else
        m_pObjectAccessor->FinishTransaction();

      xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Changing the property failed.");
    }
    break;

    case xiiPropertyEvent::Type::BeginTemporary:
    {
      xiiStringBuilder sTemp;
      sTemp.Format("Change Property '{0}'", xiiTranslate(ed.m_pProperty->GetPropertyName()));
      m_pObjectAccessor->BeginTemporaryCommands(sTemp);
    }
    break;

    case xiiPropertyEvent::Type::EndTemporary:
    {
      m_pObjectAccessor->FinishTemporaryCommands();
    }
    break;

    case xiiPropertyEvent::Type::CancelTemporary:
    {
      m_pObjectAccessor->CancelTemporaryCommands();
    }
    break;
  }
}

bool xiiQtPropertyWidget::eventFilter(QObject* pWatched, QEvent* pEvent)
{
  if (pEvent->type() == QEvent::Wheel)
  {
    if (pWatched->parent())
    {
      pWatched->parent()->event(pEvent);
    }

    return true;
  }

  return false;
}

/// *** xiiQtUnsupportedPropertyWidget ***

xiiQtUnsupportedPropertyWidget::xiiQtUnsupportedPropertyWidget(const char* szMessage) :
  xiiQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLabel(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);
  m_sMessage = szMessage;
}

void xiiQtUnsupportedPropertyWidget::OnInit()
{
  xiiQtScopedBlockSignals bs(m_pWidget);

  QString sMessage = QStringLiteral("Unsupported Type: ") % QString::fromUtf8(m_pProp->GetSpecificType()->GetTypeName());
  if (!m_sMessage.IsEmpty())
    sMessage += QStringLiteral(" (") % QString::fromUtf8(m_sMessage, m_sMessage.GetElementCount()) % QStringLiteral(")");
  m_pWidget->setText(sMessage);
  m_pWidget->setToolTip(sMessage);
}


/// *** xiiQtStandardPropertyWidget ***

xiiQtStandardPropertyWidget::xiiQtStandardPropertyWidget() :
  xiiQtPropertyWidget()
{
}

void xiiQtStandardPropertyWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtPropertyWidget::SetSelection(items);

  m_OldValue = GetCommonValue(items, m_pProp);
  InternalSetValue(m_OldValue);
}

void xiiQtStandardPropertyWidget::BroadcastValueChanged(const xiiVariant& NewValue)
{
  if (NewValue == m_OldValue)
    return;

  m_OldValue = NewValue;

  xiiPropertyEvent ed;
  ed.m_Type      = xiiPropertyEvent::Type::SingleValueChanged;
  ed.m_pProperty = m_pProp;
  ed.m_Value     = NewValue;
  ed.m_pItems    = &m_Items;
  PropertyChangedHandler(ed);
}


/// *** xiiQtPropertyPointerWidget ***

xiiQtPropertyPointerWidget::xiiQtPropertyPointerWidget() :
  xiiQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pGroup       = new xiiQtCollapsibleGroupBox(this);
  m_pGroupLayout = new QHBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->GetContent()->setLayout(m_pGroupLayout);

  m_pLayout->addWidget(m_pGroup);

  m_pAddButton = new xiiQtAddSubElementButton();
  m_pGroup->GetHeader()->layout()->addWidget(m_pAddButton);

  m_pDeleteButton = new xiiQtElementGroupButton(m_pGroup->GetHeader(), xiiQtElementGroupButton::ElementAction::DeleteElement, this);
  m_pGroup->GetHeader()->layout()->addWidget(m_pDeleteButton);
  connect(m_pDeleteButton, &QToolButton::clicked, this, &xiiQtPropertyPointerWidget::OnDeleteButtonClicked);

  m_pTypeWidget = nullptr;
}

xiiQtPropertyPointerWidget::~xiiQtPropertyPointerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    xiiMakeDelegate(&xiiQtPropertyPointerWidget::StructureEventHandler, this));
}

void xiiQtPropertyPointerWidget::OnInit()
{
  UpdateTitle();
  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &xiiQtGroupBoxBase::CollapseStateChanged, m_pGrid, &xiiQtPropertyGridWidget::OnCollapseStateChanged);

  // Add Buttons
  auto pAttr = m_pProp->GetAttributeByType<xiiContainerAttribute>();
  m_pAddButton->setVisible(!pAttr || pAttr->CanAdd());
  m_pDeleteButton->setVisible(!pAttr || pAttr->CanDelete());

  m_pAddButton->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    xiiMakeDelegate(&xiiQtPropertyPointerWidget::StructureEventHandler, this));
}

void xiiQtPropertyPointerWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtScopedUpdatesDisabled _(this);

  xiiQtPropertyWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    m_pGroupLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }


  xiiHybridArray<xiiPropertySelection, 8> emptyItems;
  xiiHybridArray<xiiPropertySelection, 8> subItems;
  for (const auto& item : m_Items)
  {
    xiiUuid ObjectGuid = m_pObjectAccessor->Get<xiiUuid>(item.m_pObject, m_pProp, item.m_Index);
    if (!ObjectGuid.IsValid())
    {
      emptyItems.PushBack(item);
    }
    else
    {
      xiiPropertySelection sel;
      sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);

      subItems.PushBack(sel);
    }
  }

  auto pAttr = m_pProp->GetAttributeByType<xiiContainerAttribute>();
  if (!pAttr || pAttr->CanAdd())
    m_pAddButton->setVisible(!emptyItems.IsEmpty());
  if (!pAttr || pAttr->CanDelete())
    m_pDeleteButton->setVisible(!subItems.IsEmpty());

  if (!emptyItems.IsEmpty())
  {
    m_pAddButton->SetSelection(emptyItems);
  }

  const xiiRTTI* pCommonType = nullptr;
  if (!subItems.IsEmpty())
  {
    pCommonType = xiiQtPropertyWidget::GetCommonBaseType(subItems);

    m_pTypeWidget = new xiiQtTypeWidget(m_pGroup->GetContent(), m_pGrid, m_pObjectAccessor, pCommonType, nullptr, nullptr);
    m_pTypeWidget->SetSelection(subItems);

    m_pGroupLayout->addWidget(m_pTypeWidget);
  }

  UpdateTitle(pCommonType);
}


void xiiQtPropertyPointerWidget::DoPrepareToDie()
{
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

void xiiQtPropertyPointerWidget::UpdateTitle(const xiiRTTI* pType /*= nullptr*/)
{
  xiiStringBuilder sb = xiiTranslate(m_pProp->GetPropertyName());
  if (pType != nullptr)
  {
    sb.Append(": ", xiiTranslate(pType->GetTypeName()));
  }
  m_pGroup->SetTitle(sb);
}

void xiiQtPropertyPointerWidget::OnDeleteButtonClicked()
{
  m_pObjectAccessor->StartTransaction("Delete Object");

  xiiStatus                                     res;
  const xiiHybridArray<xiiPropertySelection, 8> selection = m_pTypeWidget->GetSelection();
  for (auto& item : selection)
  {
    res = m_pObjectAccessor->RemoveObject(item.m_pObject);
    if (res.m_Result.Failed())
      break;
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void xiiQtPropertyPointerWidget::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (!e.m_sParentProperty.IsEqual(m_pProp->GetPropertyName()))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
                       [&](const xiiPropertySelection& sel) { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }))
        return;

      SetSelection(m_Items);
    }
    break;
    default:
      break;
  }
}

/// *** xiiQtEmbeddedClassPropertyWidget ***

xiiQtEmbeddedClassPropertyWidget::xiiQtEmbeddedClassPropertyWidget() :
  xiiQtPropertyWidget()
{
}


xiiQtEmbeddedClassPropertyWidget::~xiiQtEmbeddedClassPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtEmbeddedClassPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler, this));
}

void xiiQtEmbeddedClassPropertyWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtScopedUpdatesDisabled _(this);

  xiiQtPropertyWidget::SetSelection(items);

  // Retrieve the objects the property points to. This could be an embedded class or
  // an element of an array, be it pointer or embedded class.
  m_ResolvedObjects.Clear();
  for (const auto& item : m_Items)
  {
    xiiUuid              ObjectGuid = m_pObjectAccessor->Get<xiiUuid>(item.m_pObject, m_pProp, item.m_Index);
    xiiPropertySelection sel;
    sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
    // sel.m_Index; intentionally invalid as we just retrieved the value so it is a pointer to an object

    m_ResolvedObjects.PushBack(sel);
  }

  m_pResolvedType = m_pProp->GetSpecificType();
  if (m_pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
  {
    m_pResolvedType = xiiQtPropertyWidget::GetCommonBaseType(m_ResolvedObjects);
  }
}

void xiiQtEmbeddedClassPropertyWidget::SetPropertyValue(const xiiAbstractProperty* pProperty, const xiiVariant& NewValue)
{
  xiiStatus res;
  for (const auto& sel : m_ResolvedObjects)
  {
    res = m_pObjectAccessor->SetValue(sel.m_pObject, pProperty, NewValue, sel.m_Index);
    if (res.m_Result.Failed())
      break;
  }
  // xiiPropertyEvent ed;
  // ed.m_Type = xiiPropertyEvent::Type::SingleValueChanged;
  // ed.m_pProperty = pProperty;
  // ed.m_Value = NewValue;
  // ed.m_pItems = &m_ResolvedObjects;

  // m_Events.Broadcast(ed);
}

void xiiQtEmbeddedClassPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtEmbeddedClassPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler, this));
}


void xiiQtEmbeddedClassPropertyWidget::DoPrepareToDie() {}

void xiiQtEmbeddedClassPropertyWidget::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (std::none_of(cbegin(m_ResolvedObjects), cend(m_ResolvedObjects), [=](const xiiPropertySelection& sel) { return e.m_pObject == sel.m_pObject; }))
    return;

  if (!m_QueuedChanges.Contains(e.m_sProperty))
  {
    m_QueuedChanges.PushBack(e.m_sProperty);
  }
}


void xiiQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler(const xiiCommandHistoryEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_Type)
  {
    case xiiCommandHistoryEvent::Type::UndoEnded:
    case xiiCommandHistoryEvent::Type::RedoEnded:
    case xiiCommandHistoryEvent::Type::TransactionEnded:
    case xiiCommandHistoryEvent::Type::TransactionCanceled:
    {
      FlushQueuedChanges();
    }
    break;

    default:
      break;
  }
}

void xiiQtEmbeddedClassPropertyWidget::FlushQueuedChanges()
{
  for (const xiiString& sProperty : m_QueuedChanges)
  {
    OnPropertyChanged(sProperty);
  }

  m_QueuedChanges.Clear();
}

/// *** xiiQtPropertyTypeWidget ***

xiiQtPropertyTypeWidget::xiiQtPropertyTypeWidget(bool bAddCollapsibleGroup) :
  xiiQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
  m_pGroup       = nullptr;
  m_pGroupLayout = nullptr;

  if (bAddCollapsibleGroup)
  {
    m_pGroup       = new xiiQtCollapsibleGroupBox(this);
    m_pGroupLayout = new QHBoxLayout(nullptr);
    m_pGroupLayout->setSpacing(1);
    m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
    m_pGroup->GetContent()->setLayout(m_pGroupLayout);

    m_pLayout->addWidget(m_pGroup);
  }
  m_pTypeWidget = nullptr;
}

xiiQtPropertyTypeWidget::~xiiQtPropertyTypeWidget() = default;

void xiiQtPropertyTypeWidget::OnInit()
{
  if (m_pGroup)
  {
    m_pGroup->SetTitle(xiiTranslate(m_pProp->GetPropertyName()));
    m_pGrid->SetCollapseState(m_pGroup);
    connect(m_pGroup, &xiiQtGroupBoxBase::CollapseStateChanged, m_pGrid, &xiiQtPropertyGridWidget::OnCollapseStateChanged);
  }
}

void xiiQtPropertyTypeWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtScopedUpdatesDisabled _(this);

  xiiQtPropertyWidget::SetSelection(items);

  QHBoxLayout* pLayout = m_pGroup != nullptr ? m_pGroupLayout : m_pLayout;
  QWidget*     pOwner  = m_pGroup != nullptr ? m_pGroup->GetContent() : this;
  if (m_pTypeWidget)
  {
    pLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }

  // Retrieve the objects the property points to. This could be an embedded class or
  // an element of an array, be it pointer or embedded class.
  xiiHybridArray<xiiPropertySelection, 8> ResolvedObjects;
  for (const auto& item : m_Items)
  {
    xiiUuid              ObjectGuid = m_pObjectAccessor->Get<xiiUuid>(item.m_pObject, m_pProp, item.m_Index);
    xiiPropertySelection sel;
    sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
    // sel.m_Index; intentionally invalid as we just retrieved the value so it is a pointer to an object

    ResolvedObjects.PushBack(sel);
  }

  const xiiRTTI* pCommonType = nullptr;
  if (m_pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
  {
    pCommonType = xiiQtPropertyWidget::GetCommonBaseType(ResolvedObjects);
  }
  else
  {
    // If we create a widget for a member class we already determined the common base type at the parent type widget.
    // As we are not dealing with a pointer in this case the type must match the property exactly.
    pCommonType = m_pProp->GetSpecificType();
  }
  m_pTypeWidget = new xiiQtTypeWidget(pOwner, m_pGrid, m_pObjectAccessor, pCommonType, nullptr, nullptr);
  pLayout->addWidget(m_pTypeWidget);
  m_pTypeWidget->SetSelection(ResolvedObjects);
}


void xiiQtPropertyTypeWidget::SetIsDefault(bool bIsDefault)
{
  // The default state set by the parent object / container only refers to the element's correct position in the container but the entire state of the object. As recursively checking an entire object if is has any non-default values is quite costly, we just pretend the object is never in its default state the the user can click revert to default on any object at any time.
  m_bIsDefault = false;
}

void xiiQtPropertyTypeWidget::DoPrepareToDie()
{
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

/// *** xiiQtPropertyContainerWidget ***

xiiQtPropertyContainerWidget::xiiQtPropertyContainerWidget() :
  xiiQtPropertyWidget()
{
  m_Pal = palette();
  setAutoFillBackground(true);

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pGroup       = new xiiQtCollapsibleGroupBox(this);
  m_pGroupLayout = new QVBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->GetContent()->setLayout(m_pGroupLayout);
  m_pGroup->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(m_pGroup, &QWidget::customContextMenuRequested, this, &xiiQtPropertyContainerWidget::OnContainerContextMenu);

  setAcceptDrops(true);
  m_pLayout->addWidget(m_pGroup);
}

xiiQtPropertyContainerWidget::~xiiQtPropertyContainerWidget()
{
  Clear();
}

void xiiQtPropertyContainerWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtPropertyWidget::SetSelection(items);

  UpdateElements();

  if (m_pAddButton)
  {
    m_pAddButton->SetSelection(m_Items);
  }
}

void xiiQtPropertyContainerWidget::SetIsDefault(bool bIsDefault)
{
  // This is called from the type widget which we ignore as we have a tighter scoped default value provider for containers.
}

void xiiQtPropertyContainerWidget::DoPrepareToDie()
{
  for (const auto& e : m_Elements)
  {
    e.m_pWidget->PrepareToDie();
  }
}

void xiiQtPropertyContainerWidget::dragEnterEvent(QDragEnterEvent* event)
{
  updateDropIndex(event);
}

void xiiQtPropertyContainerWidget::dragMoveEvent(QDragMoveEvent* event)
{
  updateDropIndex(event);
}

void xiiQtPropertyContainerWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
  m_iDropSource = -1;
  m_iDropTarget = -1;
  update();
}

void xiiQtPropertyContainerWidget::dropEvent(QDropEvent* event)
{
  if (updateDropIndex(event))
  {
    xiiQtGroupBoxBase* pGroup = qobject_cast<xiiQtGroupBoxBase*>(event->source());
    Element*           pDragElement =
      std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool { return elem.m_pSubGroup == pGroup; });
    if (pDragElement)
    {
      const xiiAbstractProperty*              pProp = pDragElement->m_pWidget->GetProperty();
      xiiHybridArray<xiiPropertySelection, 8> items = pDragElement->m_pWidget->GetSelection();
      if (m_iDropSource != m_iDropTarget && (m_iDropSource + 1) != m_iDropTarget)
      {
        MoveItems(items, m_iDropTarget - m_iDropSource);
      }
    }
  }
  m_iDropSource = -1;
  m_iDropTarget = -1;
  update();
}

void xiiQtPropertyContainerWidget::paintEvent(QPaintEvent* event)
{
  xiiQtPropertyWidget::paintEvent(event);
  if (m_iDropSource != -1 && m_iDropTarget != -1)
  {
    xiiInt32 iYPos = 0;
    if (m_iDropTarget < (xiiInt32)m_Elements.GetCount())
    {
      const QPoint globalPos = m_Elements[m_iDropTarget].m_pSubGroup->mapToGlobal(QPoint(0, 0));
      iYPos                  = mapFromGlobal(globalPos).y();
    }
    else
    {
      const QPoint globalPos = m_Elements[m_Elements.GetCount() - 1].m_pSubGroup->mapToGlobal(QPoint(0, 0));
      iYPos                  = mapFromGlobal(globalPos).y() + m_Elements[m_Elements.GetCount() - 1].m_pSubGroup->height();
    }

    QPainter painter(this);
    painter.setPen(QPen(Qt::PenStyle::NoPen));
    painter.setBrush(palette().brush(QPalette::Highlight));
    painter.drawRect(0, iYPos - 3, width(), 4);
  }
}

void xiiQtPropertyContainerWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  setPalette(m_Pal);
  xiiQtPropertyWidget::showEvent(event);
}

bool xiiQtPropertyContainerWidget::updateDropIndex(QDropEvent* pEvent)
{
  if (pEvent->source() && pEvent->mimeData()->hasFormat("application/x-groupBoxDragProperty"))
  {
    // Is the drop source part of this widget?
    for (xiiUInt32 i = 0; i < m_Elements.GetCount(); i++)
    {
      if (m_Elements[i].m_pSubGroup == pEvent->source())
      {
        pEvent->setDropAction(Qt::MoveAction);
        pEvent->accept();
        xiiInt32 iNewDropTarget = -1;
        // Find closest drop target.
        const xiiInt32 iGlobalYPos = mapToGlobal(pEvent->pos()).y();
        for (xiiUInt32 j = 0; j < m_Elements.GetCount(); j++)
        {
          const QRect rect(m_Elements[j].m_pSubGroup->mapToGlobal(QPoint(0, 0)), m_Elements[j].m_pSubGroup->size());
          if (iGlobalYPos > rect.center().y())
          {
            iNewDropTarget = (xiiInt32)j + 1;
          }
          else if (iGlobalYPos < rect.center().y())
          {
            iNewDropTarget = (xiiInt32)j;
            break;
          }
        }
        if (m_iDropSource != (xiiInt32)i || m_iDropTarget != iNewDropTarget)
        {
          m_iDropSource = (xiiInt32)i;
          m_iDropTarget = iNewDropTarget;
          update();
        }
        return true;
      }
    }
  }

  if (m_iDropSource != -1 || m_iDropTarget != -1)
  {
    m_iDropSource = -1;
    m_iDropTarget = -1;
    update();
  }
  pEvent->ignore();
  return false;
}

void xiiQtPropertyContainerWidget::OnElementButtonClicked()
{
  xiiQtElementGroupButton*                pButton = qobject_cast<xiiQtElementGroupButton*>(sender());
  const xiiAbstractProperty*              pProp   = pButton->GetGroupWidget()->GetProperty();
  xiiHybridArray<xiiPropertySelection, 8> items   = pButton->GetGroupWidget()->GetSelection();

  switch (pButton->GetAction())
  {
    case xiiQtElementGroupButton::ElementAction::MoveElementUp:
    {
      MoveItems(items, -1);
    }
    break;
    case xiiQtElementGroupButton::ElementAction::MoveElementDown:
    {
      MoveItems(items, 2);
    }
    break;
    case xiiQtElementGroupButton::ElementAction::DeleteElement:
    {
      DeleteItems(items);
    }
    break;

    case xiiQtElementGroupButton::ElementAction::Help:
      // handled by custom lambda
      break;
  }
}

void xiiQtPropertyContainerWidget::OnDragStarted(QMimeData& ref_mimeData)
{
  xiiQtGroupBoxBase* pGroup = qobject_cast<xiiQtGroupBoxBase*>(sender());
  Element*           pDragElement =
    std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool { return elem.m_pSubGroup == pGroup; });
  if (pDragElement)
  {
    ref_mimeData.setData("application/x-groupBoxDragProperty", QByteArray());
  }
}

void xiiQtPropertyContainerWidget::OnContainerContextMenu(const QPoint& pt)
{
  xiiQtGroupBoxBase* pGroup = qobject_cast<xiiQtGroupBoxBase*>(sender());

  QMenu m;
  m.setToolTipsVisible(true);
  ExtendContextMenu(m);

  if (!m.isEmpty())
  {
    m.exec(pGroup->mapToGlobal(pt));
  }
}

void xiiQtPropertyContainerWidget::OnCustomElementContextMenu(const QPoint& pt)
{
  xiiQtGroupBoxBase* pGroup   = qobject_cast<xiiQtGroupBoxBase*>(sender());
  Element*           pElement = std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool { return elem.m_pSubGroup == pGroup; });

  if (pElement)
  {
    QMenu m;
    m.setToolTipsVisible(true);
    pElement->m_pWidget->ExtendContextMenu(m);

    m_pGrid->ExtendContextMenu(m, pElement->m_pWidget->GetSelection(), pElement->m_pWidget->GetProperty());

    if (!m.isEmpty())
    {
      m.exec(pGroup->mapToGlobal(pt));
    }
  }
}

xiiQtGroupBoxBase* xiiQtPropertyContainerWidget::CreateElement(QWidget* pParent)
{
  auto pBox = new xiiQtCollapsibleGroupBox(pParent);
  pBox->SetFillColor(palette().window().color());
  return pBox;
}

xiiQtPropertyWidget* xiiQtPropertyContainerWidget::CreateWidget(xiiUInt32 index)
{
  return new xiiQtPropertyTypeWidget();
}

xiiQtPropertyContainerWidget::Element& xiiQtPropertyContainerWidget::AddElement(xiiUInt32 index)
{
  xiiQtGroupBoxBase* pSubGroup = CreateElement(m_pGroup);
  pSubGroup->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(pSubGroup, &xiiQtGroupBoxBase::CollapseStateChanged, m_pGrid, &xiiQtPropertyGridWidget::OnCollapseStateChanged);
  connect(pSubGroup, &QWidget::customContextMenuRequested, this, &xiiQtPropertyContainerWidget::OnCustomElementContextMenu);

  QVBoxLayout* pSubLayout = new QVBoxLayout(nullptr);
  pSubLayout->setContentsMargins(5, 0, 0, 0);
  pSubLayout->setSpacing(1);
  pSubGroup->GetContent()->setLayout(pSubLayout);

  m_pGroupLayout->insertWidget((int)index, pSubGroup);

  xiiQtPropertyWidget* pNewWidget = CreateWidget(index);

  pNewWidget->setParent(pSubGroup);
  pSubLayout->addWidget(pNewWidget);

  pNewWidget->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);

  // Add Buttons
  auto pAttr = m_pProp->GetAttributeByType<xiiContainerAttribute>();
  if ((!pAttr || pAttr->CanMove()) && m_pProp->GetCategory() != xiiPropertyCategory::Map)
  {
    pSubGroup->SetDraggable(true);
    connect(pSubGroup, &xiiQtGroupBoxBase::DragStarted, this, &xiiQtPropertyContainerWidget::OnDragStarted);
  }

  xiiQtElementGroupButton* pHelpButton = new xiiQtElementGroupButton(pSubGroup->GetHeader(), xiiQtElementGroupButton::ElementAction::Help, pNewWidget);
  pSubGroup->GetHeader()->layout()->addWidget(pHelpButton);
  pHelpButton->setVisible(false); // added now, and shown later when we know the URL

  if (!pAttr || pAttr->CanDelete())
  {
    xiiQtElementGroupButton* pDeleteButton =
      new xiiQtElementGroupButton(pSubGroup->GetHeader(), xiiQtElementGroupButton::ElementAction::DeleteElement, pNewWidget);
    pSubGroup->GetHeader()->layout()->addWidget(pDeleteButton);
    connect(pDeleteButton, &QToolButton::clicked, this, &xiiQtPropertyContainerWidget::OnElementButtonClicked);
  }

  m_Elements.Insert(Element(pSubGroup, pNewWidget, pHelpButton), index);
  return m_Elements[index];
}

void xiiQtPropertyContainerWidget::RemoveElement(xiiUInt32 index)
{
  Element& elem = m_Elements[index];

  m_pGroupLayout->removeWidget(elem.m_pSubGroup);
  delete elem.m_pSubGroup;
  m_Elements.RemoveAtAndCopy(index);
}

void xiiQtPropertyContainerWidget::UpdateElements()
{
  xiiQtScopedUpdatesDisabled _(this);

  xiiUInt32 iElements = GetRequiredElementCount();

  while (m_Elements.GetCount() > iElements)
  {
    RemoveElement(m_Elements.GetCount() - 1);
  }
  while (m_Elements.GetCount() < iElements)
  {
    AddElement(m_Elements.GetCount());
  }

  for (xiiUInt32 i = 0; i < iElements; ++i)
  {
    UpdateElement(i);
  }

  UpdatePropertyMetaState();

  // Force re-layout of parent hierarchy to prevent flicker.
  QWidget* pCur = m_pGroup;
  while (pCur != nullptr && qobject_cast<QScrollArea*>(pCur) == nullptr)
  {
    pCur->updateGeometry();
    pCur = pCur->parentWidget();
  }
}

xiiUInt32 xiiQtPropertyContainerWidget::GetRequiredElementCount() const
{
  if (m_pProp->GetCategory() == xiiPropertyCategory::Map)
  {
    m_Keys.Clear();
    XII_VERIFY(m_pObjectAccessor->GetKeys(m_Items[0].m_pObject, m_pProp, m_Keys).m_Result.Succeeded(), "GetKeys should always succeed.");
    xiiHybridArray<xiiVariant, 16> keys;
    for (xiiUInt32 i = 1; i < m_Items.GetCount(); i++)
    {
      keys.Clear();
      XII_VERIFY(m_pObjectAccessor->GetKeys(m_Items[i].m_pObject, m_pProp, keys).m_Result.Succeeded(), "GetKeys should always succeed.");
      for (xiiInt32 k = (xiiInt32)m_Keys.GetCount() - 1; k >= 0; --k)
      {
        if (!keys.Contains(m_Keys[k]))
        {
          m_Keys.RemoveAtAndSwap(k);
        }
      }
    }
    m_Keys.Sort([](const xiiVariant& a, const xiiVariant& b) { return a.Get<xiiString>().Compare(b.Get<xiiString>()) < 0; });
    return m_Keys.GetCount();
  }
  else
  {
    xiiInt32 iElements = 0x7FFFFFFF;
    for (const auto& item : m_Items)
    {
      xiiInt32 iCount = 0;
      XII_VERIFY(m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).m_Result.Succeeded(), "GetCount should always succeed.");
      iElements = xiiMath::Min(iElements, iCount);
    }
    XII_ASSERT_DEV(iElements >= 0, "Mismatch between storage and RTTI ({0})", iElements);
    m_Keys.Clear();
    for (xiiUInt32 i = 0; i < (xiiUInt32)iElements; i++)
    {
      m_Keys.PushBack(i);
    }

    return xiiUInt32(iElements);
  }
}

void xiiQtPropertyContainerWidget::UpdatePropertyMetaState()
{
  xiiPropertyMetaState*                        pMeta = xiiPropertyMetaState::GetSingleton();
  xiiHashTable<xiiVariant, xiiPropertyUiState> ElementStates;
  pMeta->GetContainerElementsState(m_Items, m_pProp->GetPropertyName(), ElementStates);

  xiiDefaultContainerState defaultState(m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
  m_bIsDefault = defaultState.IsDefaultContainer();
  m_pGroup->SetBoldTitle(!m_bIsDefault);

  QColor qColor = xiiQtPropertyWidget::SetPaletteBackgroundColor(defaultState.GetBackgroundColor(), m_Pal);
  setPalette(m_Pal);

  const bool bReadOnly = m_pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) ||
    (m_pProp->GetAttributeByType<xiiReadOnlyAttribute>() != nullptr);
  for (xiiUInt32 i = 0; i < m_Elements.GetCount(); i++)
  {
    Element&                       element    = m_Elements[i];
    xiiVariant&                    key        = m_Keys[i];
    const bool                     bIsDefault = defaultState.IsDefaultElement(key);
    auto                           itData     = ElementStates.Find(key);
    xiiPropertyUiState::Visibility state      = xiiPropertyUiState::Default;
    if (itData.IsValid())
    {
      state = itData.Value().m_Visibility;
    }

    if (element.m_pSubGroup)
    {
      element.m_pSubGroup->setVisible(state != xiiPropertyUiState::Invisible);
      element.m_pSubGroup->setEnabled(!bReadOnly && state != xiiPropertyUiState::Disabled);
      element.m_pSubGroup->SetBoldTitle(!bIsDefault);

      // If the fill color is invalid that means no border is drawn and we don't want to change the color then.
      if (element.m_pSubGroup->GetFillColor().isValid())
      {
        element.m_pSubGroup->SetFillColor(qColor);
      }
    }
    if (element.m_pWidget)
    {
      element.m_pWidget->setVisible(state != xiiPropertyUiState::Invisible);
      element.m_pSubGroup->setEnabled(!bReadOnly && state != xiiPropertyUiState::Disabled);
      element.m_pWidget->SetIsDefault(bIsDefault);
    }
  }
}

void xiiQtPropertyContainerWidget::Clear()
{
  while (m_Elements.GetCount() > 0)
  {
    RemoveElement(m_Elements.GetCount() - 1);
  }

  m_Elements.Clear();
}

void xiiQtPropertyContainerWidget::OnInit()
{
  xiiStringBuilder fullname(m_pType->GetTypeName(), "::", m_pProp->GetPropertyName());

  m_pGroup->SetTitle(xiiTranslate(fullname));

  const xiiContainerAttribute* pArrayAttr = m_pProp->GetAttributeByType<xiiContainerAttribute>();
  if (!pArrayAttr || pArrayAttr->CanAdd())
  {
    m_pAddButton = new xiiQtAddSubElementButton();
    m_pAddButton->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);
    m_pGroup->GetHeader()->layout()->addWidget(m_pAddButton);
  }

  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &xiiQtGroupBoxBase::CollapseStateChanged, m_pGrid, &xiiQtPropertyGridWidget::OnCollapseStateChanged);
}

void xiiQtPropertyContainerWidget::DeleteItems(xiiHybridArray<xiiPropertySelection, 8>& items)
{
  m_pObjectAccessor->StartTransaction("Delete Object");

  xiiStatus  res(XII_SUCCESS);
  const bool bIsValueType = xiiReflectionUtils::IsValueType(m_pProp);

  if (bIsValueType)
  {
    for (auto& item : items)
    {
      res = m_pObjectAccessor->RemoveValue(item.m_pObject, m_pProp, item.m_Index);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    xiiRemoveObjectCommand cmd;

    for (auto& item : items)
    {
      xiiUuid                  value   = m_pObjectAccessor->Get<xiiUuid>(item.m_pObject, m_pProp, item.m_Index);
      const xiiDocumentObject* pObject = m_pObjectAccessor->GetObject(value);
      res                              = m_pObjectAccessor->RemoveObject(pObject);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void xiiQtPropertyContainerWidget::MoveItems(xiiHybridArray<xiiPropertySelection, 8>& items, xiiInt32 iMove)
{
  XII_ASSERT_DEV(m_pProp->GetCategory() != xiiPropertyCategory::Map, "Map entries can't be moved.");

  m_pObjectAccessor->StartTransaction("Reparent Object");

  xiiStatus  res(XII_SUCCESS);
  const bool bIsValueType = xiiReflectionUtils::IsValueType(m_pProp);
  if (bIsValueType)
  {
    for (auto& item : items)
    {
      xiiInt32 iCurIndex = item.m_Index.ConvertTo<xiiInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > m_pObjectAccessor->GetCount(item.m_pObject, m_pProp))
        continue;

      res = m_pObjectAccessor->MoveValue(item.m_pObject, m_pProp, item.m_Index, iCurIndex);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    xiiMoveObjectCommand cmd;

    for (auto& item : items)
    {
      xiiInt32 iCurIndex = item.m_Index.ConvertTo<xiiInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > m_pObjectAccessor->GetCount(item.m_pObject, m_pProp))
        continue;

      xiiUuid                  value   = m_pObjectAccessor->Get<xiiUuid>(item.m_pObject, m_pProp, item.m_Index);
      const xiiDocumentObject* pObject = m_pObjectAccessor->GetObject(value);

      res = m_pObjectAccessor->MoveObject(pObject, item.m_pObject, m_pProp, iCurIndex);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Moving sub-element failed.");
}


/// *** xiiQtPropertyStandardTypeContainerWidget ***

xiiQtPropertyStandardTypeContainerWidget::xiiQtPropertyStandardTypeContainerWidget() :
  xiiQtPropertyContainerWidget()
{
}

xiiQtPropertyStandardTypeContainerWidget::~xiiQtPropertyStandardTypeContainerWidget() = default;

xiiQtGroupBoxBase* xiiQtPropertyStandardTypeContainerWidget::CreateElement(QWidget* pParent)
{
  auto* pBox = new xiiQtInlinedGroupBox(pParent);
  pBox->SetFillColor(QColor::Invalid);
  return pBox;
}


xiiQtPropertyWidget* xiiQtPropertyStandardTypeContainerWidget::CreateWidget(xiiUInt32 index)
{
  return xiiQtPropertyGridWidget::CreateMemberPropertyWidget(m_pProp);
}

xiiQtPropertyContainerWidget::Element& xiiQtPropertyStandardTypeContainerWidget::AddElement(xiiUInt32 index)
{
  xiiQtPropertyContainerWidget::Element& elem = xiiQtPropertyContainerWidget::AddElement(index);
  return elem;
}

void xiiQtPropertyStandardTypeContainerWidget::RemoveElement(xiiUInt32 index)
{
  xiiQtPropertyContainerWidget::RemoveElement(index);
}

void xiiQtPropertyStandardTypeContainerWidget::UpdateElement(xiiUInt32 index)
{
  Element& elem = m_Elements[index];

  xiiHybridArray<xiiPropertySelection, 8> SubItems;

  for (const auto& item : m_Items)
  {
    xiiPropertySelection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index   = m_Keys[index];

    SubItems.PushBack(sel);
  }

  xiiStringBuilder sTitle;
  if (m_pProp->GetCategory() == xiiPropertyCategory::Map)
    sTitle.Format("{0}", m_Keys[index].ConvertTo<xiiString>());
  else
    sTitle.Format("[{0}]", m_Keys[index].ConvertTo<xiiString>());

  elem.m_pSubGroup->SetTitle(sTitle);
  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

/// *** xiiQtPropertyTypeContainerWidget ***

xiiQtPropertyTypeContainerWidget::xiiQtPropertyTypeContainerWidget() = default;

xiiQtPropertyTypeContainerWidget::~xiiQtPropertyTypeContainerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    xiiMakeDelegate(&xiiQtPropertyTypeContainerWidget::StructureEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyTypeContainerWidget::CommandHistoryEventHandler, this));
}

void xiiQtPropertyTypeContainerWidget::OnInit()
{
  xiiQtPropertyContainerWidget::OnInit();
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    xiiMakeDelegate(&xiiQtPropertyTypeContainerWidget::StructureEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyTypeContainerWidget::CommandHistoryEventHandler, this));
}

void xiiQtPropertyTypeContainerWidget::UpdateElement(xiiUInt32 index)
{
  Element&                                elem = m_Elements[index];
  xiiHybridArray<xiiPropertySelection, 8> SubItems;

  // To be in line with all other xiiQtPropertyWidget the container element will
  // be given a selection in the form of this is the parent object, this is the property and in this
  // specific case this is the index you are working on. So SubItems only decorates the items with the correct index.
  for (const auto& item : m_Items)
  {
    xiiPropertySelection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index   = m_Keys[index];

    SubItems.PushBack(sel);
  }

  {
    // To get the correct name we actually need to resolve the selection to the actual objects
    // they are pointing to.
    xiiHybridArray<xiiPropertySelection, 8> ResolvedObjects;
    for (const auto& item : SubItems)
    {
      xiiUuid              ObjectGuid = m_pObjectAccessor->Get<xiiUuid>(item.m_pObject, m_pProp, item.m_Index);
      xiiPropertySelection sel;
      sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
      ResolvedObjects.PushBack(sel);
    }

    const xiiRTTI* pCommonType = xiiQtPropertyWidget::GetCommonBaseType(ResolvedObjects);

    // Label
    {
      xiiStringBuilder sTitle;
      sTitle.Format("[{0}] - {1}", m_Keys[index].ConvertTo<xiiString>(), xiiTranslate(pCommonType->GetTypeName()));

      if (auto pInDev = pCommonType->GetAttributeByType<xiiInDevelopmentAttribute>())
      {
        sTitle.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      elem.m_pSubGroup->SetTitle(sTitle);
    }

    // Icon
    {
      xiiStringBuilder sIconName;
      sIconName.Set(":/TypeIcons/", pCommonType->GetTypeName());
      elem.m_pSubGroup->SetIcon(xiiQtUiServices::GetCachedIconResource(sIconName.GetData()));
    }

    // help URL
    {
      QString url = xiiTranslateHelpURL(pCommonType->GetTypeName());

      if (!url.isEmpty())
      {
        elem.m_pHelpButton->setVisible(true);
        connect(elem.m_pHelpButton, &QToolButton::clicked, this, [=]() {
          QDesktopServices::openUrl(QUrl(url));
        });
      }
      else
      {
        elem.m_pHelpButton->setVisible(false);
      }
    }
  }


  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

void xiiQtPropertyTypeContainerWidget::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (!e.m_sParentProperty.IsEqual(m_pProp->GetPropertyName()))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
                       [&](const xiiPropertySelection& sel) { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }))
        return;

      m_bNeedsUpdate = true;
    }
    break;
    default:
      break;
  }
}

void xiiQtPropertyTypeContainerWidget::CommandHistoryEventHandler(const xiiCommandHistoryEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_Type)
  {
    case xiiCommandHistoryEvent::Type::UndoEnded:
    case xiiCommandHistoryEvent::Type::RedoEnded:
    case xiiCommandHistoryEvent::Type::TransactionEnded:
    case xiiCommandHistoryEvent::Type::TransactionCanceled:
    {
      if (m_bNeedsUpdate)
      {
        m_bNeedsUpdate = false;
        UpdateElements();
      }
    }
    break;

    default:
      break;
  }
}

/// *** xiiQtVariantPropertyWidget ***

xiiQtVariantPropertyWidget::xiiQtVariantPropertyWidget()
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 4);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  m_pTypeList = new QComboBox(this);
  m_pTypeList->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pTypeList);
}

xiiQtVariantPropertyWidget::~xiiQtVariantPropertyWidget() = default;

void xiiQtVariantPropertyWidget::OnInit()
{
  xiiStringBuilder sName;
  for (int i = xiiVariantType::Invalid; i < xiiVariantType::LastExtendedType; ++i)
  {
    auto type = static_cast<xiiVariantType::Enum>(i);
    if (GetVariantTypeDisplayName(type, sName).Succeeded())
    {
      m_pTypeList->addItem(xiiTranslate(sName), i);
    }
  }

  connect(m_pTypeList, &QComboBox::currentIndexChanged, [this](int iIndex) {
    ChangeVariantType(static_cast<xiiVariantType::Enum>(m_pTypeList->itemData(iIndex).toInt()));
  });
}

void xiiQtVariantPropertyWidget::InternalSetValue(const xiiVariant& value)
{
  xiiVariantType::Enum commonType   = xiiVariantType::Invalid;
  const bool           sameType     = GetCommonVariantSubType(m_Items, m_pProp, commonType);
  const xiiRTTI*       pNewtSubType = commonType != xiiVariantType::Invalid ? xiiReflectionUtils::GetTypeFromVariant(commonType) : nullptr;
  if (pNewtSubType != m_pCurrentSubType || m_pWidget == nullptr)
  {
    if (m_pWidget)
    {
      m_pWidget->PrepareToDie();
      m_pWidget->deleteLater();
      m_pWidget = nullptr;
    }
    m_pCurrentSubType = pNewtSubType;
    if (pNewtSubType)
    {
      m_pWidget = xiiQtPropertyGridWidget::GetFactory().CreateObject(pNewtSubType);
      if (!m_pWidget)
        m_pWidget = new xiiQtUnsupportedPropertyWidget("Unsupported type");
    }
    else if (!sameType)
    {
      m_pWidget = new xiiQtUnsupportedPropertyWidget("Multi-selection has varying types");
    }
    else
    {
      m_pWidget = new xiiQtUnsupportedPropertyWidget("<Invalid>");
    }
    m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_pWidget->setParent(this);
    m_pLayout->addWidget(m_pWidget);
    m_pWidget->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);

    UpdateTypeListSelection(commonType);
  }
  m_pWidget->SetSelection(m_Items);
}

void xiiQtVariantPropertyWidget::DoPrepareToDie()
{
  if (m_pWidget)
    m_pWidget->PrepareToDie();
}

void xiiQtVariantPropertyWidget::UpdateTypeListSelection(xiiVariantType::Enum type)
{
  xiiQtScopedBlockSignals bs(m_pTypeList);
  for (int i = 0; i < m_pTypeList->count(); ++i)
  {
    if (m_pTypeList->itemData(i).toInt() == type)
    {
      m_pTypeList->setCurrentIndex(i);
      break;
    }
  }
}

void xiiQtVariantPropertyWidget::ChangeVariantType(xiiVariantType::Enum type)
{
  m_pObjectAccessor->StartTransaction("Change variant type");

  // check if we have multiple values
  for (const auto& item : m_Items)
  {
    xiiVariant value;
    XII_VERIFY(m_pObjectAccessor->GetValue(item.m_pObject, m_pProp, value, item.m_Index).Succeeded(), "");
    if (value.CanConvertTo(type))
    {
      XII_VERIFY(m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), item.m_Index).Succeeded(), "");
    }
    else
    {
      XII_VERIFY(m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, xiiReflectionUtils::GetDefaultVariantFromType(type), item.m_Index).Succeeded(), "");
    }
  }
  m_pObjectAccessor->FinishTransaction();
}

xiiResult xiiQtVariantPropertyWidget::GetVariantTypeDisplayName(xiiVariantType::Enum type, xiiStringBuilder& out_sName) const
{
  if (type == xiiVariantType::FirstStandardType || type == xiiVariantType::StringView || type == xiiVariantType::DataBuffer || type >= xiiVariantType::LastStandardType)
    return XII_FAILURE;

  const xiiRTTI* pVariantEnum = xiiGetStaticRTTI<xiiVariantType>();
  if (xiiReflectionUtils::EnumerationToString(pVariantEnum, type, out_sName) == false)
    return XII_FAILURE;

  return XII_SUCCESS;
}
