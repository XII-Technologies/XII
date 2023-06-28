#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiPropertyAnimObjectManager : public xiiDocumentObjectManager
{
public:
  xiiPropertyAnimObjectManager();
  ~xiiPropertyAnimObjectManager();

  bool GetAllowStructureChangeOnTemporaries() const { return m_bAllowStructureChangeOnTemporaries; }
  void SetAllowStructureChangeOnTemporaries(bool val) { m_bAllowStructureChangeOnTemporaries = val; }

private:
  virtual xiiStatus InternalCanAdd(const xiiRTTI* pRtti, const xiiDocumentObject* pParent, const char* szParentProperty, const xiiVariant& index) const override;
  virtual xiiStatus InternalCanRemove(const xiiDocumentObject* pObject) const override;
  virtual xiiStatus InternalCanMove(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const char* szParentProperty, const xiiVariant& index) const override;

private:
  bool m_bAllowStructureChangeOnTemporaries = false;
};
