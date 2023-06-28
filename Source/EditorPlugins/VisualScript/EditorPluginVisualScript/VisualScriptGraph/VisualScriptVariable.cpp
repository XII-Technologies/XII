#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptVariableAttribute, 1, xiiRTTIDefaultAllocator<xiiVisualScriptVariableAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static xiiQtPropertyWidget* VisualScriptVariableTypeCreator(const xiiRTTI* pRtti)
{
  return new xiiQtVisualScriptVariableWidget();
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, VisualScriptVariable)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiVisualScriptVariableAttribute>(), VisualScriptVariableTypeCreator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVisualScriptVariableAttribute>());
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiQtVisualScriptVariableWidget::xiiQtVisualScriptVariableWidget() = default;
xiiQtVisualScriptVariableWidget::~xiiQtVisualScriptVariableWidget() = default;

xiiResult xiiQtVisualScriptVariableWidget::GetVariantTypeDisplayName(xiiVariantType::Enum type, xiiStringBuilder& out_sName) const
{
  if (type == xiiVariantType::Int8 ||
      type == xiiVariantType::Int16 ||
      type == xiiVariantType::UInt16 ||
      type == xiiVariantType::UInt32 ||
      type == xiiVariantType::UInt64 ||
      type == xiiVariantType::StringView)
    return XII_FAILURE;

  xiiVisualScriptDataType::Enum dataType = xiiVisualScriptDataType::FromVariantType(type);
  if (type != xiiVariantType::Invalid && dataType == xiiVisualScriptDataType::Invalid)
    return XII_FAILURE;

  const xiiRTTI* pVisualScriptDataType = xiiGetStaticRTTI<xiiVisualScriptDataType>();
  if (xiiReflectionUtils::EnumerationToString(pVisualScriptDataType, dataType, out_sName) == false)
    return XII_FAILURE;

  return XII_SUCCESS;
}
