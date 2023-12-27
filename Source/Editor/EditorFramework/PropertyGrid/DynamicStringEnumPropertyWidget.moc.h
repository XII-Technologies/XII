#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QComboBox;
class xiiDynamicStringEnum;

class XII_EDITORFRAMEWORK_DLL xiiQtDynamicStringEnumPropertyWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtDynamicStringEnumPropertyWidget();

protected slots:
  void on_CurrentEnum_changed(int iEnum);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  QComboBox*            m_pWidget    = nullptr;
  QHBoxLayout*          m_pLayout    = nullptr;
  xiiDynamicStringEnum* m_pEnum      = nullptr;
  xiiInt32              m_iLastIndex = -1;
};
