#pragma once

/// \brief Operator to serialize xiiIAllocator::Stats objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiAllocatorBase::Stats& rhs);

/// \brief Operator to serialize xiiIAllocator::Stats objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiAllocatorBase::Stats& rhs);

struct xiiTime;

/// \brief Operator to serialize xiiTime objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, xiiTime value);

/// \brief Operator to serialize xiiTime objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiTime& ref_value);


class xiiUuid;

/// \brief Operator to serialize xiiUuid objects. [tested]
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiUuid& value);

/// \brief Operator to serialize xiiUuid objects. [tested]
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiUuid& ref_value);

class xiiHashedString;

/// \brief Operator to serialize xiiHashedString objects. [tested]
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiHashedString& sValue);

/// \brief Operator to serialize xiiHashedString objects. [tested]
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiHashedString& ref_sValue);

class xiiTempHashedString;

/// \brief Operator to serialize xiiHashedString objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiTempHashedString& sValue);

/// \brief Operator to serialize xiiHashedString objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiTempHashedString& ref_sValue);

class xiiVariant;

/// \brief Operator to serialize xiiVariant objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVariant& value);

/// \brief Operator to serialize xiiVariant objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVariant& ref_value);

class xiiTimestamp;

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, xiiTimestamp value);

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiTimestamp& ref_value);

struct xiiVarianceTypeFloat;

/// \brief Operator to serialize xiiVarianceTypeFloat objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeFloat& value);

/// \brief Operator to serialize xiiVarianceTypeFloat objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeFloat& ref_value);

struct xiiVarianceTypeDouble;

/// \brief Operator to serialize xiiVarianceTypeDouble objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeDouble& value);

/// \brief Operator to serialize xiiVarianceTypeDouble objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeDouble& ref_value);

struct xiiVarianceTypeTime;

/// \brief Operator to serialize xiiVarianceTypeTime objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeTime& value);

/// \brief Operator to serialize xiiVarianceTypeTime objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeTime& ref_value);

struct xiiVarianceTypeAngle;

/// \brief Operator to serialize xiiVarianceTypeAngle objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeAngle& value);

/// \brief Operator to serialize xiiVarianceTypeAngle objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeAngle& ref_value);

struct xiiVarianceTypeAngled;

/// \brief Operator to serialize xiiVarianceTypeAngled objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeAngled& value);

/// \brief Operator to serialize xiiVarianceTypeAngled objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeAngled& ref_value);
