#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>

#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/ImageHeader.h>

/// \brief A class referencing image data and holding metadata about the image.
class XII_TEXTURE_DLL xiiImageView : protected xiiImageHeader
{
public:
  /// \brief Constructs an empty image view.
  xiiImageView();

  /// \brief Constructs an image view with the given header and image data.
  xiiImageView(const xiiImageHeader& header, xiiConstByteBlobPtr imageData);

  /// \brief Constructs an empty image view.
  void Clear();

  /// \brief Returns false if the image view does not reference any data yet.
  bool IsValid() const;

  /// \brief Constructs an image view with the given header and image data.
  void ResetAndViewExternalStorage(const xiiImageHeader& header, xiiConstByteBlobPtr imageData);

  /// \brief Convenience function to save the image to the given file.
  xiiResult SaveTo(const char* szFileName) const;

  /// \brief Returns the header this image was constructed from.
  const xiiImageHeader& GetHeader() const;

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  xiiBlobPtr<const T> GetBlobPtr() const;

  xiiConstByteBlobPtr GetByteBlobPtr() const;

  /// \brief Returns a view to the given sub-image.
  xiiImageView GetSubImageView(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0) const;

  /// \brief Returns a view to a sub-plane.
  xiiImageView GetPlaneView(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0, xiiUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to z slice of the image.
  xiiImageView GetSliceView(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0, xiiUInt32 z = 0, xiiUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to a row of pixels resp. blocks.
  xiiImageView GetRowView(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0, xiiUInt32 y = 0, xiiUInt32 z = 0, xiiUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  const T* GetPixelPointer(
    xiiUInt32 uiMipLevel   = 0,
    xiiUInt32 uiFace       = 0,
    xiiUInt32 uiArrayIndex = 0,
    xiiUInt32 x            = 0,
    xiiUInt32 y            = 0,
    xiiUInt32 z            = 0,
    xiiUInt32 uiPlaneIndex = 0) const;

  /// \brief Reinterprets the image with a given format; the format must have the same size in bits per pixel as the current one.
  void ReinterpretAs(xiiImageFormat::Enum format);

public:
  using xiiImageHeader::GetDepth;
  using xiiImageHeader::GetHeight;
  using xiiImageHeader::GetWidth;

  using xiiImageHeader::GetNumArrayIndices;
  using xiiImageHeader::GetNumFaces;
  using xiiImageHeader::GetNumMipLevels;
  using xiiImageHeader::GetPlaneCount;

  using xiiImageHeader::GetImageFormat;

  using xiiImageHeader::GetNumBlocksX;
  using xiiImageHeader::GetNumBlocksY;
  using xiiImageHeader::GetNumBlocksZ;

  using xiiImageHeader::GetDepthPitch;
  using xiiImageHeader::GetRowPitch;

protected:
  xiiUInt64 ComputeLayout();

  void ValidateSubImageIndices(xiiUInt32 uiMipLevel, xiiUInt32 uiFace, xiiUInt32 uiArrayIndex, xiiUInt32 uiPlaneIndex) const;
  template <typename T>
  void ValidateDataTypeAccessor(xiiUInt32 uiPlaneIndex) const;

  const xiiUInt64& GetSubImageOffset(xiiUInt32 uiMipLevel, xiiUInt32 uiFace, xiiUInt32 uiArrayIndex, xiiUInt32 uiPlaneIndex) const;

  xiiHybridArray<xiiUInt64, 16> m_SubImageOffsets;
  xiiBlobPtr<xiiUInt8>          m_DataPtr;
};

/// \brief A class containing image data and associated meta data.
///
/// This class is a lightweight container for image data and the description required for interpreting the data,
/// such as the image format, its dimensions, number of sub-images (i.e. cubemap faces, mip levels and array sub-images).
/// However, it does not provide any methods for interpreting or  modifying of the image data.
///
/// The sub-images are stored in a predefined order compatible with the layout of DDS files, that is, it first stores
/// the mip chain for each image, then all faces in a case of a cubemap, then the individual images of an image array.
class XII_TEXTURE_DLL xiiImage : public xiiImageView
{
  /// Use Reset() instead
  void operator=(const xiiImage& rhs) = delete;

  /// Use Reset() instead
  void operator=(const xiiImageView& rhs) = delete;

  /// \brief Constructs an image with the given header; allocating internal storage for it.
  explicit xiiImage(const xiiImageHeader& header);

  /// \brief Constructs an image with the given header backed by user-supplied external storage.
  explicit xiiImage(const xiiImageHeader& header, xiiByteBlobPtr externalData);

  /// \brief Constructor from image view (copies the image data to internal storage)
  explicit xiiImage(const xiiImageView& other);

public:
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Constructs an empty image.
  xiiImage();

  /// \brief Move constructor
  xiiImage(xiiImage&& other);

  void operator=(xiiImage&& rhs);

  /// \brief Constructs an empty image. If the image is attached to an external storage, the attachment is discarded.
  void Clear();

  /// \brief Constructs an image with the given header and ensures sufficient storage is allocated.
  ///
  /// \note If this xiiImage was previously attached to external storage, this will reuse that storage.
  /// However, if the external storage is not sufficiently large, ResetAndAlloc() will detach from it and allocate internal storage.
  void ResetAndAlloc(const xiiImageHeader& header);

  /// \brief Constructs an image with the given header and attaches to the user-supplied external storage.
  ///
  /// The user is responsible to keep the external storage alive as long as this xiiImage is alive.
  void ResetAndUseExternalStorage(const xiiImageHeader& header, xiiByteBlobPtr externalData);

  /// \brief Moves the given data into this object.
  ///
  /// If \a other is attached to an external storage, this object will also be attached to it,
  /// so life-time requirements for the external storage are now bound to this instance.
  void ResetAndMove(xiiImage&& other);

  /// \brief Constructs from an image view. Copies the image data to internal storage.
  ///
  /// If the image is currently attached to external storage, the attachment is discarded.
  void ResetAndCopy(const xiiImageView& other);

  /// \brief Convenience function to load the image from the given file.
  xiiResult LoadFrom(const char* szFileName);

  /// \brief Convenience function to convert the image to the given format.
  xiiResult Convert(xiiImageFormat::Enum targetFormat);

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  xiiBlobPtr<T> GetBlobPtr();

  xiiByteBlobPtr GetByteBlobPtr();

  using xiiImageView::GetBlobPtr;
  using xiiImageView::GetByteBlobPtr;

  /// \brief Returns a view to the given sub-image.
  xiiImage GetSubImageView(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0);

  using xiiImageView::GetSubImageView;

  /// \brief Returns a view to a sub-plane.
  xiiImage GetPlaneView(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0, xiiUInt32 uiPlaneIndex = 0);

  using xiiImageView::GetPlaneView;

  /// \brief Returns a view to z slice of the image.
  xiiImage GetSliceView(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0, xiiUInt32 z = 0, xiiUInt32 uiPlaneIndex = 0);

  using xiiImageView::GetSliceView;

  /// \brief Returns a view to a row of pixels resp. blocks.
  xiiImage GetRowView(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0, xiiUInt32 y = 0, xiiUInt32 z = 0, xiiUInt32 uiPlaneIndex = 0);

  using xiiImageView::GetRowView;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  T* GetPixelPointer(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0, xiiUInt32 x = 0, xiiUInt32 y = 0, xiiUInt32 z = 0, xiiUInt32 uiPlaneIndex = 0);

  using xiiImageView::GetPixelPointer;

private:
  bool UsesExternalStorage() const;

  xiiBlob m_InternalStorage;
};

#include <Texture/Image/Implementation/Image_inl.h>
