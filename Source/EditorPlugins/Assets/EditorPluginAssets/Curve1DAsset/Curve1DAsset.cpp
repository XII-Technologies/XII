#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCurve1DAssetDocument, 3, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiCurve1DAssetDocument::xiiCurve1DAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiCurveGroupData>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiCurve1DAssetDocument::~xiiCurve1DAssetDocument() = default;

void xiiCurve1DAssetDocument::FillCurve(xiiUInt32 uiCurveIdx, xiiCurve1D& out_result) const
{
  const xiiCurveGroupData* pProp = static_cast<const xiiCurveGroupData*>(GetProperties());
  pProp->ConvertToRuntimeData(uiCurveIdx, out_result);
}

xiiUInt32 xiiCurve1DAssetDocument::GetCurveCount() const
{
  const xiiCurveGroupData* pProp = GetProperties();
  return pProp->m_Curves.GetCount();
}

void xiiCurve1DAssetDocument::WriteResource(xiiStreamWriter& inout_stream) const
{
  const xiiCurveGroupData* pProp = GetProperties();

  xiiCurve1DResourceDescriptor desc;
  desc.m_Curves.SetCount(pProp->m_Curves.GetCount());

  for (xiiUInt32 i = 0; i < pProp->m_Curves.GetCount(); ++i)
  {
    FillCurve(i, desc.m_Curves[i]);
    desc.m_Curves[i].SortControlPoints();
  }

  desc.Save(inout_stream);
}

xiiTransformStatus xiiCurve1DAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  WriteResource(stream);
  return XII_SUCCESS;
}

xiiTransformStatus xiiCurve1DAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  const xiiCurveGroupData* pProp = GetProperties();

  QImage qimg(xiiThumbnailSize, xiiThumbnailSize, QImage::Format_RGBA8888);
  qimg.fill(QColor(50, 50, 50));

  QPainter  p(&qimg);
  QPainter* painter = &p;
  painter->setBrush(Qt::NoBrush);
  painter->setRenderHint(QPainter::Antialiasing);

  if (!pProp->m_Curves.IsEmpty())
  {

    double fExtentsMin, fExtentsMax;
    double fExtremesMin, fExtremesMax;

    for (xiiUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
    {
      xiiCurve1D curve;
      FillCurve(curveIdx, curve);

      curve.SortControlPoints();
      curve.CreateLinearApproximation();

      double fMin, fMax;
      curve.QueryExtents(fMin, fMax);

      double fMin2, fMax2;
      curve.QueryExtremeValues(fMin2, fMax2);

      if (curveIdx == 0)
      {
        fExtentsMin  = fMin;
        fExtentsMax  = fMax;
        fExtremesMin = fMin2;
        fExtremesMax = fMax2;
      }
      else
      {
        fExtentsMin  = xiiMath::Min(fExtentsMin, fMin);
        fExtentsMax  = xiiMath::Max(fExtentsMax, fMax);
        fExtremesMin = xiiMath::Min(fExtremesMin, fMin2);
        fExtremesMax = xiiMath::Max(fExtremesMax, fMax2);
      }
    }

    const float range  = fExtentsMax - fExtentsMin;
    const float div    = 1.0f / (qimg.width() - 1);
    const float factor = range * div;

    const float lowValue  = (fExtremesMin > 0) ? 0.0f : fExtremesMin;
    const float highValue = (fExtremesMax < 0) ? 0.0f : fExtremesMax;

    const float range2 = highValue - lowValue;

    for (xiiUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
    {
      QPainterPath path;

      xiiCurve1D curve;
      FillCurve(curveIdx, curve);
      curve.SortControlPoints();
      curve.CreateLinearApproximation();

      const QColor curColor = xiiToQtColor(pProp->m_Curves[curveIdx]->m_CurveColor);
      QPen         pen(curColor, 8.0f);
      painter->setPen(pen);

      for (xiiUInt32 x = 0; x < (xiiUInt32)qimg.width(); ++x)
      {
        const float pos   = fExtentsMin + x * factor;
        const float value = 1.0f - (curve.Evaluate(pos) - lowValue) / range2;

        const xiiUInt32 y = xiiMath::Clamp<xiiUInt32>(qimg.height() * value, 0, qimg.height() - 1);

        if (x == 0)
          path.moveTo(x, y);
        else
          path.lineTo(x, y);
      }

      painter->drawPath(path);
    }
  }

  return SaveThumbnail(qimg, ThumbnailInfo);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiCurve1DControlPointPatch_1_2 : public xiiGraphPatch
{
public:
  xiiCurve1DControlPointPatch_1_2() :
    xiiGraphPatch("xiiCurve1DControlPoint", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Left Tangent", "LeftTangent");
    pNode->RenameProperty("Right Tangent", "RightTangent");
  }
};

xiiCurve1DControlPointPatch_1_2 g_xiiCurve1DControlPointPatch_1_2;


class xiiCurve1DDataPatch_1_2 : public xiiGraphPatch
{
public:
  xiiCurve1DDataPatch_1_2() :
    xiiGraphPatch("xiiCurve1DData", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override { pNode->RenameProperty("Control Points", "ControlPoints"); }
};

xiiCurve1DDataPatch_1_2 g_xiiCurve1DDataPatch_1_2;
