/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

#include <QEvent>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QWidget>
#include <QWidgetAction>

class QAction;
class QMenu;
class QLabel;
class QSlider;
class xiiAction;

/// Glue class that maps xiiActions to QActions. QActions are only created if the xiiAction is actually mapped somewhere. Document and Global actions are manually executed and don't solely rely on Qt's ShortcutContext setting to prevent ambiguous action shortcuts.
class XII_GUIFOUNDATION_DLL xiiQtProxy : public QObject
{
  Q_OBJECT

public:
  xiiQtProxy();
  virtual ~xiiQtProxy();

  virtual void Update() = 0;

  virtual void SetAction(xiiAction* pAction);
  xiiAction*   GetAction() { return m_pAction; }

  /// Converts the QKeyEvent into a shortcut and tries to find a matching action in the document and global action list.
  ///
  /// Document actions are not mapped as ShortcutContext::WindowShortcut because docking allows for multiple documents to be mapped into the same window. Instead, ShortcutContext::WidgetWithChildrenShortcut is used to prevent ambiguous action shortcuts and the actions are executed manually via filtering QEvent::ShortcutOverride at the dock widget level.
  /// The function always has to be called two times:
  /// A: QEvent::ShortcutOverride: Only check with bTestOnly = true that we want to override the shortcut. This will instruct Qt to send the event as a regular key press event to the widget that accepted the override.
  /// B: QEvent::keyPressEvent: Execute the actual action with bTestOnly = false;
  ///
  /// \param pDocument The document for which matching actions should be searched for. If null, only global actions are searched.
  /// \param pEvent The key event that should be converted into a shortcut.
  /// \param bTestOnly Accept the event and return true but don't execute the action. Use this inside QEvent::ShortcutOverride.
  /// \return Whether the key event was consumed and an action executed.
  static bool TriggerDocumentAction(xiiDocument* pDocument, QKeyEvent* pEvent, bool bTestOnly);

  static xiiRttiMappedObjectFactory<xiiQtProxy>& GetFactory();
  static QSharedPointer<xiiQtProxy>              GetProxy(xiiActionContext& ref_context, xiiActionDescriptorHandle hAction);

protected:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, QtProxies);
  static xiiRttiMappedObjectFactory<xiiQtProxy>                                                  s_Factory;
  static xiiMap<xiiActionDescriptorHandle, QWeakPointer<xiiQtProxy>>                             s_GlobalActions;
  static xiiMap<const xiiDocument*, xiiMap<xiiActionDescriptorHandle, QWeakPointer<xiiQtProxy>>> s_DocumentActions;
  static xiiMap<QWidget*, xiiMap<xiiActionDescriptorHandle, QWeakPointer<xiiQtProxy>>>           s_WindowActions;
  static QObject*                                                                                s_pSignalProxy;

protected:
  xiiAction* m_pAction;
};

class XII_GUIFOUNDATION_DLL xiiQtActionProxy : public xiiQtProxy
{
  Q_OBJECT

public:
  virtual QAction* GetQAction() = 0;
};

class XII_GUIFOUNDATION_DLL xiiQtCategoryProxy : public xiiQtProxy
{
  Q_OBJECT
public:
  virtual void Update() override {}
};

class XII_GUIFOUNDATION_DLL xiiQtMenuProxy : public xiiQtProxy
{
  Q_OBJECT

public:
  xiiQtMenuProxy();
  ~xiiQtMenuProxy();

  virtual void Update() override;
  virtual void SetAction(xiiAction* pAction) override;

  virtual QMenu* GetQMenu();

private:
  void StatusUpdateEventHandler(xiiAction* pAction);

protected:
  QMenu* m_pMenu;
};

class XII_GUIFOUNDATION_DLL xiiQtButtonProxy : public xiiQtActionProxy
{
  Q_OBJECT

public:
  xiiQtButtonProxy();
  ~xiiQtButtonProxy();

  virtual void Update() override;
  virtual void SetAction(xiiAction* pAction) override;

  virtual QAction* GetQAction() override;

private Q_SLOTS:
  void OnTriggered();

private:
  void StatusUpdateEventHandler(xiiAction* pAction);

private:
  QPointer<QAction> m_pQtAction;
};


class XII_GUIFOUNDATION_DLL xiiQtDynamicMenuProxy : public xiiQtMenuProxy
{
  Q_OBJECT

public:
  virtual void SetAction(xiiAction* pAction) override;

private Q_SLOTS:
  void SlotMenuAboutToShow();
  void SlotMenuEntryTriggered();

private:
  xiiHybridArray<xiiDynamicMenuAction::Item, 16> m_Entries;
};

class XII_GUIFOUNDATION_DLL xiiQtDynamicActionAndMenuProxy : public xiiQtDynamicMenuProxy
{
  Q_OBJECT

public:
  xiiQtDynamicActionAndMenuProxy();
  ~xiiQtDynamicActionAndMenuProxy();

  virtual void     Update() override;
  virtual void     SetAction(xiiAction* pAction) override;
  virtual QAction* GetQAction();

private Q_SLOTS:
  void OnTriggered();

private:
  QPointer<QAction> m_pQtAction;
};


class XII_GUIFOUNDATION_DLL xiiQtLabeledSlider : public QWidget
{
  Q_OBJECT

public:
  xiiQtLabeledSlider(QWidget* pParent);

  QLabel*  m_pLabel;
  QSlider* m_pSlider;
};


class XII_GUIFOUNDATION_DLL xiiQtSliderWidgetAction : public QWidgetAction
{
  Q_OBJECT

public:
  xiiQtSliderWidgetAction(QWidget* pParent);
  void setMinimum(int value);
  void setMaximum(int value);
  void setValue(int value);

Q_SIGNALS:
  void valueChanged(int value);

private Q_SLOTS:
  void OnValueChanged(int value);

protected:
  virtual QWidget* createWidget(QWidget* parent) override;
  virtual bool     eventFilter(QObject* obj, QEvent* e) override;

  xiiInt32 m_iMinimum;
  xiiInt32 m_iMaximum;
  xiiInt32 m_iValue;
};

class XII_GUIFOUNDATION_DLL xiiQtSliderProxy : public xiiQtActionProxy
{
  Q_OBJECT

public:
  xiiQtSliderProxy();
  ~xiiQtSliderProxy();

  virtual void Update() override;
  virtual void SetAction(xiiAction* pAction) override;

  virtual QAction* GetQAction() override;

private Q_SLOTS:
  void OnValueChanged(int value);

private:
  void StatusUpdateEventHandler(xiiAction* pAction);

private:
  QPointer<xiiQtSliderWidgetAction> m_pQtAction;
};
