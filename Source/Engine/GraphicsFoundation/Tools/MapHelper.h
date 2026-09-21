/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \class xiiGALMapHelper
/// Helper class that automates resource mapping and unmapping.
///
/// This class simplifies the process of mapping and unmapping resources within a command list.
/// It automatically maps the specified resource upon construction and unmaps it upon destruction.
///
/// \tparam DataType The type of data that will be mapped.
/// \note This class is designed to streamline manual mapping/unmapping operations.
///
/// Usage example:
/// \code{.cpp}
/// {
///   xiiGALMapHelper<SomeDataType> mapHelper(pCommandList, hBuffer, xiiGALMapType::Read, xiiGALMapFlags::Discard);
///   mapHelper->m_uiVar = 1.0f; // Access mapped data
/// }
/// \endcode
template <typename DataType>
class xiiGALMapHelper
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALMapHelper);

public:
  /// Default constructor.
  xiiGALMapHelper();

  /// Constructs the helper and maps the specified resource.
  ///
  /// \param pCommandList - Pointer to the command list.
  /// \param hBuffer      - Handle to the buffer resource.
  /// \param mapType      - Type of mapping operation.
  /// \param mapFlags     - Mapping flags.
  xiiGALMapHelper(xiiGALCommandList* pCommandList, xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags);

  /// Constructs the helper and maps the specified resource.
  ///
  /// \param commandList - Reference to the command list.
  /// \param hBuffer     - Handle to the buffer resource.
  /// \param mapType     - Type of mapping operation.
  /// \param mapFlags    - Mapping flags.
  xiiGALMapHelper(xiiGALCommandList& commandList, xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags);

  /// Move constructor.
  ///
  /// \param other - The other helper object to move from.
  xiiGALMapHelper(xiiGALMapHelper&& other) noexcept;

  /// Destructor, automatically unmaps the resource.
  ~xiiGALMapHelper();

  /// Move assignment operator.
  ///
  /// \param other - The other helper object to move from.
  ///
  /// \return Reference to this object.
  xiiGALMapHelper<DataType>& operator=(xiiGALMapHelper&& other) noexcept;

  /// Maps the specified resource.
  ///
  /// \param pCommandList - Pointer to the command list.
  /// \param hBuffer      - Handle to the buffer resource.
  /// \param mapType      - Type of mapping operation.
  /// \param mapFlags     - Mapping flags.
  ///
  /// \return Result of the operation.
  xiiResult Map(xiiGALCommandList* pCommandList, xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags);

  /// Unmaps the specified resource.
  ///
  /// \return Result of the operation.
  xiiResult Unmap();

  /// Implicit conversion to DataType pointer.
  operator DataType*();

  /// Implicit conversion to const DataType pointer.
  operator const DataType*() const;

  /// Provides direct access to the mapped data.
  ///
  /// \return Pointer to the mapped data.
  DataType* operator->();

  /// Provides direct access to the mapped data (const version).
  ///
  /// \return Pointer to the mapped data.
  const DataType* operator->() const;

  /// Returns a pointer to the mapped buffer data.
  ///
  /// This function provides direct access to the memory where the buffer is mapped.
  /// It is typically used for reading from or writing to GPU buffer memory directly, assuming the buffer has been properly mapped beforehand.
  ///
  /// \return A raw pointer to the mapped data buffer.
  XII_ALWAYS_INLINE DataType* GetMappedData() const;

private:
  xiiGALCommandList*          m_pCommandList; ///< Pointer to the command list.
  xiiGALBuffer*               m_pBuffer;      ///< Handle to the buffer resource.
  DataType*                   m_pMappedData;  ///< Pointer to the mapped data.
  xiiEnum<xiiGALMapType>      m_MapType;      ///< Type of mapping operation.
  xiiBitflags<xiiGALMapFlags> m_MapFlags;     ///< Mapping flags.
};

#include <GraphicsFoundation/Tools/Implementation/MapHelper_inl.h>
