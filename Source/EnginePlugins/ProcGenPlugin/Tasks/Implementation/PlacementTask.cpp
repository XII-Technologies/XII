#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace xiiProcGenInternal
{

  XII_CHECK_AT_COMPILETIME(sizeof(PlacementPoint) == 32);
  XII_CHECK_AT_COMPILETIME(sizeof(PlacementTransform) == 64);

  PlacementTask::PlacementTask(PlacementData* pData, const char* szName) :
    m_pData(pData)
  {
    ConfigureTask(szName, xiiTaskNesting::Maybe);

    m_VM.RegisterFunction(xiiProcGenExpressionFunctions::s_ApplyVolumesFunc);
    m_VM.RegisterFunction(xiiProcGenExpressionFunctions::s_GetInstanceSeedFunc);
  }

  PlacementTask::~PlacementTask() = default;

  void PlacementTask::Clear()
  {
    m_InputPoints.Clear();
    m_OutputTransforms.Clear();
    m_Density.Clear();
    m_ValidPoints.Clear();
  }

  void PlacementTask::Execute()
  {
    FindPlacementPoints();

    if (!m_InputPoints.IsEmpty())
    {
      ExecuteVM();
    }
  }

  void PlacementTask::FindPlacementPoints()
  {
    XII_PROFILE_SCOPE("FindPlacementPoints");

    auto pOutput = m_pData->m_pOutput;

    xiiSimdVec4u seed = xiiSimdVec4u(m_pData->m_uiTileSeed) + xiiSimdVec4u(0, 3, 7, 11);

    float        fZRange    = m_pData->m_TileBoundingBox.GetExtents().z;
    xiiSimdFloat fZStart    = m_pData->m_TileBoundingBox.m_vMax.z;
    xiiSimdVec4f vXY        = xiiSimdConversion::ToVec3(m_pData->m_TileBoundingBox.m_vMin);
    xiiSimdVec4f vMinOffset = xiiSimdConversion::ToVec3(pOutput->m_vMinOffset);
    xiiSimdVec4f vMaxOffset = xiiSimdConversion::ToVec3(pOutput->m_vMaxOffset);

    // use center for fixed plane placement
    vXY.SetZ(m_pData->m_TileBoundingBox.GetCenter().z);

    xiiVec3   rayDir           = xiiVec3(0, 0, -1);
    xiiUInt32 uiCollisionLayer = pOutput->m_uiCollisionLayer;

    auto& patternPoints = pOutput->m_pPattern->m_Points;

    for (xiiUInt32 i = 0; i < patternPoints.GetCount(); ++i)
    {
      auto&        patternPoint  = patternPoints[i];
      xiiSimdVec4f patternCoords = xiiSimdConversion::ToVec3(patternPoint.m_Coordinates.GetAsVec3(0.0f));

      xiiPhysicsCastResult hitResult;

      if (m_pData->m_pPhysicsModule != nullptr && m_pData->m_pOutput->m_Mode == xiiProcPlacementMode::Raycast)
      {
        xiiSimdVec4f rayStart = (vXY + patternCoords * pOutput->m_fFootprint);
        rayStart += xiiSimdRandom::FloatMinMax(xiiSimdVec4i(i), vMinOffset, vMaxOffset, seed);
        rayStart.SetZ(fZStart);

        if (!m_pData->m_pPhysicsModule->Raycast(hitResult, xiiSimdConversion::ToVec3(rayStart), rayDir, fZRange, xiiPhysicsQueryParameters(uiCollisionLayer, xiiPhysicsShapeType::Static)))
          continue;

        if (pOutput->m_hSurface.IsValid())
        {
          if (!hitResult.m_hSurface.IsValid())
            continue;

          xiiResourceLock<xiiSurfaceResource> hitSurface(hitResult.m_hSurface, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
          if (hitSurface.GetAcquireResult() == xiiResourceAcquireResult::MissingFallback)
            continue;

          if (!hitSurface->IsBasedOn(pOutput->m_hSurface))
            continue;
        }
      }
      else if (m_pData->m_pOutput->m_Mode == xiiProcPlacementMode::Fixed)
      {
        xiiSimdVec4f rayStart = (vXY + patternCoords * pOutput->m_fFootprint);
        rayStart += xiiSimdRandom::FloatMinMax(xiiSimdVec4i(i), vMinOffset, vMaxOffset, seed);

        hitResult.m_vPosition = xiiSimdConversion::ToVec3(rayStart);
        hitResult.m_fDistance = 0;
        hitResult.m_vNormal.Set(0, 0, 1);
      }

      bool         bInBoundingBox = false;
      xiiSimdVec4f hitPosition    = xiiSimdConversion::ToVec3(hitResult.m_vPosition);
      xiiSimdVec4f allOne         = xiiSimdVec4f(1.0f);
      for (auto& globalToLocalBox : m_pData->m_GlobalToLocalBoxTransforms)
      {
        xiiSimdVec4f localHitPosition = globalToLocalBox.TransformPosition(hitPosition).Abs();
        if ((localHitPosition <= allOne).AllSet<3>())
        {
          bInBoundingBox = true;
          break;
        }
      }

      if (bInBoundingBox)
      {
        PlacementPoint& placementPoint = m_InputPoints.ExpandAndGetRef();
        placementPoint.m_vPosition     = hitResult.m_vPosition;
        placementPoint.m_fScale        = 1.0f;
        placementPoint.m_vNormal       = hitResult.m_vNormal;
        placementPoint.m_uiColorIndex  = 0;
        placementPoint.m_uiObjectIndex = 0;
        placementPoint.m_uiPointIndex  = static_cast<xiiUInt16>(i);
      }
    }
  }

  void PlacementTask::ExecuteVM()
  {
    auto pOutput = m_pData->m_pOutput;

    // Execute bytecode
    if (pOutput->m_pByteCode != nullptr)
    {
      XII_PROFILE_SCOPE("ExecuteVM");

      xiiUInt32 uiNumInstances = m_InputPoints.GetCount();
      m_Density.SetCountUninitialized(uiNumInstances);

      xiiHybridArray<xiiProcessingStream, 8> inputs;
      {
        inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionX, offsetof(PlacementPoint, m_vPosition.x)));
        inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionY, offsetof(PlacementPoint, m_vPosition.y)));
        inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionZ, offsetof(PlacementPoint, m_vPosition.z)));

        inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalX, offsetof(PlacementPoint, m_vNormal.x)));
        inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalY, offsetof(PlacementPoint, m_vNormal.y)));
        inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalZ, offsetof(PlacementPoint, m_vNormal.z)));

        inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPointIndex, offsetof(PlacementPoint, m_uiPointIndex), xiiProcessingStream::DataType::Short));
      }

      xiiHybridArray<xiiProcessingStream, 8> outputs;
      {
        outputs.PushBack(xiiProcessingStream(ExpressionOutputs::s_sOutDensity, m_Density.GetByteArrayPtr(), xiiProcessingStream::DataType::Float));
        outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutScale, offsetof(PlacementPoint, m_fScale)));
        outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutColorIndex, offsetof(PlacementPoint, m_uiColorIndex), xiiProcessingStream::DataType::Byte));
        outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutObjectIndex, offsetof(PlacementPoint, m_uiObjectIndex), xiiProcessingStream::DataType::Byte));
      }

      // Execute expression bytecode
      if (m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumInstances, m_pData->m_GlobalData).Failed())
      {
        return;
      }

      // Test density against point threshold and fill remaining input point data from expression
      float          fObjectCount = static_cast<float>(pOutput->m_ObjectsToPlace.GetCount());
      const Pattern* pPattern     = pOutput->m_pPattern;
      for (xiiUInt32 i = 0; i < uiNumInstances; ++i)
      {
        auto&           inputPoint   = m_InputPoints[i];
        const xiiUInt32 uiPointIndex = inputPoint.m_uiPointIndex;
        const float     fThreshold   = pPattern->m_Points[uiPointIndex].m_fThreshold;

        if (m_Density[i] >= fThreshold)
        {
          m_ValidPoints.PushBack(i);
        }
      }
    }

    if (m_ValidPoints.IsEmpty())
    {
      return;
    }

    XII_PROFILE_SCOPE("Construct final transforms");

    m_OutputTransforms.SetCountUninitialized(m_ValidPoints.GetCount());

    xiiSimdVec4u seed = xiiSimdVec4u(m_pData->m_uiTileSeed) + xiiSimdVec4u(13, 17, 31, 79);

    float fMinAngle = 0.0f;
    float fMaxAngle = xiiMath::Pi<float>() * 2.0f;

    xiiSimdVec4f vMinValue        = xiiSimdVec4f(fMinAngle, pOutput->m_vMinOffset.z, 0.0f);
    xiiSimdVec4f vMaxValue        = xiiSimdVec4f(fMaxAngle, pOutput->m_vMaxOffset.z, 0.0f);
    xiiSimdVec4f vYawRotationSnap = xiiSimdVec4f(pOutput->m_YawRotationSnap);
    xiiSimdVec4f vUp              = xiiSimdVec4f(0, 0, 1);
    xiiSimdVec4f vHalf            = xiiSimdVec4f(0.5f);
    xiiSimdVec4f vAlignToNormal   = xiiSimdVec4f(pOutput->m_fAlignToNormal);
    xiiSimdVec4f vMinScale        = xiiSimdConversion::ToVec3(pOutput->m_vMinScale);
    xiiSimdVec4f vMaxScale        = xiiSimdConversion::ToVec3(pOutput->m_vMaxScale);

    const xiiColorGradient* pColorGradient = nullptr;
    if (pOutput->m_hColorGradient.IsValid())
    {
      xiiResourceLock<xiiColorGradientResource> pColorGradientResource(pOutput->m_hColorGradient, xiiResourceAcquireMode::BlockTillLoaded);
      pColorGradient = &(pColorGradientResource->GetDescriptor().m_Gradient);
    }

    for (xiiUInt32 i = 0; i < m_ValidPoints.GetCount(); ++i)
    {
      xiiUInt32 uiInputPointIndex  = m_ValidPoints[i];
      auto&     placementPoint     = m_InputPoints[uiInputPointIndex];
      auto&     placementTransform = m_OutputTransforms[i];

      xiiSimdVec4f random = xiiSimdRandom::FloatMinMax(xiiSimdVec4i(placementPoint.m_uiPointIndex), vMinValue, vMaxValue, seed);

      xiiSimdVec4f offset = xiiSimdVec4f::ZeroVector();
      offset.SetZ(random.y());
      placementTransform.m_Transform.m_Position = xiiSimdConversion::ToVec3(placementPoint.m_vPosition) + offset;

      xiiSimdVec4f yaw        = xiiSimdVec4f(random.x());
      xiiSimdVec4f roundedYaw = (yaw.CompDiv(vYawRotationSnap) + vHalf).Floor().CompMul(vYawRotationSnap);
      yaw                     = xiiSimdVec4f::Select(vYawRotationSnap == xiiSimdVec4f::ZeroVector(), yaw, roundedYaw);

      xiiSimdQuat qYawRot;
      qYawRot.SetFromAxisAndAngle(vUp, yaw.x());
      xiiSimdVec4f vNormal = xiiSimdConversion::ToVec3(placementPoint.m_vNormal);
      xiiSimdQuat  qToNormalRot;
      qToNormalRot.SetShortestRotation(vUp, xiiSimdVec4f::Lerp(vUp, vNormal, vAlignToNormal));
      placementTransform.m_Transform.m_Rotation = qToNormalRot * qYawRot;

      xiiSimdVec4f scale                     = xiiSimdVec4f(xiiMath::Clamp(placementPoint.m_fScale, 0.0f, 1.0f));
      placementTransform.m_Transform.m_Scale = xiiSimdVec4f::Lerp(vMinScale, vMaxScale, scale);

      placementTransform.m_ObjectColor    = xiiColor::ZeroColor();
      placementTransform.m_uiPointIndex   = placementPoint.m_uiPointIndex;
      placementTransform.m_uiObjectIndex  = placementPoint.m_uiObjectIndex;
      placementTransform.m_bHasValidColor = false;

      if (pColorGradient != nullptr)
      {
        float colorIndex = xiiMath::ColorByteToFloat(placementPoint.m_uiColorIndex);

        xiiColor objectColor;
        xiiUInt8 alpha;
        float    intensity = 1.0f;
        pColorGradient->EvaluateColor(colorIndex, objectColor);
        pColorGradient->EvaluateIntensity(colorIndex, intensity);
        pColorGradient->EvaluateAlpha(colorIndex, alpha);
        objectColor.r *= intensity;
        objectColor.g *= intensity;
        objectColor.b *= intensity;
        objectColor.a = alpha;

        placementTransform.m_ObjectColor    = objectColor;
        placementTransform.m_bHasValidColor = true;
      }
    }
  }

} // namespace xiiProcGenInternal
