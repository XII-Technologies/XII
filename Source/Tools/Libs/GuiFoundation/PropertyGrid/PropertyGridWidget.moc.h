#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

class QSpacerItem;
class QVBoxLayout;
class QScrollArea;

class xiiQtGroupBoxBase;
class xiiDocument;
class xiiDocumentObjectManager;
class xiiCommandHistory;
class xiiObjectAccessorBase;
struct xiiDocumentObjectPropertyEvent;
struct xiiPropertyMetaStateEvent;
struct xiiObjectAccessorChangeEvent;
struct xiiPropertyDefaultEvent;
struct xiiContainerElementMetaStateEvent;

class XII_GUIFOUNDATION_DLL xiiQtPropertyGridWidget : public QWidget
{
  Q_OBJECT
public:
  xiiQtPropertyGridWidget(QWidget* pParent, xiiDocument* pDocument = nullptr, bool bBindToSelectionManager = true);
  ~xiiQtPropertyGridWidget();

  void SetDocument(xiiDocument* pDocument, bool bBindToSelectionManager = true);

  void                            ClearSelection();
  void                            SetSelectionIncludeExcludeProperties(const char* szIncludeProperties = nullptr, const char* szExcludeProperties = nullptr);
  void                            SetSelection(const xiiDeque<const xiiDocumentObject*>& selection);
  const xiiDocument*              GetDocument() const;
  const xiiDocumentObjectManager* GetObjectManager() const;
  xiiCommandHistory*              GetCommandHistory() const;
  xiiObjectAccessorBase*          GetObjectAccessor() const;

  static xiiRttiMappedObjectFactory<xiiQtPropertyWidget>& GetFactory();
  static xiiQtPropertyWidget*                             CreateMemberPropertyWidget(const xiiAbstractProperty* pProp);
  static xiiQtPropertyWidget*                             CreatePropertyWidget(const xiiAbstractProperty* pProp);

  void SetCollapseState(xiiQtGroupBoxBase* pBox);

Q_SIGNALS:
  void ExtendContextMenu(QMenu& menu, const xiiHybridArray<xiiPropertySelection, 8>& items, const xiiAbstractProperty* pProp);

public Q_SLOTS:
  void OnCollapseStateChanged(bool bCollapsed);

private:
  static xiiRttiMappedObjectFactory<xiiQtPropertyWidget> s_Factory;
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, PropertyGrid);

private:
  void      ObjectAccessorChangeEventHandler(const xiiObjectAccessorChangeEvent& e);
  void      SelectionEventHandler(const xiiSelectionManagerEvent& e);
  void      FactoryEventHandler(const xiiRttiMappedObjectFactory<xiiQtPropertyWidget>::Event& e);
  void      TypeEventHandler(const xiiPhantomRttiManagerEvent& e);
  xiiUInt32 GetGroupBoxHash(xiiQtGroupBoxBase* pBox) const;

private:
  xiiDocument*                       m_pDocument;
  bool                               m_bBindToSelectionManager = false;
  xiiDeque<const xiiDocumentObject*> m_Selection;
  xiiMap<xiiUInt32, bool>            m_CollapseState;
  xiiString                          m_sSelectionIncludeProperties;
  xiiString                          m_sSelectionExcludeProperties;

  QVBoxLayout* m_pLayout;
  QScrollArea* m_pScroll;
  QWidget*     m_pContent;
  QVBoxLayout* m_pContentLayout;

  xiiQtTypeWidget* m_pTypeWidget;
  QSpacerItem*     m_pSpacer;
};
