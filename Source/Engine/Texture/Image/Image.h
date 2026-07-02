/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/HybridArray.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

#include <Texture/Image/Formats/ImageFileFormat.h>

/// \brief A class referencing image data and holding metadata about the image.
class XII_TEXTURE_DLL xiiImageView
{
public:
  /// \brief Constructs an empty image view.
  xiiImageView();

  /// \brief Constructs an image view with the given description and image data.
  xiiImageView(const xiiGALTextureCreationDescription& description, xiiConstByteBlobPtr imageData);

  /// \brief Constructs an empty image view.
  void Clear();

  /// \brief Returns false if the image view does not reference any data yet.
  bool IsValid() const;

  /// \brief Constructs an image view with the given description and image data.
  void ResetAndViewExternalStorage(const xiiGALTextureCreationDescription& description, xiiConstByteBlobPtr imageData);

  /// \brief Convenience function to save the image to the given file.
  xiiResult SaveTo(xiiStringView sFileName) const;

  /// \brief Returns the description this image was constructed from.
  const xiiGALTextureCreationDescription& GetDescription() const;

  /// \brief Returns the image resource format.
  xiiEnum<xiiGALResourceFormat> GetImageFormat() const;

  /// \brief Returns width/height/depth for the given mip level.
  xiiUInt32 GetWidth(xiiUInt32 uiMipLevel = 0) const;
  xiiUInt32 GetHeight(xiiUInt32 uiMipLevel = 0) const;
  xiiUInt32 GetDepth(xiiUInt32 uiMipLevel = 0) const;

  xiiUInt32 GetMipLevelCount() const;
  xiiUInt32 GetNumFaces() const;
  xiiUInt32 GetNumArrayIndices() const;
  xiiUInt32 GetPlaneCount() const;

  xiiUInt32 GetNumBlocksX(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const;
  xiiUInt32 GetNumBlocksY(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const;
  xiiUInt32 GetNumBlocksZ(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const;

  xiiUInt64 GetRowPitch(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const;
  xiiUInt64 GetDepthPitch(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const;

  xiiUInt64 ComputeDataSize() const;

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
  const T* GetPixelPointer(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiFace = 0, xiiUInt32 uiArrayIndex = 0, xiiUInt32 x = 0, xiiUInt32 y = 0, xiiUInt32 z = 0, xiiUInt32 uiPlaneIndex = 0) const;

  /// \brief Reinterprets the image with a given format; the format must have the same size in bits per pixel as the current one.
  void ReinterpretAs(xiiGALResourceFormat::Enum format);

protected:
  xiiUInt64 ComputeLayout();

  void ValidateSubImageIndices(xiiUInt32 uiMipLevel, xiiUInt32 uiFace, xiiUInt32 uiArrayIndex, xiiUInt32 uiPlaneIndex) const;

  template <typename T>
  void ValidateDataTypeAccessor([[maybe_unused]] xiiUInt32 uiPlaneIndex) const;

  const xiiUInt64& GetSubImageOffset(xiiUInt32 uiMipLevel, xiiUInt32 uiFace, xiiUInt32 uiArrayIndex, xiiUInt32 uiPlaneIndex) const;

  xiiHybridArray<xiiUInt64, 16> m_SubImageOffsets;
  xiiBlobPtr<xiiUInt8>          m_DataPtr;
  xiiGALTextureCreationDescription m_Description;
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

  /// \brief Constructs an image with the given description; allocating internal storage for it.
  explicit xiiImage(const xiiGALTextureCreationDescription& description);

  /// \brief Constructs an image with the given description backed by user-supplied external storage.
  explicit xiiImage(const xiiGALTextureCreationDescription& description, xiiByteBlobPtr externalData);

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
  void ResetAndAlloc(const xiiGALTextureCreationDescription& description);

  /// \brief Constructs an image with the given description and attaches to the user-supplied external storage.
  ///
  /// The user is responsible to keep the external storage alive as long as this xiiImage is alive.
  void ResetAndUseExternalStorage(const xiiGALTextureCreationDescription& description, xiiByteBlobPtr pExternalData);

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
  xiiResult LoadFrom(xiiStringView sFileName);

  /// \brief Convenience function to convert the image to the given format.
  xiiResult Convert(xiiGALResourceFormat::Enum targetFormat);

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
