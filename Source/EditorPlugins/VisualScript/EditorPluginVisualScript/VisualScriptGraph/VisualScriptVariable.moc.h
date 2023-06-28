#pragma once

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class xiiVisualScriptVariableAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptVariableAttribute, xiiTypeWidgetAttribute);
};

//////////////////////////////////////////////////////////////////////////

class xiiQtVisualScriptVariableWidget : public xiiQtVariantPropertyWidget
{
  Q_OBJECT;

public:
  xiiQtVisualScriptVariableWidget();
  virtual ~xiiQtVisualScriptVariableWidget();

protected:
  virtual xiiResult GetVariantTypeDisplayName(xiiVariantType::Enum type, xiiStringBuilder& out_sName) const override;
};
