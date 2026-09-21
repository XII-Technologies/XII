/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/Utilities/EnumerableClass.h>

#include <Texture/Image/Image.h>

XII_DECLARE_FLAGS(xiiUInt8, xiiImageConversionFlags, InPlace);

XII_DECLARE_REFLECTABLE_TYPE(XII_TEXTURE_DLL, xiiImageConversionFlags);

/// A structure describing the pairs of source/target format that may be converted using the conversion routine.
struct XII_TEXTURE_DLL xiiImageConversionEntry
{
  xiiImageConversionEntry(xiiEnum<xiiGALResourceFormat> source, xiiEnum<xiiGALResourceFormat> target, xiiBitflags<xiiImageConversionFlags> flags, float fAdditionalPenalty = 0) :
    m_SourceFormat(source), m_TargetFormat(target), m_Flags(flags), m_fAdditionalPenalty(fAdditionalPenalty)
  {
  }

  const xiiEnum<xiiGALResourceFormat>        m_SourceFormat;              ///< The source and target format of this conversion step. The actual conversion functionality is implemented in the derived classes of xiiImageConversionStep, which advertise the supported conversions through GetSupportedConversions().
  const xiiEnum<xiiGALResourceFormat>        m_TargetFormat;              ///< The source and target format of this conversion step. The actual conversion functionality is implemented in the derived classes of xiiImageConversionStep, which advertise the supported conversions through GetSupportedConversions().
  const xiiBitflags<xiiImageConversionFlags> m_Flags;                     ///< This member describes properties of the conversion step, e.g. whether it can be performed in-place or not.
  float                                      m_fAdditionalPenalty = 0.0f; ///< This member adds an additional amount to the cost estimate for this conversion step. It can be used to bias the choice between steps when there are comparable conversion steps available.
};

/// Interface for a single image conversion step.
///
/// The actual functionality is implemented as either xiiImageConversionStepLinear or xiiImageConversionStepDecompressBlocks.
/// Depending on the types on conversion advertised by GetSupportedConversions(), users of this class need to cast it to a derived type
/// first to access the desired functionality.
class XII_TEXTURE_DLL xiiImageConversionStep : public xiiEnumerable<xiiImageConversionStep>
{
  XII_DECLARE_ENUMERABLE_CLASS(xiiImageConversionStep);

protected:
  xiiImageConversionStep();
  virtual ~xiiImageConversionStep();

public:
  /// Returns an array pointer of supported conversions.
  ///
  /// \note The returned array must have the same entries each time this method is called.
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const = 0;
};

/// Interface for a single image conversion step where both the source and target format are uncompressed.
class XII_TEXTURE_DLL xiiImageConversionStepLinear : public xiiImageConversionStep
{
public:
  /// Converts a batch of pixels.
  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const = 0;
};

/// Interface for a single image conversion step where the source format is compressed and the target format is uncompressed.
class XII_TEXTURE_DLL xiiImageConversionStepDecompressBlocks : public xiiImageConversionStep
{
public:
  /// Decompresses the given number of blocks.
  virtual xiiResult DecompressBlocks(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt32 uiNumBlocks, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const = 0;
};

/// Interface for a single image conversion step where the source format is uncompressed and the target format is compressed.
class XII_TEXTURE_DLL xiiImageConversionStepCompressBlocks : public xiiImageConversionStep
{
public:
  /// Compresses the given number of blocks.
  virtual xiiResult CompressBlocks(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt32 uiNumBlocksX, xiiUInt32 uiNumBlocksY, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const = 0;
};

/// Interface for a single image conversion step from a linear to a planar format.
class XII_TEXTURE_DLL xiiImageConversionStepPlanarize : public xiiImageConversionStep
{
public:
  /// Converts a batch of pixels into the given target planes.
  virtual xiiResult ConvertPixels(const xiiImageView& source, xiiArrayPtr<xiiImage> pTarget, xiiUInt32 uiNumPixelsX, xiiUInt32 uiNumPixelsY, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const = 0;
};

/// Interface for a single image conversion step from a planar to a linear format.
class XII_TEXTURE_DLL xiiImageConversionStepDeplanarize : public xiiImageConversionStep
{
public:
  /// Converts a batch of pixels from the given source planes.
  virtual xiiResult ConvertPixels(xiiArrayPtr<xiiImageView> pSource, xiiImage target, xiiUInt32 uiNumPixelsX, xiiUInt32 uiNumPixelsY, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const = 0;
};

/// Helper class containing utilities to convert between different image formats and layouts.
class XII_TEXTURE_DLL xiiImageConversion
{
public:
  /// Checks if there is a known conversion path between the two formats
  static bool IsConvertible(xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat);

  /// Finds the image format from a given list of formats which is the cheapest to convert to.
  static xiiEnum<xiiGALResourceFormat> FindClosestCompatibleFormat(xiiEnum<xiiGALResourceFormat> format, xiiArrayPtr<const xiiEnum<xiiGALResourceFormat>> pCompatibleFormats);

  /// A single node along a computed conversion path.
  struct ConversionPathNode
  {
    XII_DECLARE_POD_TYPE();

    const xiiImageConversionStep* m_pStep;               ///< The conversion step to perform for this node. The actual conversion functionality is implemented in the derived classes of xiiImageConversionStep, which advertise the supported conversions through GetSupportedConversions().
    xiiEnum<xiiGALResourceFormat> m_SourceFormat;        ///< The source format for this conversion step.
    xiiEnum<xiiGALResourceFormat> m_TargetFormat;        ///< The target format for this conversion step.
    xiiUInt32                     m_uiSourceBufferIndex; ///< The index of the source buffer for this conversion step. 0 means the original source data, 1 means the first scratch buffer, etc.
    xiiUInt32                     m_uiTargetBufferIndex; ///< The index of the target buffer for this conversion step. 0 means the final target data, 1 means the first scratch buffer, etc.
    bool                          m_bInPlace;            ///< Whether this conversion step can be performed in-place, i.e. whether the source and target buffer can be the same. If false, the source and target buffer must not be the same for this conversion step.
  };

  /// Precomputes an optimal conversion path between two formats and the minimal number of required scratch buffers.
  ///
  /// The generated path can be cached by the user if the same conversion is performed multiple times. The path must not be reused if the
  /// set of supported conversions changes, e.g. when plugins are loaded or unloaded.
  ///
  /// \param sourceFormat           The source format.
  /// \param targetFormat           The target format.
  /// \param sourceEqualsTarget     If true, the generated path is applicable if source and target memory regions are equal, and may contain additional copy-steps if the conversion can't be performed in-place.
  ///                               A path generated with sourceEqualsTarget == true will work correctly even if source and target are not the same, but may not be optimal. A path generated with sourceEqualsTarget == false will not work correctly when source and target are the same.
  /// \param out_path               The generated path.
  /// \param out_numScratchBuffers  The number of scratch buffers required for the conversion path.
  /// \returns                      xii_SUCCESS if a path was found, xii_FAILURE otherwise.
  static xiiResult BuildPath(xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, bool bSourceEqualsTarget, xiiHybridArray<ConversionPathNode, 16>& out_path, xiiUInt32& out_uiNumScratchBuffers);

  ///  Converts the source image into a target image with the given format. Source and target may be the same.
  static xiiResult Convert(const xiiImageView& source, xiiImage& ref_target, xiiEnum<xiiGALResourceFormat> targetFormat);

  /// Converts the source image into a target image using a precomputed conversion path.
  static xiiResult Convert(const xiiImageView& source, xiiImage& ref_target, xiiArrayPtr<ConversionPathNode> pPath, xiiUInt32 uiNumScratchBuffers);

  /// Converts the raw source data into a target data buffer with the given format. Source and target may be the same.
  static xiiResult ConvertRaw(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt32 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat);

  /// Converts the raw source data into a target data buffer using a precomputed conversion path.
  static xiiResult ConvertRaw(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt32 uiElementCount, xiiArrayPtr<ConversionPathNode> pPath, xiiUInt32 uiNumScratchBuffers);

private:
  xiiImageConversion();
  xiiImageConversion(const xiiImageConversion&);

  static xiiResult ConvertSingleStep(const xiiImageConversionStep* pStep, const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> targetFormat);

  static xiiResult ConvertSingleStepDecompress(const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, const xiiImageConversionStep* pStep);

  static xiiResult ConvertSingleStepCompress(const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, const xiiImageConversionStep* pStep);

  static xiiResult ConvertSingleStepDeplanarize(const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, const xiiImageConversionStep* pStep);

  static xiiResult ConvertSingleStepPlanarize(const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, const xiiImageConversionStep* pStep);

  static void RebuildConversionTable();
};
