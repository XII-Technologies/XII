#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QComboBox;
class xiiDynamicEnum;

/// *** Asset Browser ***

class XII_EDITORFRAMEWORK_DLL xiiQtDynamicEnumPropertyWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtDynamicEnumPropertyWidget();

protected slots:
  void on_CurrentEnum_changed(int iEnum);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  QComboBox*      m_pWidget;
  QHBoxLayout*    m_pLayout;
  xiiDynamicEnum* m_pDynamicEnum = nullptr;
  xiiInt32        m_iLastIndex   = -1;
};
