#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiColorControlPoint, 2, xiiRTTIDefaultAllocator<xiiColorControlPoint>)
{
  XII_BEGIN_PROPERTIES
  {
    //XII_MEMBER_PROPERTY("Position", m_fPositionX),
    XII_MEMBER_PROPERTY("Tick", m_iTick),
    XII_MEMBER_PROPERTY("Red", m_Red)->AddAttributes(new xiiDefaultValueAttribute(255)),
    XII_MEMBER_PROPERTY("Green", m_Green)->AddAttributes(new xiiDefaultValueAttribute(255)),
    XII_MEMBER_PROPERTY("Blue", m_Blue)->AddAttributes(new xiiDefaultValueAttribute(255)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAlphaControlPoint, 2, xiiRTTIDefaultAllocator<xiiAlphaControlPoint>)
{
  XII_BEGIN_PROPERTIES
  {
    //XII_MEMBER_PROPERTY("Position", m_fPositionX),
    XII_MEMBER_PROPERTY("Tick", m_iTick),
    XII_MEMBER_PROPERTY("Alpha", m_Alpha)->AddAttributes(new xiiDefaultValueAttribute(255)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiIntensityControlPoint, 2, xiiRTTIDefaultAllocator<xiiIntensityControlPoint>)
{
  XII_BEGIN_PROPERTIES
  {
    //XII_MEMBER_PROPERTY("Position", m_fPositionX),
    XII_MEMBER_PROPERTY("Tick", m_iTick),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiColorGradientAssetData, 2, xiiRTTIDefaultAllocator<xiiColorGradientAssetData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("ColorCPs", m_ColorCPs),
    XII_ARRAY_MEMBER_PROPERTY("AlphaCPs", m_AlphaCPs),
    XII_ARRAY_MEMBER_PROPERTY("IntensityCPs", m_IntensityCPs),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiColorGradientAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiColorControlPoint::SetTickFromTime(xiiTime time, xiiInt64 iFps)
{
  const xiiUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick                        = (xiiInt64)xiiMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void xiiAlphaControlPoint::SetTickFromTime(xiiTime time, xiiInt64 iFps)
{
  const xiiUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick                        = (xiiInt64)xiiMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void xiiIntensityControlPoint::SetTickFromTime(xiiTime time, xiiInt64 iFps)
{
  const xiiUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick                        = (xiiInt64)xiiMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

xiiColorGradientAssetDocument::xiiColorGradientAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiColorGradientAssetData>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

void xiiColorGradientAssetDocument::WriteResource(xiiStreamWriter& inout_stream) const
{
  const xiiColorGradientAssetData* pProp = GetProperties();

  xiiColorGradientResourceDescriptor desc;
  pProp->FillGradientData(desc.m_Gradient);
  desc.m_Gradient.SortControlPoints();

  desc.Save(inout_stream);
}

xiiInt64 xiiColorGradientAssetData::TickFromTime(xiiTime time)
{
  /// \todo Make this a property ?
  const xiiUInt32 uiFramesPerSecond = 60;
  const xiiUInt32 uiTicksPerStep    = 4800 / uiFramesPerSecond;
  return (xiiInt64)xiiMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void xiiColorGradientAssetData::FillGradientData(xiiColorGradient& out_result) const
{
  for (const auto& cp : m_ColorCPs)
  {
    out_result.AddColorControlPoint(cp.GetTickAsTime().GetSeconds(), xiiColorGammaUB(cp.m_Red, cp.m_Green, cp.m_Blue));
  }

  for (const auto& cp : m_AlphaCPs)
  {
    out_result.AddAlphaControlPoint(cp.GetTickAsTime().GetSeconds(), cp.m_Alpha);
  }

  for (const auto& cp : m_IntensityCPs)
  {
    out_result.AddIntensityControlPoint(cp.GetTickAsTime().GetSeconds(), cp.m_fIntensity);
  }
}

xiiColor xiiColorGradientAssetData::Evaluate(xiiInt64 iTick) const
{
  xiiColorGradient temp;
  {
    const xiiColorControlPoint* llhs = nullptr;
    const xiiColorControlPoint* lhs  = nullptr;
    const xiiColorControlPoint* rhs  = nullptr;
    const xiiColorControlPoint* rrhs = nullptr;
    FindNearestControlPoints(m_ColorCPs.GetArrayPtr(), iTick, llhs, lhs, rhs, rrhs);

    if (llhs)
      temp.AddColorControlPoint(llhs->GetTickAsTime().GetSeconds(), xiiColorGammaUB(llhs->m_Red, llhs->m_Green, llhs->m_Blue));
    if (lhs)
      temp.AddColorControlPoint(lhs->GetTickAsTime().GetSeconds(), xiiColorGammaUB(lhs->m_Red, lhs->m_Green, lhs->m_Blue));
    if (rhs)
      temp.AddColorControlPoint(rhs->GetTickAsTime().GetSeconds(), xiiColorGammaUB(rhs->m_Red, rhs->m_Green, rhs->m_Blue));
    if (rrhs)
      temp.AddColorControlPoint(rrhs->GetTickAsTime().GetSeconds(), xiiColorGammaUB(rrhs->m_Red, rrhs->m_Green, rrhs->m_Blue));
  }
  {
    const xiiAlphaControlPoint* llhs = nullptr;
    const xiiAlphaControlPoint* lhs  = nullptr;
    const xiiAlphaControlPoint* rhs  = nullptr;
    const xiiAlphaControlPoint* rrhs = nullptr;
    FindNearestControlPoints(m_AlphaCPs.GetArrayPtr(), iTick, llhs, lhs, rhs, rrhs);

    if (llhs)
      temp.AddAlphaControlPoint(llhs->GetTickAsTime().GetSeconds(), llhs->m_Alpha);
    if (lhs)
      temp.AddAlphaControlPoint(lhs->GetTickAsTime().GetSeconds(), lhs->m_Alpha);
    if (rhs)
      temp.AddAlphaControlPoint(rhs->GetTickAsTime().GetSeconds(), rhs->m_Alpha);
    if (rrhs)
      temp.AddAlphaControlPoint(rrhs->GetTickAsTime().GetSeconds(), rrhs->m_Alpha);
  }
  {
    const xiiIntensityControlPoint* llhs = nullptr;
    const xiiIntensityControlPoint* lhs  = nullptr;
    const xiiIntensityControlPoint* rhs  = nullptr;
    const xiiIntensityControlPoint* rrhs = nullptr;
    FindNearestControlPoints(m_IntensityCPs.GetArrayPtr(), iTick, llhs, lhs, rhs, rrhs);

    if (llhs)
      temp.AddIntensityControlPoint(llhs->GetTickAsTime().GetSeconds(), llhs->m_fIntensity);
    if (lhs)
      temp.AddIntensityControlPoint(lhs->GetTickAsTime().GetSeconds(), lhs->m_fIntensity);
    if (rhs)
      temp.AddIntensityControlPoint(rhs->GetTickAsTime().GetSeconds(), rhs->m_fIntensity);
    if (rrhs)
      temp.AddIntensityControlPoint(rrhs->GetTickAsTime().GetSeconds(), rrhs->m_fIntensity);
  }
  xiiColor color;
  // #TODO: This is rather slow as we eval lots of points but only need one
  temp.SortControlPoints();
  temp.Evaluate(iTick / 4800.0, color);
  return color;
}

xiiTransformStatus xiiColorGradientAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  WriteResource(stream);
  return xiiStatus(XII_SUCCESS);
}

xiiTransformStatus xiiColorGradientAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  const xiiColorGradientAssetData* pProp = GetProperties();

  xiiImageHeader imgHeader;
  imgHeader.SetWidth(256);
  imgHeader.SetHeight(256);
  imgHeader.SetImageFormat(xiiImageFormat::R8G8B8A8_UNORM);
  xiiImage img;
  img.ResetAndAlloc(imgHeader);

  xiiColorGradient gradient;
  pProp->FillGradientData(gradient);
  gradient.SortControlPoints();

  double fMin, fMax;
  gradient.GetExtents(fMin, fMax);
  const double range  = fMax - fMin;
  const double div    = 1.0 / (img.GetWidth() - 1);
  const double factor = range * div;

  for (xiiUInt32 x = 0; x < img.GetWidth(); ++x)
  {
    const double pos = fMin + x * factor;

    xiiColorGammaUB color;
    gradient.EvaluateColor(pos, color);

    xiiUInt8 alpha;
    gradient.EvaluateAlpha(pos, alpha);
    const xiiColorLinearUB alphaColor = xiiColorLinearUB(alpha, alpha, alpha, 255);

    const float fAlphaFactor   = xiiMath::ColorByteToFloat(alpha);
    xiiColor    colorWithAlpha = color;
    colorWithAlpha.r *= fAlphaFactor;
    colorWithAlpha.g *= fAlphaFactor;
    colorWithAlpha.b *= fAlphaFactor;

    const xiiColorGammaUB colWithAlpha = colorWithAlpha;

    for (xiiUInt32 y = 0; y < img.GetHeight() / 4; ++y)
    {
      xiiColorGammaUB* pixel = img.GetPixelPointer<xiiColorGammaUB>(0, 0, 0, x, y);
      *pixel                 = alphaColor;
    }

    for (xiiUInt32 y = img.GetHeight() / 4; y < img.GetHeight() / 2; ++y)
    {
      xiiColorGammaUB* pixel = img.GetPixelPointer<xiiColorGammaUB>(0, 0, 0, x, y);
      *pixel                 = colWithAlpha;
    }

    for (xiiUInt32 y = img.GetHeight() / 2; y < img.GetHeight(); ++y)
    {
      xiiColorGammaUB* pixel = img.GetPixelPointer<xiiColorGammaUB>(0, 0, 0, x, y);
      *pixel                 = color;
    }
  }

  return SaveThumbnail(img, ThumbnailInfo);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiColorGradientAssetDataPatch_1_2 : public xiiGraphPatch
{
public:
  xiiColorGradientAssetDataPatch_1_2() :
    xiiGraphPatch("xiiColorGradientAssetData", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Color CPs", "ColorCPs");
    pNode->RenameProperty("Alpha CPs", "AlphaCPs");
    pNode->RenameProperty("Intensity CPs", "IntensityCPs");
  }
};

xiiColorGradientAssetDataPatch_1_2 g_xiiColorGradientAssetDataPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class xiiColorControlPoint_1_2 : public xiiGraphPatch
{
public:
  xiiColorControlPoint_1_2() :
    xiiGraphPatch("xiiColorControlPoint", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", (xiiInt64)xiiMath::RoundToMultiple(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

xiiColorControlPoint_1_2 g_xiiColorControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class xiiAlphaControlPoint_1_2 : public xiiGraphPatch
{
public:
  xiiAlphaControlPoint_1_2() :
    xiiGraphPatch("xiiAlphaControlPoint", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", (xiiInt64)xiiMath::RoundToMultiple(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

xiiAlphaControlPoint_1_2 g_xiiAlphaControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class xiiIntensityControlPoint_1_2 : public xiiGraphPatch
{
public:
  xiiIntensityControlPoint_1_2() :
    xiiGraphPatch("xiiIntensityControlPoint", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", (xiiInt64)xiiMath::RoundToMultiple(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

xiiIntensityControlPoint_1_2 g_xiiIntensityControlPoint_1_2;
