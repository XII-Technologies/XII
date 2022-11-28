#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/Math.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiCurveTangentMode, 1)
XII_ENUM_CONSTANTS(xiiCurveTangentMode::Bezier, xiiCurveTangentMode::FixedLength, xiiCurveTangentMode::Linear, xiiCurveTangentMode::Auto)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCurveControlPointData, 5, xiiRTTIDefaultAllocator<xiiCurveControlPointData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Tick", m_iTick),
    XII_MEMBER_PROPERTY("Value", m_fValue),
    XII_MEMBER_PROPERTY("LeftTangent", m_LeftTangent)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(-0.1f, 0))),
    XII_MEMBER_PROPERTY("RightTangent", m_RightTangent)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(+0.1f, 0))),
    XII_MEMBER_PROPERTY("Linked", m_bTangentsLinked)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ENUM_MEMBER_PROPERTY("LeftTangentMode", xiiCurveTangentMode, m_LeftTangentMode),
    XII_ENUM_MEMBER_PROPERTY("RightTangentMode", xiiCurveTangentMode, m_RightTangentMode),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSingleCurveData, 3, xiiRTTIDefaultAllocator<xiiSingleCurveData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_CurveColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColorScheme::LightUI(xiiColorScheme::Lime))),
    XII_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCurveGroupData, 2, xiiRTTIDefaultAllocator<xiiCurveGroupData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FPS", m_uiFramesPerSecond)->AddAttributes(new xiiDefaultValueAttribute(60)),
    XII_ARRAY_MEMBER_PROPERTY("Curves", m_Curves)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCurveExtentsAttribute, 1, xiiRTTIDefaultAllocator<xiiCurveExtentsAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LowerExtent", m_fLowerExtent),
    XII_MEMBER_PROPERTY("UpperExtent", m_fUpperExtent),
    XII_MEMBER_PROPERTY("LowerExtentFixed", m_bLowerExtentFixed),
    XII_MEMBER_PROPERTY("UpperExtentFixed", m_bUpperExtentFixed),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float, bool, float, bool),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCurveExtentsAttribute::xiiCurveExtentsAttribute(double fLowerExtent, bool bLowerExtentFixed, double fUpperExtent, bool bUpperExtentFixed)
{
  m_fLowerExtent      = fLowerExtent;
  m_fUpperExtent      = fUpperExtent;
  m_bLowerExtentFixed = bLowerExtentFixed;
  m_bUpperExtentFixed = bUpperExtentFixed;
}

void xiiCurveControlPointData::SetTickFromTime(xiiTime time, xiiInt64 fps)
{
  const xiiUInt32 uiTicksPerStep = 4800 / fps;
  m_iTick                        = (xiiInt64)xiiMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

xiiCurveGroupData::~xiiCurveGroupData()
{
  Clear();
}

void xiiCurveGroupData::CloneFrom(const xiiCurveGroupData& rhs)
{
  Clear();

  m_bOwnsData         = true;
  m_uiFramesPerSecond = rhs.m_uiFramesPerSecond;
  m_Curves.SetCount(rhs.m_Curves.GetCount());

  for (xiiUInt32 i = 0; i < m_Curves.GetCount(); ++i)
  {
    m_Curves[i]  = XII_DEFAULT_NEW(xiiSingleCurveData);
    *m_Curves[i] = *(rhs.m_Curves[i]);
  }
}

void xiiCurveGroupData::Clear()
{
  m_uiFramesPerSecond = 60;

  if (m_bOwnsData)
  {
    m_bOwnsData = false;

    for (xiiUInt32 i = 0; i < m_Curves.GetCount(); ++i)
    {
      XII_DEFAULT_DELETE(m_Curves[i]);
    }
  }

  m_Curves.Clear();
}

xiiInt64 xiiCurveGroupData::TickFromTime(xiiTime time) const
{
  const xiiUInt32 uiTicksPerStep = 4800 / m_uiFramesPerSecond;
  return (xiiInt64)xiiMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

static void ConvertControlPoint(const xiiCurveControlPointData& cp, xiiCurve1D& out_Result)
{
  auto& ccp              = out_Result.AddControlPoint(cp.GetTickAsTime().GetSeconds());
  ccp.m_Position.y       = cp.m_fValue;
  ccp.m_LeftTangent      = cp.m_LeftTangent;
  ccp.m_RightTangent     = cp.m_RightTangent;
  ccp.m_TangentModeLeft  = cp.m_LeftTangentMode;
  ccp.m_TangentModeRight = cp.m_RightTangentMode;
}

void xiiSingleCurveData::ConvertToRuntimeData(xiiCurve1D& out_Result) const
{
  out_Result.Clear();

  for (const auto& cp : m_ControlPoints)
  {
    ConvertControlPoint(cp, out_Result);
  }
}

double xiiSingleCurveData::Evaluate(xiiInt64 iTick) const
{
  xiiCurve1D                      temp;
  const xiiCurveControlPointData* llhs = nullptr;
  const xiiCurveControlPointData* lhs  = nullptr;
  const xiiCurveControlPointData* rhs  = nullptr;
  const xiiCurveControlPointData* rrhs = nullptr;
  FindNearestControlPoints(m_ControlPoints.GetArrayPtr(), iTick, llhs, lhs, rhs, rrhs);

  if (llhs)
    ConvertControlPoint(*llhs, temp);
  if (lhs)
    ConvertControlPoint(*lhs, temp);
  if (rhs)
    ConvertControlPoint(*rhs, temp);
  if (rrhs)
    ConvertControlPoint(*rrhs, temp);

  //#TODO: This is rather slow as we eval lots of points but only need one
  temp.CreateLinearApproximation();
  return temp.Evaluate(iTick / 4800.0);
}

void xiiCurveGroupData::ConvertToRuntimeData(xiiUInt32 uiCurveIdx, xiiCurve1D& out_Result) const
{
  m_Curves[uiCurveIdx]->ConvertToRuntimeData(out_Result);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiCurve1DControlPoint_2_3 : public xiiGraphPatch
{
public:
  xiiCurve1DControlPoint_2_3() :
    xiiGraphPatch("xiiCurve1DControlPoint", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Point");
    if (pPoint && pPoint->m_Value.IsA<xiiVec2>())
    {
      xiiVec2 pt = pPoint->m_Value.Get<xiiVec2>();
      pNode->AddProperty("Time", (double)xiiMath::Max(0.0f, pt.x));
      pNode->AddProperty("Value", (double)pt.y);
      pNode->AddProperty("LeftTangentMode", (xiiUInt32)xiiCurveTangentMode::Bezier);
      pNode->AddProperty("RightTangentMode", (xiiUInt32)xiiCurveTangentMode::Bezier);
    }
  }
};

xiiCurve1DControlPoint_2_3 g_xiiCurve1DControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class xiiCurve1DControlPoint_3_4 : public xiiGraphPatch
{
public:
  xiiCurve1DControlPoint_3_4() :
    xiiGraphPatch("xiiCurve1DControlPoint", 4)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Time");
    if (pPoint && pPoint->m_Value.IsA<double>())
    {
      const double fTime = pPoint->m_Value.Get<double>();
      pNode->AddProperty("Tick", (xiiInt64)xiiMath::RoundToMultiple(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

xiiCurve1DControlPoint_3_4 g_xiiCurve1DControlPoint_3_4;

//////////////////////////////////////////////////////////////////////////

class xiiCurve1DControlPoint_4_5 : public xiiGraphPatch
{
public:
  xiiCurve1DControlPoint_4_5() :
    xiiGraphPatch("xiiCurve1DControlPoint", 5)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    context.RenameClass("xiiCurveControlPointData");
  }
};

xiiCurve1DControlPoint_4_5 g_xiiCurve1DControlPoint_4_5;

//////////////////////////////////////////////////////////////////////////

class xiiCurve1DData_2_3 : public xiiGraphPatch
{
public:
  xiiCurve1DData_2_3() :
    xiiGraphPatch("xiiCurve1DData", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    context.RenameClass("xiiSingleCurveData");
  }
};

xiiCurve1DData_2_3 g_xiiCurve1DData_2_3;

//////////////////////////////////////////////////////////////////////////

class xiiCurve1DAssetData_1_2 : public xiiGraphPatch
{
public:
  xiiCurve1DAssetData_1_2() :
    xiiGraphPatch("xiiCurve1DAssetData", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    context.RenameClass("xiiCurveGroupData");
  }
};

xiiCurve1DAssetData_1_2 g_xiiCurve1DAssetData_1_2;
