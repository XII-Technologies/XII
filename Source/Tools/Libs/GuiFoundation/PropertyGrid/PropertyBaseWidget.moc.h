/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyEventHandler.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

#include <QWidget>

class xiiDocumentObject;
class xiiQtTypeWidget;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QMenu;
class QComboBox;
class xiiQtGroupBoxBase;
class xiiQtAddSubElementButton;
class xiiQtPropertyGridWidget;
class xiiQtElementGroupButton;
class QMimeData;
struct xiiCommandHistoryEvent;
class xiiObjectAccessorBase;

/// Base class for all property widgets
class XII_GUIFOUNDATION_DLL xiiQtPropertyWidget : public QWidget
{
  Q_OBJECT;

public:
  explicit xiiQtPropertyWidget();
  virtual ~xiiQtPropertyWidget();

  void                       Init(xiiQtPropertyGridWidget* pGrid, xiiObjectAccessorBase* pObjectAccessor, const xiiRTTI* pType, const xiiAbstractProperty* pProp);
  const xiiAbstractProperty* GetProperty() const { return m_pProp; }

  /// This is called whenever the selection in the editor changes and thus the widget may need to display a different value.
  ///
  /// If the array holds more than one element, the user selected multiple objects. In this case, the code should check whether
  /// the values differ across the selected objects and if so, the widget should display "multiple values".
  virtual void                                   SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items);
  const xiiHybridArray<xiiPropertySelection, 8>& GetSelection() const { return m_Items; }

  /// If this returns true (default), a QLabel is created and the text that GetLabel() returns is displayed.
  virtual bool HasLabel() const { return true; }

  /// The return value is used to display a label, if HasLabel() returns true.
  virtual const char* GetLabel(xiiStringBuilder& ref_sTmp) const;

  virtual void ExtendContextMenu(QMenu& ref_menu);

  /// Whether the variable that the widget represents is currently set to the default value or has been modified.
  virtual void SetIsDefault(bool bIsDefault) { m_bIsDefault = bIsDefault; }

  /// If the property is of type xiiVariant this function returns whether all items have the same type.
  /// If true is returned, out_Type contains the common type. Note that 'invalid' can be a common type.
  bool GetCommonVariantSubType(const xiiHybridArray<xiiPropertySelection, 8>& items, const xiiAbstractProperty* pProperty, xiiVariantType::Enum& out_type);

  xiiVariant GetCommonValue(const xiiHybridArray<xiiPropertySelection, 8>& items, const xiiAbstractProperty* pProperty);
  void       PrepareToDie();

  /// By default disables the widget, but can be overridden to make a widget more interactable (for example to be able to copy text from it).
  virtual void SetReadOnly(bool bReadOnly = true);

public:
  static const xiiRTTI* GetCommonBaseType(const xiiHybridArray<xiiPropertySelection, 8>& items);
  static QColor         SetPaletteBackgroundColor(xiiColorGammaUB inputColor, QPalette& ref_palette);

public Q_SLOTS:
  void OnCustomContextMenu(const QPoint& pt);

protected:
  void Broadcast(xiiPropertyEvent::Type type);
  void PropertyChangedHandler(const xiiPropertyEvent& ed);

  virtual void OnInit() = 0;
  bool         IsUndead() const { return m_bUndead; }

protected:
  virtual void DoPrepareToDie() = 0;

  virtual bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

  xiiQtPropertyGridWidget*                m_pGrid           = nullptr;
  xiiObjectAccessorBase*                  m_pObjectAccessor = nullptr;
  const xiiRTTI*                          m_pType           = nullptr;
  const xiiAbstractProperty*              m_pProp           = nullptr;
  xiiHybridArray<xiiPropertySelection, 8> m_Items;
  bool                                    m_bIsDefault; ///< Whether the variable that the widget represents is currently set to the default value or has been modified.

private:
  bool m_bUndead; ///< Widget is being destroyed
};


/// Fallback widget for all property types for which no other widget type is registered
class XII_GUIFOUNDATION_DLL xiiQtUnsupportedPropertyWidget : public xiiQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit xiiQtUnsupportedPropertyWidget(xiiStringView sMessage = {});

protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override {}

  QHBoxLayout* m_pLayout;
  QLabel*      m_pWidget;
  xiiString    m_sMessage;
};


/// Base class for most 'simple' property type widgets. Implements some of the standard functionality.
class XII_GUIFOUNDATION_DLL xiiQtStandardPropertyWidget : public xiiQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit xiiQtStandardPropertyWidget();

  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;

protected:
  void         BroadcastValueChanged(const xiiVariant& NewValue);
  virtual void DoPrepareToDie() override {}

  const xiiVariant& GetOldValue() const { return m_OldValue; }
  virtual void      InternalSetValue(const xiiVariant& value) = 0;

protected:
  xiiVariant m_OldValue;
};


/// Base class for more 'advanced' property type widgets for Pointer or Class type properties.
/// Implements some of xiiQtTypeWidget functionality at property widget level.
class XII_GUIFOUNDATION_DLL xiiQtEmbeddedClassPropertyWidget : public xiiQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit xiiQtEmbeddedClassPropertyWidget();
  ~xiiQtEmbeddedClassPropertyWidget();

  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;

protected:
  void SetPropertyValue(const xiiAbstractProperty* pProperty, const xiiVariant& NewValue);

  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  virtual void OnPropertyChanged(const xiiString& sProperty) = 0;

private:
  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);
  void FlushQueuedChanges();

protected:
  bool                                    m_bTemporaryCommand = false;
  const xiiRTTI*                          m_pResolvedType     = nullptr;
  xiiHybridArray<xiiPropertySelection, 8> m_ResolvedObjects;

  xiiHybridArray<xiiString, 1> m_QueuedChanges;
};


/// Used for pointers and embedded classes.
/// Does not inherit from xiiQtEmbeddedClassPropertyWidget as it just embeds
/// a xiiQtTypeWidget for the property's value which handles everything already.
class XII_GUIFOUNDATION_DLL xiiQtPropertyTypeWidget : public xiiQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit xiiQtPropertyTypeWidget(bool bAddCollapsibleGroup = false);
  virtual ~xiiQtPropertyTypeWidget();

  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }
  virtual void SetIsDefault(bool bIsDefault) override;

protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;

protected:
  QVBoxLayout*       m_pLayout;
  xiiQtGroupBoxBase* m_pGroup;
  QVBoxLayout*       m_pGroupLayout;
  xiiQtTypeWidget*   m_pTypeWidget;
};

/// Used for property types that are pointers.
class XII_GUIFOUNDATION_DLL xiiQtPropertyPointerWidget : public xiiQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit xiiQtPropertyPointerWidget();
  virtual ~xiiQtPropertyPointerWidget();

  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }


public Q_SLOTS:
  void OnDeleteButtonClicked();

protected:
  virtual void OnInit() override;
  void         StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  virtual void DoPrepareToDie() override;
  void         UpdateTitle(const xiiRTTI* pType = nullptr);

protected:
  QHBoxLayout*              m_pLayout       = nullptr;
  xiiQtGroupBoxBase*        m_pGroup        = nullptr;
  xiiQtAddSubElementButton* m_pAddButton    = nullptr;
  xiiQtElementGroupButton*  m_pDeleteButton = nullptr;
  QHBoxLayout*              m_pGroupLayout  = nullptr;
  xiiQtTypeWidget*          m_pTypeWidget   = nullptr;
};


/// Base class for all container properties
class XII_GUIFOUNDATION_DLL xiiQtPropertyContainerWidget : public xiiQtPropertyWidget
{
  Q_OBJECT;

public:
  xiiQtPropertyContainerWidget();
  virtual ~xiiQtPropertyContainerWidget();

  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }
  virtual void SetIsDefault(bool bIsDefault) override;

public Q_SLOTS:
  void OnElementButtonClicked();
  void OnDragStarted(QMimeData& ref_mimeData);
  void OnContainerContextMenu(const QPoint& pt);
  void OnCustomElementContextMenu(const QPoint& pt);

protected:
  struct Element
  {
    Element() = default;

    Element(xiiQtGroupBoxBase* pSubGroup, xiiQtPropertyWidget* pWidget, xiiQtElementGroupButton* pHelpButton) :
      m_pSubGroup(pSubGroup), m_pWidget(pWidget), m_pHelpButton(pHelpButton)
    {
    }

    xiiQtGroupBoxBase*       m_pSubGroup   = nullptr;
    xiiQtPropertyWidget*     m_pWidget     = nullptr;
    xiiQtElementGroupButton* m_pHelpButton = nullptr;
  };

  virtual xiiQtGroupBoxBase*   CreateElement(QWidget* pParent);
  virtual xiiQtPropertyWidget* CreateWidget(xiiUInt32 index);
  virtual Element&             AddElement(xiiUInt32 index);
  virtual void                 RemoveElement(xiiUInt32 index);
  virtual void                 UpdateElement(xiiUInt32 index) = 0;
  void                         UpdateElements();
  virtual void                 GetRequiredElements(xiiDynamicArray<xiiVariant>& out_keys) const;
  virtual void                 UpdatePropertyMetaState();
  /// Some containers like xiiVariant can be both a map or an array so we can't reply on the property type alone. For these containers, this method can be overwritten to retrieve the category from something other than `m_pProp->GetCategory()`.
  virtual xiiPropertyCategory::Enum GetContainerCategory() const;

  void         Clear();
  virtual void OnInit() override;

  void         DeleteItems(xiiHybridArray<xiiPropertySelection, 8>& items);
  void         MoveItems(xiiHybridArray<xiiPropertySelection, 8>& items, xiiInt32 iMove);
  virtual void DoPrepareToDie() override;
  virtual void dragEnterEvent(QDragEnterEvent* event) override;
  virtual void dragMoveEvent(QDragMoveEvent* event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* event) override;
  virtual void dropEvent(QDropEvent* event) override;
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void showEvent(QShowEvent* event) override;

private:
  bool updateDropIndex(QDropEvent* pEvent);

protected:
  QHBoxLayout*              m_pLayout;
  xiiQtGroupBoxBase*        m_pGroup;
  QVBoxLayout*              m_pGroupLayout;
  xiiQtAddSubElementButton* m_pAddButton = nullptr;
  QPalette                  m_Pal;

  xiiHybridArray<xiiVariant, 16> m_Keys;
  xiiDynamicArray<Element>       m_Elements;
  xiiInt32                       m_iDropSource = -1;
  xiiInt32                       m_iDropTarget = -1;
};


class XII_GUIFOUNDATION_DLL xiiQtPropertyStandardTypeContainerWidget : public xiiQtPropertyContainerWidget
{
  Q_OBJECT;

public:
  xiiQtPropertyStandardTypeContainerWidget();
  virtual ~xiiQtPropertyStandardTypeContainerWidget();

protected:
  virtual xiiQtGroupBoxBase*   CreateElement(QWidget* pParent) override;
  virtual xiiQtPropertyWidget* CreateWidget(xiiUInt32 index) override;
  virtual Element&             AddElement(xiiUInt32 index) override;
  virtual void                 RemoveElement(xiiUInt32 index) override;
  virtual void                 UpdateElement(xiiUInt32 index) override;
};

class XII_GUIFOUNDATION_DLL xiiQtPropertyTypeContainerWidget : public xiiQtPropertyContainerWidget
{
  Q_OBJECT;

public:
  xiiQtPropertyTypeContainerWidget();
  virtual ~xiiQtPropertyTypeContainerWidget();

protected:
  virtual void OnInit() override;
  virtual void UpdateElement(xiiUInt32 index) override;

  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);

private:
  bool m_bNeedsUpdate = false;
};

class XII_GUIFOUNDATION_DLL xiiQtVariantPropertyWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT;

public:
  xiiQtVariantPropertyWidget();
  virtual ~xiiQtVariantPropertyWidget();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;
  virtual void DoPrepareToDie() override;
  void         UpdateTypeListSelection(xiiVariantType::Enum type);
  void         ChangeVariantType(xiiVariantType::Enum type);
  void         EnableTypeSelection(bool bEnable);

  virtual xiiResult GetVariantTypeDisplayName(xiiVariantType::Enum type, xiiStringBuilder& out_sName) const;

protected:
  QVBoxLayout*         m_pLayout         = nullptr;
  QComboBox*           m_pTypeList       = nullptr;
  xiiQtPropertyWidget* m_pWidget         = nullptr;
  const xiiRTTI*       m_pCurrentSubType = nullptr;
};

// Used for sub-containers of an xiiVariant, e.g. an xiiVariantArray or xiiVariantDictionary stored inside an xiiVariant. xiiVariantSubAccessor is used to create a view into a sub-tree container of the xiiVariant.
class XII_GUIFOUNDATION_DLL xiiQtVariantContainerWidget : public xiiQtPropertyStandardTypeContainerWidget
{
  Q_OBJECT;

public:
  xiiQtVariantContainerWidget(xiiVariantType::Enum variantType);
  virtual ~xiiQtVariantContainerWidget() = default;

protected:
  virtual void                      OnInit() override;
  virtual void                      SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;
  virtual xiiPropertyCategory::Enum GetContainerCategory() const override;

private:
  xiiUniquePtr<xiiVariantSubAccessor> m_pVariantSubAccessor;
  xiiEnum<xiiPropertyCategory>        m_ContainerCategory;
};
