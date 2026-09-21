/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/SpatialData.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

/// A volume sampler is used to sample the registered values from volumes at a given position. It also takes care of interpolation over time of those values.
class XII_GAMEENGINE_DLL xiiVolumeSampler
{
public:
  xiiVolumeSampler();
  ~xiiVolumeSampler();

  void RegisterValue(xiiHashedString sName, xiiVariant defaultValue, xiiTime interpolationDuration = xiiTime::MakeZero());
  void DeregisterValue(xiiHashedString sName);
  void DeregisterAllValues();

  void SampleAtPosition(const xiiWorld& world, xiiSpatialData::Category spatialCategory, const xiiVec3& vGlobalPosition, xiiTime deltaTime);

  xiiVariant GetValue(xiiTempHashedString sName) const
  {
    if (const Value* pValue = m_Values.GetValue(sName))
    {
      return pValue->m_CurrentValue;
    }

    return xiiVariant();
  }

  static xiiUInt32 ComputeSortingKey(float fSortOrder, float fMaxScale);

private:
  struct Value
  {
    xiiVariant m_DefaultValue;
    xiiVariant m_TargetValue;
    xiiVariant m_CurrentValue;
    double     m_fInterpolationFactor = -1.0;
  };

  xiiHashTable<xiiHashedString, Value> m_Values;
};
