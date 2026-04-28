/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>

#include <Texture/Image/Image.h>

XII_DECLARE_FLAGS(xiiUInt8, xiiImageConversionFlags, InPlace);

/// A structure describing the pairs of source/target format that may be converted using the conversion routine.
struct xiiImageConversionEntry
{
  xiiImageConversionEntry(xiiImageFormat::Enum source, xiiImageFormat::Enum target, xiiImageConversionFlags::Enum flags, float fAdditionalPenalty = 0) :
    m_sourceFormat(source), m_targetFormat(target), m_flags(flags), m_fAdditionalPenalty(fAdditionalPenalty)
  {
  }

  const xiiImageFormat::Enum                 m_sourceFormat;
  const xiiImageFormat::Enum                 m_targetFormat;
  const xiiBitflags<xiiImageConversionFlags> m_flags;

  /// This member adds an additional amount to the cost estimate for this conversion step.
  /// It can be used to bias the choice between steps when there are comparable conversion
  /// steps available.
  float m_fAdditionalPenalty = 0.0f;
};

/// \brief Interface for a single image conversion step.
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
  /// \brief Returns an array pointer of supported conversions.
  ///
  /// \note The returned array must have the same entries each time this method is called.
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const = 0;
};

/// \brief Interface for a single image conversion step where both the source and target format are uncompressed.
class XII_TEXTURE_DLL xiiImageConversionStepLinear : public xiiImageConversionStep
{
public:
  /// \brief Converts a batch of pixels.
  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr source, xiiByteBlobPtr target, xiiUInt64 uiNumElements, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is compressed and the target format is uncompressed.
class XII_TEXTURE_DLL xiiImageConversionStepDecompressBlocks : public xiiImageConversionStep
{
public:
  /// \brief Decompresses the given number of blocks.
  virtual xiiResult DecompressBlocks(xiiConstByteBlobPtr source, xiiByteBlobPtr target, xiiUInt32 uiNumBlocks, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is uncompressed and the target format is compressed.
class XII_TEXTURE_DLL xiiImageConversionStepCompressBlocks : public xiiImageConversionStep
{
public:
  /// \brief Compresses the given number of blocks.
  virtual xiiResult CompressBlocks(xiiConstByteBlobPtr source, xiiByteBlobPtr target, xiiUInt32 uiNumBlocksX, xiiUInt32 uiNumBlocksY, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a linear to a planar format.
class XII_TEXTURE_DLL xiiImageConversionStepPlanarize : public xiiImageConversionStep
{
public:
  /// \brief Converts a batch of pixels into the given target planes.
  virtual xiiResult ConvertPixels(const xiiImageView& source, xiiArrayPtr<xiiImage> target, xiiUInt32 uiNumPixelsX, xiiUInt32 uiNumPixelsY, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a planar to a linear format.
class XII_TEXTURE_DLL xiiImageConversionStepDeplanarize : public xiiImageConversionStep
{
public:
  /// \brief Converts a batch of pixels from the given source planes.
  virtual xiiResult ConvertPixels(xiiArrayPtr<xiiImageView> source, xiiImage target, xiiUInt32 uiNumPixelsX, xiiUInt32 uiNumPixelsY, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat) const = 0;
};


/// \brief Helper class containing utilities to convert between different image formats and layouts.
class XII_TEXTURE_DLL xiiImageConversion
{
public:
  /// \brief Checks if there is a known conversion path between the two formats
  static bool IsConvertible(xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat);

  /// \brief Finds the image format from a given list of formats which is the cheapest to convert to.
  static xiiImageFormat::Enum FindClosestCompatibleFormat(xiiImageFormat::Enum format, xiiArrayPtr<const xiiImageFormat::Enum> compatibleFormats);

  /// \brief A single node along a computed conversion path.
  struct ConversionPathNode
  {
    XII_DECLARE_POD_TYPE();

    const xiiImageConversionStep* m_step;
    xiiImageFormat::Enum          m_sourceFormat;
    xiiImageFormat::Enum          m_targetFormat;
    xiiUInt32                     m_sourceBufferIndex;
    xiiUInt32                     m_targetBufferIndex;
    bool                          m_inPlace;
  };

  /// \brief Precomputes an optimal conversion path between two formats and the minimal number of required scratch buffers.
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
  static xiiResult BuildPath(xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat, bool bSourceEqualsTarget, xiiHybridArray<ConversionPathNode, 16>& out_path, xiiUInt32& out_uiNumScratchBuffers);

  /// \brief  Converts the source image into a target image with the given format. Source and target may be the same.
  static xiiResult Convert(const xiiImageView& source, xiiImage& ref_target, xiiImageFormat::Enum targetFormat);

  /// \brief Converts the source image into a target image using a precomputed conversion path.
  static xiiResult Convert(const xiiImageView& source, xiiImage& ref_target, xiiArrayPtr<ConversionPathNode> path, xiiUInt32 uiNumScratchBuffers);

  /// \brief Converts the raw source data into a target data buffer with the given format. Source and target may be the same.
  static xiiResult ConvertRaw(xiiConstByteBlobPtr source, xiiByteBlobPtr target, xiiUInt32 uiNumElements, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat);

  /// \brief Converts the raw source data into a target data buffer using a precomputed conversion path.
  static xiiResult ConvertRaw(xiiConstByteBlobPtr source, xiiByteBlobPtr target, xiiUInt32 uiNumElements, xiiArrayPtr<ConversionPathNode> path, xiiUInt32 uiNumScratchBuffers);

private:
  xiiImageConversion();
  xiiImageConversion(const xiiImageConversion&);

  static xiiResult ConvertSingleStep(const xiiImageConversionStep* pStep, const xiiImageView& source, xiiImage& target, xiiImageFormat::Enum targetFormat);

  static xiiResult ConvertSingleStepDecompress(const xiiImageView& source, xiiImage& target, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat, const xiiImageConversionStep* pStep);

  static xiiResult ConvertSingleStepCompress(const xiiImageView& source, xiiImage& target, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat, const xiiImageConversionStep* pStep);

  static xiiResult ConvertSingleStepDeplanarize(const xiiImageView& source, xiiImage& target, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat, const xiiImageConversionStep* pStep);

  static xiiResult ConvertSingleStepPlanarize(const xiiImageView& source, xiiImage& target, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat, const xiiImageConversionStep* pStep);

  static void RebuildConversionTable();
};
