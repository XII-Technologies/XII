#pragma once

#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiRandomGauss;

class xiiDuplicateObjectsCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDuplicateObjectsCommand, xiiCommand);

public:
  xiiDuplicateObjectsCommand();

public: // Properties
  xiiString m_sGraphTextFormat;
  xiiString m_sParentNodes; /// A stringyfied map in format "uuidObj1=uuidParent1;..." that defines the previous parents of all top level objects

  xiiUInt32 m_uiNumberOfCopies; /// if set to 0 (the default), all the 'advanced' duplication code is skipped and only a single straight copy is made

  xiiVec3 m_vAccumulativeTranslation;
  xiiVec3 m_vAccumulativeRotation;
  xiiVec3 m_vRandomRotation;
  xiiVec3 m_vRandomTranslation;
  bool    m_bGroupDuplicates;

  xiiInt8  m_iRevolveAxis; ///< 0 = disabled, 1 = x, 2 = y, 3 = z
  float    m_fRevolveRadius;
  xiiAngle m_RevolveStartAngle;
  xiiAngle m_RevolveAngleStep;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;

  void SetAsSelection();

  void DeserializeGraph(xiiAbstractObjectGraph& graph);

  void CreateOneDuplicate(xiiAbstractObjectGraph& graph, xiiHybridArray<xiiDocument::PasteInfo, 16>& ToBePasted);
  void AdjustObjectPositions(xiiHybridArray<xiiDocument::PasteInfo, 16>& Duplicates, xiiUInt32 uiNumDuplicate, xiiRandomGauss& rngRotX, xiiRandomGauss& rngRotY, xiiRandomGauss& rngRotZ, xiiRandomGauss& rngTransX, xiiRandomGauss& rngTransY, xiiRandomGauss& rngTransZ);

  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override;

private:
  struct DuplicatedObject
  {
    xiiDocumentObject* m_pObject;
    xiiDocumentObject* m_pParent;
    xiiString          m_sParentProperty;
    xiiVariant         m_Index;
    xiiUInt32          m_uiSelectionOrder = 0;
  };

  xiiDeque<const xiiDocumentObject*>  m_OriginalSelection;
  xiiHybridArray<DuplicatedObject, 4> m_DuplicatedObjects;
};
