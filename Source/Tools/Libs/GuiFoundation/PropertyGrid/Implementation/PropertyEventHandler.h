#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

class xiiQtPropertyGridWidget;
class xiiAbstractProperty;

struct xiiPropertyEvent
{
  enum class Type
  {
    SingleValueChanged,
    BeginTemporary,
    EndTemporary,
    CancelTemporary,
  };

  Type                                           m_Type;
  const xiiAbstractProperty*                     m_pProperty;
  const xiiHybridArray<xiiPropertySelection, 8>* m_pItems;
  xiiVariant                                     m_Value;
};
