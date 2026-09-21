/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Containers/Implementation/BitIterator.h>

/// Helper base class to iterate over the bit indices or bit values of an integer.
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnsIndex If set, returns the index of the bit. Otherwise returns the value of the bit, i.e. XII_BIT(value).
/// \tparam ReturnType Returned value type of the iterator.
/// \sa xiiIterateBitValues, xiiIterateBitIndices
template <typename DataType, bool ReturnsIndex, typename ReturnType = DataType>
struct xiiIterateBits
{
  explicit xiiIterateBits(DataType data)
  {
    m_Data = data;
  }

  xiiBitIterator<DataType, ReturnsIndex, ReturnType> begin() const
  {
    return xiiBitIterator<DataType, ReturnsIndex, ReturnType>(m_Data);
  };

  xiiBitIterator<DataType, ReturnsIndex, ReturnType> end() const
  {
    return xiiBitIterator<DataType, ReturnsIndex, ReturnType>();
  };

  DataType m_Data = {};
};

/// Helper class to iterate over the bit values of an integer.
/// The class can iterate over the bits of any unsigned integer type that is equal to or smaller than xiiUInt64.
/// \code{.cpp}
///    xiiUInt64 bits = 0b1101;
///    for (auto bit : xiiIterateBitValues(bits))
///    {
///      xiiLog::Info("{}", bit); // Outputs 1, 4, 8
///    }
/// \endcode
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnType Returned value type of the iterator. Defaults to same as DataType.
template <typename DataType, typename ReturnType = DataType>
struct xiiIterateBitValues : public xiiIterateBits<DataType, false, ReturnType>
{
  explicit xiiIterateBitValues(DataType data) :
    xiiIterateBits<DataType, false, ReturnType>(data)
  {
  }
};

/// Helper class to iterate over the bit indices of an integer.
/// The class can iterate over the bits of any unsigned integer type that is equal to or smaller than xiiUInt64.
/// \code{.cpp}
///    xiiUInt64 bits = 0b1101;
///    for (auto bit : xiiIterateBitIndices(bits))
///    {
///      xiiLog::Info("{}", bit); // Outputs 0, 2, 3
///    }
/// \endcode
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnType Returned value type of the iterator. Defaults to same as DataType.
template <typename DataType, typename ReturnType = DataType>
struct xiiIterateBitIndices : public xiiIterateBits<DataType, true, ReturnType>
{
  explicit xiiIterateBitIndices(DataType data) :
    xiiIterateBits<DataType, true, ReturnType>(data)
  {
  }
};
