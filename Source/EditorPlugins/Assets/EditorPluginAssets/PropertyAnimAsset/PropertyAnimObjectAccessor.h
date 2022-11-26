#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class xiiPropertyAnimAssetDocument;
class xiiPropertyAnimObjectManager;

class xiiPropertyAnimObjectAccessor : public xiiObjectCommandAccessor
{
public:
  xiiPropertyAnimObjectAccessor(xiiPropertyAnimAssetDocument* pDoc, xiiCommandHistory* pHistory);

  virtual xiiStatus GetValue(
    const xiiDocumentObject*   pObject,
    const xiiAbstractProperty* pProp,
    xiiVariant&                out_value,
    xiiVariant                 index = xiiVariant()) override;
  virtual xiiStatus SetValue(
    const xiiDocumentObject*   pObject,
    const xiiAbstractProperty* pProp,
    const xiiVariant&          newValue,
    xiiVariant                 index = xiiVariant()) override;

  virtual xiiStatus InsertValue(
    const xiiDocumentObject*   pObject,
    const xiiAbstractProperty* pProp,
    const xiiVariant&          newValue,
    xiiVariant                 index = xiiVariant()) override;
  virtual xiiStatus RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus MoveValue(
    const xiiDocumentObject*   pObject,
    const xiiAbstractProperty* pProp,
    const xiiVariant&          oldIndex,
    const xiiVariant&          newIndex) override;

  virtual xiiStatus AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid) override;
  virtual xiiStatus RemoveObject(const xiiDocumentObject* pObject) override;
  virtual xiiStatus MoveObject(
    const xiiDocumentObject*   pObject,
    const xiiDocumentObject*   pNewParent,
    const xiiAbstractProperty* pParentProp,
    const xiiVariant&          index) override;

private:
  bool                                      IsTemporary(const xiiDocumentObject* pObject) const;
  bool                                      IsTemporary(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp) const;
  typedef xiiDelegate<void(const xiiUuid&)> OnAddTrack;
  xiiUuid                                   FindOrAddTrack(
                                      const xiiDocumentObject*    pObject,
                                      const xiiAbstractProperty*  pProp,
                                      xiiVariant                  index,
                                      xiiPropertyAnimTarget::Enum target,
                                      OnAddTrack                  onAddTrack);

  xiiStatus SetCurveCp(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiPropertyAnimTarget::Enum target, double fOldValue, double fNewValue);
  xiiStatus SetOrInsertCurveCp(const xiiUuid& track, double fValue);

  xiiStatus SetColorCurveCp(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, const xiiColorGammaUB& oldValue, const xiiColorGammaUB& newValue);
  xiiStatus SetOrInsertColorCurveCp(const xiiUuid& track, const xiiColorGammaUB& value);

  xiiStatus SetAlphaCurveCp(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiUInt8 oldValue, xiiUInt8 newValue);
  xiiStatus SetOrInsertAlphaCurveCp(const xiiUuid& track, xiiUInt8 value);

  xiiStatus SetIntensityCurveCp(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, float oldValue, float newValue);
  xiiStatus SetOrInsertIntensityCurveCp(const xiiUuid& track, float value);

  void SeparateColor(const xiiColor& color, xiiColorGammaUB& gamma, xiiUInt8& alpha, float& intensity);

  xiiUniquePtr<xiiObjectAccessorBase> m_pObjAccessor;
  xiiPropertyAnimAssetDocument*       m_pDocument      = nullptr;
  xiiPropertyAnimObjectManager*       m_pObjectManager = nullptr;
};
