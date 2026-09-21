/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiCurve1D;

XII_DECLARE_REFLECTABLE_TYPE(XII_GUIFOUNDATION_DLL, xiiCurveTangentMode);

template <typename T>
void FindNearestControlPoints(xiiArrayPtr<T> cps, xiiInt64 iTick, T*& ref_pLlhs, T*& lhs, T*& rhs, T*& ref_pRrhs)
{
  ref_pLlhs         = nullptr;
  lhs               = nullptr;
  rhs               = nullptr;
  ref_pRrhs         = nullptr;
  xiiInt64 lhsTick  = xiiMath::MinValue<xiiInt64>();
  xiiInt64 llhsTick = xiiMath::MinValue<xiiInt64>();
  xiiInt64 rhsTick  = xiiMath::MaxValue<xiiInt64>();
  xiiInt64 rrhsTick = xiiMath::MaxValue<xiiInt64>();

  for (decltype(auto) cp : cps)
  {
    if (cp.m_iTick <= iTick)
    {
      if (cp.m_iTick > lhsTick)
      {
        ref_pLlhs = lhs;
        llhsTick  = lhsTick;

        lhs     = &cp;
        lhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick > llhsTick)
      {
        ref_pLlhs = &cp;
        llhsTick  = cp.m_iTick;
      }
    }

    if (cp.m_iTick > iTick)
    {
      if (cp.m_iTick < rhsTick)
      {
        ref_pRrhs = rhs;
        rrhsTick  = rhsTick;

        rhs     = &cp;
        rhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick < rrhsTick)
      {
        ref_pRrhs = &cp;
        rrhsTick  = cp.m_iTick;
      }
    }
  }
}

class XII_GUIFOUNDATION_DLL xiiCurveControlPointData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCurveControlPointData, xiiReflectedClass);

public:
  xiiTime GetTickAsTime() const { return xiiTime::MakeFromSeconds(m_iTick / 4800.0); }
  void    SetTickFromTime(xiiTime time, xiiInt64 iFps);

  xiiInt64                     m_iTick; // 4800 ticks per second
  double                       m_fValue;
  xiiVec2                      m_LeftTangent     = xiiVec2(-0.1f, 0.0f);
  xiiVec2                      m_RightTangent    = xiiVec2(+0.1f, 0.0f);
  bool                         m_bTangentsLinked = true;
  xiiEnum<xiiCurveTangentMode> m_LeftTangentMode;
  xiiEnum<xiiCurveTangentMode> m_RightTangentMode;
};

class XII_GUIFOUNDATION_DLL xiiSingleCurveData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSingleCurveData, xiiReflectedClass);

public:
  xiiColorGammaUB                           m_CurveColor;
  xiiDynamicArray<xiiCurveControlPointData> m_ControlPoints;

  void   ConvertToRuntimeData(xiiCurve1D& out_result) const;
  double Evaluate(xiiInt64 iTick) const;
};

class XII_GUIFOUNDATION_DLL xiiCurveExtentsAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCurveExtentsAttribute, xiiPropertyAttribute);

public:
  xiiCurveExtentsAttribute() = default;
  xiiCurveExtentsAttribute(double fLowerExtent, bool bLowerExtentFixed, double fUpperExtent, bool bUpperExtentFixed);

  double m_fLowerExtent      = 0.0;
  double m_fUpperExtent      = 1.0;
  bool   m_bLowerExtentFixed = false;
  bool   m_bUpperExtentFixed = false;
};


class XII_GUIFOUNDATION_DLL xiiCurveGroupData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCurveGroupData, xiiReflectedClass);

public:
  xiiCurveGroupData()                             = default;
  xiiCurveGroupData(const xiiCurveGroupData& rhs) = delete;
  ~xiiCurveGroupData();
  xiiCurveGroupData& operator=(const xiiCurveGroupData& rhs) = delete;

  /// Makes a deep copy of rhs.
  void CloneFrom(const xiiCurveGroupData& rhs);

  /// Clears the curve and deallocates the curve data, if it is owned (e.g. if it was created through CloneFrom())
  void Clear();

  /// Can be set to false for cases where the instance is only supposed to act like a container for passing curve pointers around
  bool                                 m_bOwnsData = true;
  xiiDynamicArray<xiiSingleCurveData*> m_Curves;
  xiiUInt16                            m_uiFramesPerSecond = 60;

  xiiInt64 TickFromTime(xiiTime time) const;

  void ConvertToRuntimeData(xiiUInt32 uiCurveIdx, xiiCurve1D& out_result) const;
};

struct XII_GUIFOUNDATION_DLL xiiSelectedCurveCP
{
  XII_DECLARE_POD_TYPE();

  xiiUInt16 m_uiCurve;
  xiiUInt16 m_uiPoint;
};
