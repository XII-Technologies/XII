#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \class xiiGALMapHelper
/// \brief Helper class that automates resource mapping and unmapping.
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
class XII_GRAPHICSFOUNDATION_DLL xiiGALMapHelper : public xiiReflectedClass
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALMapHelper);

public:
  /// \brief Default constructor.
  xiiGALMapHelper();

  /// \brief Constructs the helper and maps the specified resource.
  ///
  /// \param pCommandList - Pointer to the command list.
  /// \param hBuffer      - Handle to the buffer resource.
  /// \param mapType      - Type of mapping operation.
  /// \param mapFlags     - Mapping flags.
  xiiGALMapHelper(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags);

  /// \brief Move constructor.
  ///
  /// \param other - The other helper object to move from.
  xiiGALMapHelper(xiiGALMapHelper&& other) noexcept;

  /// \brief Destructor, automatically unmaps the resource.
  ~xiiGALMapHelper();

  /// \brief Move assignment operator.
  ///
  /// \param other - The other helper object to move from.
  ///
  /// \return Reference to this object.
  xiiGALMapHelper<DataType>& operator=(xiiGALMapHelper&& other) noexcept;

  /// \brief Maps the specified resource.
  ///
  /// \param pCommandList - Pointer to the command list.
  /// \param hBuffer      - Handle to the buffer resource.
  /// \param mapType      - Type of mapping operation.
  /// \param mapFlags     - Mapping flags.
  ///
  /// \return Result of the operation.
  xiiResult Map(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags);

  /// \brief Unmaps the specified resource.
  ///
  /// \param pCommandList - Pointer to the command list.
  /// \param hBuffer      - Handle to the buffer resource.
  ///
  /// \return Result of the operation.
  xiiResult Unmap(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiSharedPtr<xiiGALBuffer> pBuffer);

  /// \brief Implicit conversion to DataType pointer.
  operator DataType*();

  /// \brief Implicit conversion to const DataType pointer.
  operator const DataType*() const;

  /// \brief Provides direct access to the mapped data.
  ///
  /// \return Pointer to the mapped data.
  DataType* operator->();

  /// \brief Provides direct access to the mapped data (const version).
  ///
  /// \return Pointer to the mapped data.
  const DataType* operator->() const;

private:
  xiiSharedPtr<xiiGALCommandList> m_pCommandList; ///< Pointer to the command list.
  xiiSharedPtr<xiiGALBuffer>      m_pBuffer;      ///< Handle to the buffer resource.
  DataType*                       m_pMappedData;  ///< Pointer to the mapped data.
  xiiEnum<xiiGALMapType>          m_MapType;      ///< Type of mapping operation.
  xiiBitflags<xiiGALMapFlags>     m_MapFlags;     ///< Mapping flags.
};

#include <GraphicsFoundation/Tools/Implementation/MapHelper_inl.h>
