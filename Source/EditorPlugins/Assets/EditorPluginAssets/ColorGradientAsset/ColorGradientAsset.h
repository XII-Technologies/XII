/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class xiiColorGradient;

class xiiColorControlPoint : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiColorControlPoint, xiiReflectedClass);

public:
  xiiTime GetTickAsTime() const { return xiiTime::MakeFromSeconds(m_iTick / 4800.0); }
  void    SetTickFromTime(xiiTime time, xiiInt64 iFps);

  // double m_fPositionX;
  xiiInt64 m_iTick; // 4800 ticks per second
  xiiUInt8 m_Red;
  xiiUInt8 m_Green;
  xiiUInt8 m_Blue;
};

class xiiAlphaControlPoint : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAlphaControlPoint, xiiReflectedClass);

public:
  xiiTime GetTickAsTime() const { return xiiTime::MakeFromSeconds(m_iTick / 4800.0); }
  void    SetTickFromTime(xiiTime time, xiiInt64 iFps);

  // double m_fPositionX;
  xiiInt64 m_iTick; // 4800 ticks per second
  xiiUInt8 m_Alpha;
};

class xiiIntensityControlPoint : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiIntensityControlPoint, xiiReflectedClass);

public:
  xiiTime GetTickAsTime() const { return xiiTime::MakeFromSeconds(m_iTick / 4800.0); }
  void    SetTickFromTime(xiiTime time, xiiInt64 iFps);

  // double m_fPositionX;
  xiiInt64 m_iTick; // 4800 ticks per second
  float    m_fIntensity;
};

class xiiColorGradientAssetData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiColorGradientAssetData, xiiReflectedClass);

public:
  xiiDynamicArray<xiiColorControlPoint>     m_ColorCPs;
  xiiDynamicArray<xiiAlphaControlPoint>     m_AlphaCPs;
  xiiDynamicArray<xiiIntensityControlPoint> m_IntensityCPs;

  static xiiInt64 TickFromTime(xiiTime time);

  /// Fills out the xiiColorGradient structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the color gradient, that must be called manually.
  void     FillGradientData(xiiColorGradient& out_result) const;
  xiiColor Evaluate(xiiInt64 iTick) const;
};

class xiiColorGradientAssetDocument : public xiiSimpleAssetDocument<xiiColorGradientAssetData>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiColorGradientAssetDocument, xiiSimpleAssetDocument<xiiColorGradientAssetData>);

public:
  xiiColorGradientAssetDocument(xiiStringView sDocumentPath);

  void WriteResource(xiiStreamWriter& inout_stream) const;

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
