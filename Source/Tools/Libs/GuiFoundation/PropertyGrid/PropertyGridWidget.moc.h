/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <Foundation/Containers/HybridArray.h>
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
class QLabel;
class QToolButton;
class xiiQtSearchWidget;

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
  void                            SetSelectionIncludeExcludeProperties(xiiStringView sIncludeProperties = {}, xiiStringView sExcludeProperties = {});
  void                            SetSelection(const xiiDeque<const xiiDocumentObject*>& selection);
  bool                            IsSelectionPinned() const;
  const xiiDocument*              GetDocument() const;
  const xiiDocumentObjectManager* GetObjectManager() const;
  xiiCommandHistory*              GetCommandHistory() const;
  xiiObjectAccessorBase*          GetObjectAccessor() const;

  static xiiRttiMappedObjectFactory<xiiQtPropertyWidget>& GetFactory();
  static xiiQtPropertyWidget*                             CreateMemberPropertyWidget(const xiiAbstractProperty* pProp);
  static xiiQtPropertyWidget*                             CreatePropertyWidget(const xiiAbstractProperty* pProp);

  void SetCollapseState(xiiQtGroupBoxBase* pBox);

Q_SIGNALS:
  void ExtendContextMenu(QMenu& ref_menu, const xiiHybridArray<xiiPropertySelection, 8>& items, const xiiAbstractProperty* pProp);

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
  void      NavigateProperty(int iDirection);
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

  xiiQtTypeWidget*                                                              m_pTypeWidget;
  QSpacerItem*                                                                  m_pSpacer;
  xiiQtSearchWidget*                                                            m_pSearch         = nullptr;
  QLabel*                                                                       m_pSelectionLabel = nullptr;
  QLabel*                                                                       m_pSearchResult   = nullptr;
  xiiInt32                                                                      m_iSearchIndex    = -1;
  QToolButton*                                                                  m_pPinSelection   = nullptr;
  xiiCopyOnBroadcastEvent<const xiiDocumentObjectStructureEvent&>::Unsubscriber m_PinSubscription;
};
