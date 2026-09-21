/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageFilter.h>

class XII_TEXTURE_DLL xiiImageUtils
{
public:
  /// Returns the image with the difference (absolute values) between ImageA and ImageB.
  static void ComputeImageDifferenceABS(const xiiImageView& imageA, const xiiImageView& imageB, xiiImage& out_difference);

  /// Same as ComputeImageDifferenceABS, but for every pixel in imageA, the minimum diff in imageB is searched in a 1-pixel radius, allowing pixels in B to shift slightly without incurring a difference.
  static void ComputeImageDifferenceABSRelaxed(const xiiImageView& imageA, const xiiImageView& imageB, xiiImage& out_difference);

  /// Computes the mean square error for the block at (offsetx, offsety) to (offsetx + uiBlockSize, offsety + uiBlockSize).
  /// DifferenceImage is expected to be an image that represents the difference between two images.
  static xiiUInt32 ComputeMeanSquareError(const xiiImageView& differenceImage, xiiUInt8 uiBlockSize, xiiUInt32 uiOffsetx, xiiUInt32 uiOffsety);

  /// Computes the mean square error of DifferenceImage, by computing the MSE for blocks of uiBlockSize and returning the maximum MSE
  /// that was found.
  static xiiUInt32 ComputeMeanSquareError(const xiiImageView& differenceImage, xiiUInt8 uiBlockSize);

  /// Rescales pixel values to use the full value range by scaling from [min, max] to [0, 255].
  /// Computes combined min/max for RGB and separate min/max for alpha.
  static void Normalize(xiiImage& ref_image);
  static void Normalize(xiiImage& ref_image, xiiUInt8& ref_uiMinRgb, xiiUInt8& ref_uiMaxRgb, xiiUInt8& ref_uiMinAlpha, xiiUInt8& ref_uiMaxAlpha);

  /// Extracts the alpha channel from 8bpp 4 channel images into a 8bpp single channel image.
  static void ExtractAlphaChannel(const xiiImageView& inputImage, xiiImage& ref_outputImage);

  /// Returns the sub-image of \a input that starts at \a offset and has the size \a newsize
  static void CropImage(const xiiImageView& input, const xiiVec2I32& vOffset, const xiiSizeU32& newsize, xiiImage& ref_output);

  /// rotates a sub image by 180 degrees in place. Only works with uncompressed images.
  static void RotateSubImage180(xiiImage& ref_image, xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0);

  /// Copies the source image into the destination image at the specified location.
  ///
  /// The image must fit, no scaling or cropping is done. Image formats must be identical. Compressed formats are not supported.
  /// If the target location leaves not enough room for the source image to be copied, bad stuff will happen.
  static xiiResult Copy(const xiiImageView& srcImg, const xiiRectU32& srcRect, xiiImage& ref_dstImg, const xiiVec3U32& vDstOffset, xiiUInt32 uiDstMipLevel = 0, xiiUInt32 uiDstFace = 0, xiiUInt32 uiDstArrayIndex = 0);

  /// Copies the lower uiNumMips data of a 2D image into another one.
  static xiiResult ExtractLowerMipChain(const xiiImageView& src, xiiImage& ref_dst, xiiUInt32 uiNumMips);

  /// Mip map generation options.
  struct MipMapOptions
  {
    const xiiImageFilter*             m_pFilter             = nullptr;                         ///< The filter to use for mipmap generation. Defaults to bilinear filtering (Triangle filter) if none is given.
    bool                              m_bRenormalizeNormals = false;                           ///< If true, the RGB components are scaled to unit length.
    bool                              m_bPreserveCoverage   = false;                           ///< If true, the alpha values are scaled to preserve the average coverage when alpha testing is enabled.
    float                             m_fAlphaThreshold     = 0.5f;                            ///< The alpha test threshold to use when PreserveCoverage == true.
    xiiEnum<xiiGALTextureAddressMode> m_AddressModeU        = xiiGALTextureAddressMode::Clamp; ///< The address mode for samples when filtering outside of the image dimensions in the horizontal direction.
    xiiEnum<xiiGALTextureAddressMode> m_AddressModeV        = xiiGALTextureAddressMode::Clamp; ///< The address mode for samples when filtering outside of the image dimensions in the vertical direction.
    xiiEnum<xiiGALTextureAddressMode> m_AddressModeW        = xiiGALTextureAddressMode::Clamp; ///< The address mode for samples when filtering outside of the image dimensions in the depth direction.
    xiiColor                          m_BorderColor         = xiiColor::Black;                 ///< The border color if texture address mode equals BORDER.
    xiiUInt32                         m_uiMipLevelCount     = 0;                               ///< How many mip maps should be generated. Pass 0 to generate all mip map levels.
  };

  /// Scales the image.
  static xiiResult Scale(const xiiImageView& source, xiiImage& ref_target, xiiUInt32 uiWidth, xiiUInt32 uiHeight, const xiiImageFilter* pFilter = nullptr, xiiGALTextureAddressMode::Enum addressModeU = xiiGALTextureAddressMode::Clamp, xiiGALTextureAddressMode::Enum addressModeV = xiiGALTextureAddressMode::Clamp, const xiiColor& borderColor = xiiColor::Black);

  /// Scales the image.
  static xiiResult Scale3D(const xiiImageView& source, xiiImage& ref_target, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiDepth, const xiiImageFilter* pFilter = nullptr, xiiGALTextureAddressMode::Enum addressModeU = xiiGALTextureAddressMode::Clamp, xiiGALTextureAddressMode::Enum addressModeV = xiiGALTextureAddressMode::Clamp, xiiGALTextureAddressMode::Enum addressModeW = xiiGALTextureAddressMode::Clamp, const xiiColor& borderColor = xiiColor::Black);

  /// Genererates the mip maps for the image. The input texture must be in xiiImageFormat::R32_G32_B32_A32_FLOAT
  static void GenerateMipMaps(const xiiImageView& source, xiiImage& ref_target, const MipMapOptions& options);

  /// Assumes that the Red and Green components of an image contain XY of an unit length normal and reconstructs the Z component into B
  static void ReconstructNormalZ(xiiImage& ref_source);

  /// Renormalizes a normal map to unit length.
  static void RenormalizeNormalMap(xiiImage& ref_image);

  /// Adjust the roughness in lower mip levels so it maintains the same look from all distances.
  static void AdjustRoughness(xiiImage& ref_roughnessMap, const xiiImageView& normalMap);

  /// Changes the exposure of an HDR image by 2^bias
  static void ChangeExposure(xiiImage& ref_image, float fBias);

  /// Creates a cubemap from srcImg and stores it in dstImg.
  ///
  /// If srcImg is already a cubemap, the data will be copied 1:1 to dstImg.
  /// If it is a 2D texture, it is analyzed and sub-images are copied to the proper faces of the output cubemap.
  ///
  /// Supported input layouts are:
  ///  * Vertical Cross
  ///  * Horizontal Cross
  ///  * Spherical mapping
  static xiiResult CreateCubemapFromSingleFile(xiiImage& ref_dstImg, const xiiImageView& srcImg);

  /// Copies the 6 given source images to the faces of dstImg.
  ///
  /// All input images must have the same square, power-of-two dimensions and mustn't be compressed.
  static xiiResult CreateCubemapFrom6Files(xiiImage& ref_dstImg, const xiiImageView* pSourceImages);

  static xiiResult CreateVolumeTextureFromSingleFile(xiiImage& ref_dstImg, const xiiImageView& srcImg);

  static xiiUInt32 GetSampleIndex(xiiUInt32 uiNumTexels, xiiInt32 iIndex, xiiGALTextureAddressMode::Enum addressMode, bool& out_bUseBorderColor);

  /// Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static xiiColor NearestSample(const xiiImageView& image, xiiGALTextureAddressMode::Enum addressMode, xiiVec2 vUv);

  /// Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// Prefer this function over the one that takes a xiiImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as xiiImage::GetPixelPointer() is rather slow due to validation overhead.
  static xiiColor NearestSample(const xiiColor* pPixelPointer, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiGALTextureAddressMode::Enum addressMode, xiiVec2 vUv);

  /// Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static xiiColor BilinearSample(const xiiImageView& image, xiiGALTextureAddressMode::Enum addressMode, xiiVec2 vUv);

  /// Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// Prefer this function over the one that takes a xiiImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as xiiImage::GetPixelPointer() is rather slow due to validation overhead.
  static xiiColor BilinearSample(const xiiColor* pPixelPointer, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiGALTextureAddressMode::Enum addressMode, xiiVec2 vUv);

  /// Copies channel 0, 1, 2 or 3 from srcImg into dstImg.
  ///
  /// Currently only supports images of format R32G32B32A32_FLOAT and with identical resolution.
  /// Returns failure if any of those requirements are not met.
  static xiiResult CopyChannel(xiiImage& ref_dstImg, xiiUInt8 uiDstChannelIdx, const xiiImage& srcImg, xiiUInt8 uiSrcChannelIdx);

  /// Embeds the image as Base64 encoded text into an HTML file.
  static void EmbedImageData(xiiStringBuilder& out_sHtml, const xiiImage& image);

  /// Generates an HTML file containing the given images with mouse-over functionality to compare them.
  static void CreateImageDiffHtml(xiiStringBuilder& out_sHtml, xiiStringView sTitle, const xiiImage& referenceImgRgb, const xiiImage& referenceImgAlpha, const xiiImage& capturedImgRgb, const xiiImage& capturedImgAlpha, const xiiImage& diffImgRgb, const xiiImage& diffImgAlpha, xiiUInt32 uiError, xiiUInt32 uiThreshold, xiiUInt8 uiMinDiffRgb, xiiUInt8 uiMaxDiffRgb, xiiUInt8 uiMinDiffAlpha, xiiUInt8 uiMaxDiffAlpha);
};
