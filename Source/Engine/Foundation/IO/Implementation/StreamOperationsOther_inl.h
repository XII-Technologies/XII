#pragma once

/// \brief Operator to serialize xiiIAllocator::Stats objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, const xiiAllocatorBase::Stats& rhs);

/// \brief Operator to serialize xiiIAllocator::Stats objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiAllocatorBase::Stats& rhs);

struct xiiTime;

/// \brief Operator to serialize xiiTime objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, xiiTime Value);

/// \brief Operator to serialize xiiTime objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiTime& Value);


class xiiUuid;

/// \brief Operator to serialize xiiUuid objects. [tested]
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, const xiiUuid& Value);

/// \brief Operator to serialize xiiUuid objects. [tested]
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiUuid& Value);

class xiiHashedString;

/// \brief Operator to serialize xiiHashedString objects. [tested]
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, const xiiHashedString& Value);

/// \brief Operator to serialize xiiHashedString objects. [tested]
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiHashedString& Value);

class xiiTempHashedString;

/// \brief Operator to serialize xiiHashedString objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, const xiiTempHashedString& Value);

/// \brief Operator to serialize xiiHashedString objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiTempHashedString& Value);

class xiiVariant;

/// \brief Operator to serialize xiiVariant objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, const xiiVariant& Value);

/// \brief Operator to serialize xiiVariant objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiVariant& Value);

class xiiTimestamp;

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, xiiTimestamp Value);

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiTimestamp& Value);

struct xiiVarianceTypeFloat;

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, const xiiVarianceTypeFloat& Value);

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiVarianceTypeFloat& Value);

struct xiiVarianceTypeTime;

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, const xiiVarianceTypeTime& Value);

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiVarianceTypeTime& Value);

struct xiiVarianceTypeAngle;

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& Stream, const xiiVarianceTypeAngle& Value);

/// \brief Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& Stream, xiiVarianceTypeAngle& Value);
