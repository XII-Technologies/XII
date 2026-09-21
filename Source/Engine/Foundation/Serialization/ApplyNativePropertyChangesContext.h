/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Serialization/RttiConverter.h>

/// The xiiApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those of the xiiAbstractObjectGraph that was passed in. This allows native changes to be tracked and applied to the object graph at a later point.
/// \sa xiiAbstractObjectGraph::ModifyNodeViaNativeCounterpart
class XII_FOUNDATION_DLL xiiApplyNativePropertyChangesContext : public xiiRttiConverterContext
{
public:
  xiiApplyNativePropertyChangesContext(xiiRttiConverterContext& ref_source, const xiiAbstractObjectGraph& originalGraph);

  virtual xiiUuid GenerateObjectGuid(const xiiUuid& parentGuid, const xiiAbstractProperty* pProp, xiiVariant index, void* pObject) const override;

private:
  xiiRttiConverterContext&      m_NativeContext;
  const xiiAbstractObjectGraph& m_OriginalGraph;
};
