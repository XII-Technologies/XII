#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Volumes/VolumeComponent.h>
#include <GameEngine/Volumes/VolumeSampler.h>

xiiVolumeSampler::xiiVolumeSampler()  = default;
xiiVolumeSampler::~xiiVolumeSampler() = default;

void xiiVolumeSampler::RegisterValue(xiiHashedString sName, xiiVariant defaultValue, xiiTime interpolationDuration /*= xiiTime::MakeZero()*/)
{
  auto& value          = m_Values[sName];
  value.m_DefaultValue = defaultValue;
  value.m_TargetValue  = defaultValue;
  value.m_CurrentValue = defaultValue;

  if (interpolationDuration.IsPositive())
  {
    // Reach 90% of target value after interpolation duration:
    // Lerp factor for exponential moving average:
    // y = 1-(1-f)^t
    // solve for f with y = 0.9:
    // f = 1 - 10^(-1 / t)
    value.m_fInterpolationFactor = 1.0 - xiiMath::Pow(10.0, -1.0 / interpolationDuration.GetSeconds());
  }
  else
  {
    value.m_fInterpolationFactor = -1.0;
  }
}

void xiiVolumeSampler::DeregisterValue(xiiHashedString sName)
{
  m_Values.Remove(sName);
}

void xiiVolumeSampler::DeregisterAllValues()
{
  m_Values.Clear();
}

void xiiVolumeSampler::SampleAtPosition(const xiiWorld& world, xiiSpatialData::Category spatialCategory, const xiiVec3& vGlobalPosition, xiiTime deltaTime)
{
  struct ComponentInfo
  {
    const xiiVolumeComponent* m_pComponent   = nullptr;
    xiiUInt32                 m_uiSortingKey = 0;
    float                     m_fAlpha       = 0.0f;

    bool operator<(const ComponentInfo& other) const
    {
      return m_uiSortingKey < other.m_uiSortingKey;
    }
  };

  auto              vPos   = xiiSimdConversion::ToVec3(vGlobalPosition);
  xiiBoundingSphere sphere = xiiBoundingSphere::MakeFromCenterAndRadius(vGlobalPosition, 0.01f);

  xiiSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = spatialCategory.GetBitmask();

  xiiHybridArray<ComponentInfo, 16> componentInfos;
  world.GetSpatialSystem()->FindObjectsInSphere(sphere, queryParams, [&](xiiGameObject* pObject) {
      xiiVolumeComponent* pComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pComponent))
      {
        ComponentInfo info;
        info.m_pComponent = pComponent;

        xiiSimdTransform scaledTransform = pComponent->GetOwner()->GetGlobalTransformSimd();

        if (auto pBoxComponent = xiiDynamicCast<const xiiVolumeBoxComponent*>(pComponent))
        {
          scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(xiiSimdConversion::ToVec3(pBoxComponent->GetExtents())) * 0.5f;

          xiiSimdMat4f globalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
          const xiiSimdVec4f absLocalPos = globalToLocalTransform.TransformPosition(vPos).Abs();
          if ((absLocalPos <= xiiSimdVec4f(1.0f)).AllSet<3>())
          {
            xiiSimdVec4f vAlpha = (xiiSimdVec4f(1.0f) - absLocalPos).CompDiv(xiiSimdConversion::ToVec3(pBoxComponent->GetFalloff().CompMax(xiiVec3(0.0001f))));
            vAlpha              = vAlpha.CompMin(xiiSimdVec4f(1.0f)).CompMax(xiiSimdVec4f::MakeZero());
            info.m_fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
          }
        }
        else if (auto pSphereComponent = xiiDynamicCast<const xiiVolumeSphereComponent*>(pComponent))
        {
          scaledTransform.m_Scale *= pSphereComponent->GetRadius();

          xiiSimdMat4f globalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
          const xiiSimdVec4f localPos = globalToLocalTransform.TransformPosition(vPos);
          const float distSquared = localPos.GetLengthSquared<3>();
          if (distSquared <= 1.0f)
          {
            info.m_fAlpha = xiiMath::Saturate((1.0f - xiiMath::Sqrt(distSquared)) / pSphereComponent->GetFalloff());
          }
        }
        else
        {
          XII_ASSERT_NOT_IMPLEMENTED;
        }

        if (info.m_fAlpha > 0.0f)
        {
          info.m_uiSortingKey = ComputeSortingKey(pComponent->GetSortOrder(), scaledTransform.GetMaxScale());

          componentInfos.PushBack(info);
        }
      }

      return xiiVisitorExecution::Continue; });

  // Sort
  {
    componentInfos.Sort();
  }

  for (auto& it : m_Values)
  {
    auto& sName = it.Key();
    auto& value = it.Value();

    value.m_TargetValue = value.m_DefaultValue;

    for (auto& info : componentInfos)
    {
      xiiVariant volumeValue = info.m_pComponent->GetValue(sName);
      if (volumeValue.IsValid() == false)
        continue;

      xiiResult               conversionStatus = XII_SUCCESS;
      xiiEnum<xiiVariantType> targetType       = value.m_TargetValue.GetType();
      xiiVariant              newTargetValue   = volumeValue.ConvertTo(targetType, &conversionStatus);
      if (conversionStatus.Failed())
      {
        xiiLog::Error("VolumeSampler: Can't convert volume value '{}' to '{}'.", sName, targetType);
        continue;
      }

      value.m_TargetValue = xiiMath::Lerp(value.m_TargetValue, newTargetValue, double(info.m_fAlpha));
    }

    if (value.m_fInterpolationFactor > 0.0)
    {
      double f             = 1.0 - xiiMath::Pow(1.0 - value.m_fInterpolationFactor, deltaTime.GetSeconds());
      value.m_CurrentValue = xiiMath::Lerp(value.m_CurrentValue, value.m_TargetValue, f);
    }
    else
    {
      value.m_CurrentValue = value.m_TargetValue;
    }
  }
}

// static
xiiUInt32 xiiVolumeSampler::ComputeSortingKey(float fSortOrder, float fMaxScale)
{
  xiiUInt32 uiSortingKey = (xiiUInt32)(xiiMath::Min(fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  uiSortingKey           = (uiSortingKey << 16) | (0xFFFF - ((xiiUInt32)(fMaxScale * 100.0f) & 0xFFFF));
  return uiSortingKey;
}
