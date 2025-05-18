#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

xiiPropertyAnimObjectAccessor::xiiPropertyAnimObjectAccessor(xiiPropertyAnimAssetDocument* pDoc, xiiCommandHistory* pHistory) :
  xiiObjectCommandAccessor(pHistory), m_pDocument(pDoc), m_pObjectManager(static_cast<xiiPropertyAnimObjectManager*>(pDoc->GetObjectManager()))
{
  m_pObjAccessor = XII_DEFAULT_NEW(xiiObjectCommandAccessor, pHistory);
}

xiiStatus xiiPropertyAnimObjectAccessor::GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index /*= xiiVariant()*/)
{
  return xiiObjectCommandAccessor::GetValue(pObject, pProp, out_value, index);
}

xiiStatus xiiPropertyAnimObjectAccessor::SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index)
{
  if (IsTemporary(pObject))
  {
    xiiVariant oldValue;
    XII_VERIFY(m_pObjAccessor->GetValue(pObject, pProp, oldValue, index).Succeeded(), "Property does not exist, can't animate");

    xiiVariantType::Enum type = pProp->GetSpecificType()->GetVariantType();
    if (type >= xiiVariantType::Bool && type <= xiiVariantType::Double)
    {
      return SetCurveCp(pObject, pProp, index, xiiPropertyAnimTarget::Number, oldValue.ConvertTo<double>(), newValue.ConvertTo<double>());
    }
    else if (type >= xiiVariantType::Vector2 && type <= xiiVariantType::Vector4U)
    {
      const xiiUInt32 uiComponents = xiiReflectionUtils::GetComponentCount(type);
      for (xiiUInt32 c = 0; c < uiComponents; c++)
      {
        const double fOldValue = xiiReflectionUtils::GetComponent(oldValue, c);
        const double fValue    = xiiReflectionUtils::GetComponent(newValue, c);

        if (xiiMath::IsEqual(fOldValue, fValue, xiiMath::SmallEpsilon<double>()))
          continue;

        XII_SUCCEED_OR_RETURN(
          SetCurveCp(pObject, pProp, index, static_cast<xiiPropertyAnimTarget::Enum>((int)xiiPropertyAnimTarget::VectorX + c), fOldValue, fValue));
      }

      return xiiStatus(XII_SUCCESS);
    }
    else if (type == xiiVariantType::Color)
    {
      auto            oldColor = oldValue.Get<xiiColor>();
      xiiColorGammaUB oldColorGamma;
      xiiUInt8        oldAlpha;
      float           oldIntensity;
      SeparateColor(oldColor, oldColorGamma, oldAlpha, oldIntensity);
      auto            newColor = newValue.Get<xiiColor>();
      xiiColorGammaUB newColorGamma;
      xiiUInt8        newAlpha;
      float           newIntensity;
      SeparateColor(newColor, newColorGamma, newAlpha, newIntensity);

      xiiStatus res(XII_SUCCESS);
      if (oldColorGamma != newColorGamma)
      {
        res = SetColorCurveCp(pObject, pProp, index, oldColorGamma, newColorGamma);
      }
      if (oldAlpha != newAlpha && res.Succeeded())
      {
        res = SetAlphaCurveCp(pObject, pProp, index, oldAlpha, newAlpha);
      }
      if (!xiiMath::IsEqual(oldIntensity, newIntensity, xiiMath::SmallEpsilon<float>()) && res.Succeeded())
      {
        res = SetIntensityCurveCp(pObject, pProp, index, oldIntensity, newIntensity);
      }
      return res;
    }
    else if (type == xiiVariantType::ColorGamma)
    {
      auto     oldColorGamma = oldValue.Get<xiiColorGammaUB>();
      xiiUInt8 oldAlpha      = oldColorGamma.a;
      oldColorGamma.a        = 255;

      auto     newColorGamma = newValue.Get<xiiColorGammaUB>();
      xiiUInt8 newAlpha      = newColorGamma.a;
      newColorGamma.a        = 255;

      xiiStatus res(XII_SUCCESS);
      if (oldColorGamma != newColorGamma)
      {
        res = SetColorCurveCp(pObject, pProp, index, oldColorGamma, newColorGamma);
      }
      if (oldAlpha != newAlpha && res.Succeeded())
      {
        res = SetAlphaCurveCp(pObject, pProp, index, oldAlpha, newAlpha);
      }
      return res;
    }
    else if (type == xiiVariantType::Quaternion)
    {

      const xiiQuat qOldRot = oldValue.Get<xiiQuat>();
      const xiiQuat qNewRot = newValue.Get<xiiQuat>();

      xiiAngle oldEuler[3];
      qOldRot.GetAsEulerAngles(oldEuler[0], oldEuler[1], oldEuler[2]);
      xiiAngle newEuler[3];
      qNewRot.GetAsEulerAngles(newEuler[0], newEuler[1], newEuler[2]);

      for (xiiUInt32 c = 0; c < 3; c++)
      {
        XII_SUCCEED_OR_RETURN(
          m_pDocument->CanAnimate(pObject, pProp, index, static_cast<xiiPropertyAnimTarget::Enum>((int)xiiPropertyAnimTarget::RotationX + c)));
        float       oldValue = oldEuler[c].GetDegree();
        xiiUuid     track    = FindOrAddTrack(pObject, pProp, index, static_cast<xiiPropertyAnimTarget::Enum>((int)xiiPropertyAnimTarget::RotationX + c),
                                              [this, oldValue](const xiiUuid& trackGuid) {
                                         // add a control point at the start of the curve with the original value
                                         m_pDocument->InsertCurveCpAt(trackGuid, 0, oldValue);
                                       });
        const auto* pTrack   = m_pDocument->GetTrack(track);
        oldEuler[c]          = xiiAngle::MakeFromDegree(pTrack->m_FloatCurve.Evaluate(m_pDocument->GetScrubberPosition()));
      }

      for (xiiUInt32 c = 0; c < 3; c++)
      {
        // We assume the change is less than 180 degrees from the old value
        float fDiff   = (newEuler[c] - oldEuler[c]).GetDegree();
        float iRounds = xiiMath::RoundToMultiple(fDiff, 360.0f);
        fDiff -= iRounds;
        newEuler[c] = oldEuler[c] + xiiAngle::MakeFromDegree(fDiff);
        if (oldEuler[c].IsEqualSimple(newEuler[c], xiiAngle::MakeFromDegree(0.01f)))
          continue;

        XII_SUCCEED_OR_RETURN(SetCurveCp(pObject, pProp, index, static_cast<xiiPropertyAnimTarget::Enum>((int)xiiPropertyAnimTarget::RotationX + c),
                                         oldEuler[c].GetDegree(), newEuler[c].GetDegree()));
      }

      return xiiStatus(XII_SUCCESS);
    }

    return xiiStatus(xiiFmt("The property '{0}' cannot be animated.", pProp->GetPropertyName()));
  }
  else
  {
    return xiiObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
  }
}

xiiStatus xiiPropertyAnimObjectAccessor::InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return xiiStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return xiiObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
}

xiiStatus xiiPropertyAnimObjectAccessor::RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index /*= xiiVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return xiiStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return xiiObjectCommandAccessor::RemoveValue(pObject, pProp, index);
  }
}

xiiStatus xiiPropertyAnimObjectAccessor::MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  if (IsTemporary(pObject))
  {
    return xiiStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return xiiObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex);
  }
}

xiiStatus xiiPropertyAnimObjectAccessor::AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid)
{
  if (IsTemporary(pParent, pParentProp))
  {
    return xiiStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return xiiObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
  }
}

xiiStatus xiiPropertyAnimObjectAccessor::RemoveObject(const xiiDocumentObject* pObject)
{
  if (IsTemporary(pObject))
  {
    return xiiStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return xiiObjectCommandAccessor::RemoveObject(pObject);
  }
}

xiiStatus xiiPropertyAnimObjectAccessor::MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index)
{
  if (IsTemporary(pObject))
  {
    return xiiStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return xiiObjectCommandAccessor::MoveObject(pObject, pNewParent, pParentProp, index);
  }
}

bool xiiPropertyAnimObjectAccessor::IsTemporary(const xiiDocumentObject* pObject) const
{
  return m_pObjectManager->IsTemporary(pObject);
}

bool xiiPropertyAnimObjectAccessor::IsTemporary(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp) const
{
  return m_pObjectManager->IsTemporary(pParent, pParentProp->GetPropertyName());
}

xiiStatus xiiPropertyAnimObjectAccessor::SetCurveCp(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiPropertyAnimTarget::Enum target, double fOldValue, double fNewValue)
{
  XII_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, target));
  xiiUuid track = FindOrAddTrack(pObject, pProp, index, target, [this, fOldValue](const xiiUuid& trackGuid) {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertCurveCpAt(trackGuid, 0, fOldValue); });
  return SetOrInsertCurveCp(track, fNewValue);
}

xiiUuid xiiPropertyAnimObjectAccessor::FindOrAddTrack(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiPropertyAnimTarget::Enum target, OnAddTrack onAddTrack)
{
  xiiUuid track = m_pDocument->FindTrack(pObject, pProp, index, target);
  if (!track.IsValid())
  {
    auto pHistory                 = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    track = m_pDocument->CreateTrack(pObject, pProp, index, target);
    onAddTrack(track);

    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  XII_ASSERT_DEBUG(track.IsValid(), "Creating track failed.");
  return track;
}

xiiStatus xiiPropertyAnimObjectAccessor::SetOrInsertCurveCp(const xiiUuid& track, double fValue)
{
  const xiiInt64 iScrubberPos = (xiiInt64)m_pDocument->GetScrubberPosition();
  xiiUuid        cpGuid       = m_pDocument->FindCurveCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    XII_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Value", fValue).Succeeded(), "");
  }
  else
  {
    auto pHistory                 = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertCurveCpAt(track, iScrubberPos, fValue);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiPropertyAnimObjectAccessor::SetColorCurveCp(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, const xiiColorGammaUB& oldValue, const xiiColorGammaUB& newValue)
{
  XII_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, xiiPropertyAnimTarget::Color));
  xiiUuid track = FindOrAddTrack(pObject, pProp, index, xiiPropertyAnimTarget::Color, [this, &oldValue](const xiiUuid& trackGuid) {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertGradientColorCpAt(trackGuid, 0, oldValue);
    //
  });

  XII_SUCCEED_OR_RETURN(SetOrInsertColorCurveCp(track, newValue));

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiPropertyAnimObjectAccessor::SetOrInsertColorCurveCp(const xiiUuid& track, const xiiColorGammaUB& value)
{
  const xiiInt64 iScrubberPos = (xiiInt64)m_pDocument->GetScrubberPosition();
  xiiUuid        cpGuid       = m_pDocument->FindGradientColorCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    XII_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Red", value.r).Succeeded(), "");
    XII_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Green", value.g).Succeeded(), "");
    XII_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Blue", value.b).Succeeded(), "");
  }
  else
  {
    auto pHistory                 = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientColorCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiPropertyAnimObjectAccessor::SetAlphaCurveCp(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiUInt8 oldValue, xiiUInt8 newValue)
{
  XII_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, xiiPropertyAnimTarget::Color));
  xiiUuid track = FindOrAddTrack(pObject, pProp, index, xiiPropertyAnimTarget::Color, [this, &oldValue](const xiiUuid& trackGuid) {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertGradientAlphaCpAt(trackGuid, 0, oldValue);
    //
  });

  XII_SUCCEED_OR_RETURN(SetOrInsertAlphaCurveCp(track, newValue));
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiPropertyAnimObjectAccessor::SetOrInsertAlphaCurveCp(const xiiUuid& track, xiiUInt8 value)
{
  const xiiInt64 iScrubberPos = (xiiInt64)m_pDocument->GetScrubberPosition();
  xiiUuid        cpGuid       = m_pDocument->FindGradientAlphaCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    XII_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Alpha", value).Succeeded(), "");
  }
  else
  {
    auto pHistory                 = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientAlphaCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiPropertyAnimObjectAccessor::SetIntensityCurveCp(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, float oldValue, float newValue)
{
  xiiUuid track = FindOrAddTrack(pObject, pProp, index, xiiPropertyAnimTarget::Color, [this, &oldValue](const xiiUuid& trackGuid) {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertGradientIntensityCpAt(trackGuid, 0, oldValue);
    //
  });

  XII_SUCCEED_OR_RETURN(SetOrInsertIntensityCurveCp(track, newValue));
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiPropertyAnimObjectAccessor::SetOrInsertIntensityCurveCp(const xiiUuid& track, float value)
{
  const xiiInt64 iScrubberPos = (xiiInt64)m_pDocument->GetScrubberPosition();
  xiiUuid        cpGuid       = m_pDocument->FindGradientIntensityCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    XII_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Intensity", value).Succeeded(), "");
  }
  else
  {
    auto pHistory                 = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientIntensityCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return xiiStatus(XII_SUCCESS);
}

void xiiPropertyAnimObjectAccessor::SeparateColor(const xiiColor& color, xiiColorGammaUB& gamma, xiiUInt8& alpha, float& intensity)
{
  alpha     = static_cast<xiiColorGammaUB>(color).a;
  intensity = xiiMath::Max(color.r, color.g, color.b);
  if (intensity > 1.0f)
  {
    gamma   = (color / intensity);
    gamma.a = 255;
  }
  else
  {
    intensity = 1.0f;
    gamma     = color;
    gamma.a   = 255;
  }
}
