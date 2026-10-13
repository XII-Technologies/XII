/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Math/Random.h>

void xiiDuplicateObjectsCommand::AdjustObjectPositions(xiiHybridArray<xiiDocument::PasteInfo, 16>& Duplicates, xiiUInt32 uiNumDuplicate, xiiRandomGauss& rngRotX, xiiRandomGauss& rngRotY, xiiRandomGauss& rngRotZ, xiiRandomGauss& rngTransX, xiiRandomGauss& rngTransY, xiiRandomGauss& rngTransZ)
{
  xiiSceneDocument* pScene = static_cast<xiiSceneDocument*>(m_pDocument);

  const float fStep = uiNumDuplicate;

  xiiVec3 vRandT(0.0f);
  xiiVec3 vRandR(0.0f);

  if (m_vRandomRotation.x != 0)
    vRandR.x = rngRotX.SignedValue() * (m_vRandomRotation.x / 1023.0f);
  if (m_vRandomRotation.y != 0)
    vRandR.y = rngRotY.SignedValue() * (m_vRandomRotation.y / 1023.0f);
  if (m_vRandomRotation.z != 0)
    vRandR.z = rngRotZ.SignedValue() * (m_vRandomRotation.z / 1023.0f);

  if (m_vRandomTranslation.x != 0)
    vRandT.x = rngTransX.SignedValue() * (m_vRandomTranslation.x / 1023.0f);
  if (m_vRandomTranslation.y != 0)
    vRandT.y = rngTransY.SignedValue() * (m_vRandomTranslation.y / 1023.0f);
  if (m_vRandomTranslation.z != 0)
    vRandT.z = rngTransZ.SignedValue() * (m_vRandomTranslation.z / 1023.0f);

  xiiVec3 vPosOffset(0.0f);

  if (m_iRevolveAxis > 0 && m_fRevolveRadius != 0.0f)
  {
    xiiVec3  vRevolveAxis(0.0f);
    xiiAngle revolve = m_RevolveStartAngle;

    switch (m_iRevolveAxis)
    {
      case 1:
        vRevolveAxis.Set(1, 0, 0);
        vPosOffset.Set(0, 0, m_fRevolveRadius);
        break;
      case 2:
        vRevolveAxis.Set(0, 1, 0);
        vPosOffset.Set(m_fRevolveRadius, 0, 0);
        break;
      case 3:
        vRevolveAxis.Set(0, 0, 1);
        vPosOffset.Set(0, m_fRevolveRadius, 0);
        break;
    }

    revolve += fStep * m_RevolveAngleStep;

    xiiMat3 mRevolve = xiiMat3::MakeAxisRotation(vRevolveAxis, revolve);

    vPosOffset = mRevolve * vPosOffset;
  }

  xiiQuat qRot = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeFromDegree(fStep * m_vAccumulativeRotation.x + vRandR.x), xiiAngle::MakeFromDegree(fStep * m_vAccumulativeRotation.y + vRandR.y), xiiAngle::MakeFromDegree(fStep * m_vAccumulativeRotation.z + vRandR.z));

  for (const auto& pi : Duplicates)
  {
    xiiTransform tGlobal = pScene->GetGlobalTransform(pi.m_pObject);

    tGlobal.m_vScale.Set(1.0f);
    tGlobal.m_vPosition += vPosOffset + (1.0f + fStep) * m_vAccumulativeTranslation + vRandT;
    tGlobal.m_qRotation = qRot * tGlobal.m_qRotation;

    pScene->SetGlobalTransform(pi.m_pObject, tGlobal, TransformationChanges::Translation | TransformationChanges::Rotation);
  }
}
