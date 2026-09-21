/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

class xiiCurve1D;

class xiiCurve1DAssetDocument : public xiiSimpleAssetDocument<xiiCurveGroupData>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCurve1DAssetDocument, xiiSimpleAssetDocument<xiiCurveGroupData>);

public:
  xiiCurve1DAssetDocument(xiiStringView sDocumentPath);
  ~xiiCurve1DAssetDocument();

  /// Fills out the xiiCurve1D structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the curve, that must be called manually.
  void FillCurve(xiiUInt32 uiCurveIdx, xiiCurve1D& out_result) const;

  xiiUInt32 GetCurveCount() const;

  void WriteResource(xiiStreamWriter& inout_stream) const;

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
