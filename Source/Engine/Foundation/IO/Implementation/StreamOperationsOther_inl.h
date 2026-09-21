/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// Operator to serialize xiiIAllocator::Stats objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiAllocator::Stats& rhs);

/// Operator to serialize xiiIAllocator::Stats objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiAllocator::Stats& rhs);

struct xiiTime;

/// Operator to serialize xiiTime objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, xiiTime value);

/// Operator to serialize xiiTime objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiTime& ref_value);


class xiiUuid;

/// Operator to serialize xiiUuid objects. [tested]
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiUuid& value);

/// Operator to serialize xiiUuid objects. [tested]
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiUuid& ref_value);

class xiiHashedString;

/// Operator to serialize xiiHashedString objects. [tested]
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiHashedString& sValue);

/// Operator to serialize xiiHashedString objects. [tested]
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiHashedString& ref_sValue);

class xiiTempHashedString;

/// Operator to serialize xiiHashedString objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiTempHashedString& sValue);

/// Operator to serialize xiiHashedString objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiTempHashedString& ref_sValue);

class xiiVariant;

/// Operator to serialize xiiVariant objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVariant& value);

/// Operator to serialize xiiVariant objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVariant& ref_value);

class xiiTimestamp;

/// Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, xiiTimestamp value);

/// Operator to serialize xiiTimestamp objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiTimestamp& ref_value);

struct xiiVarianceTypeFloat;

/// Operator to serialize xiiVarianceTypeFloat objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeFloat& value);

/// Operator to serialize xiiVarianceTypeFloat objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeFloat& ref_value);

struct xiiVarianceTypeDouble;

/// Operator to serialize xiiVarianceTypeDouble objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeDouble& value);

/// Operator to serialize xiiVarianceTypeDouble objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeDouble& ref_value);

struct xiiVarianceTypeTime;

/// Operator to serialize xiiVarianceTypeTime objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeTime& value);

/// Operator to serialize xiiVarianceTypeTime objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeTime& ref_value);

struct xiiVarianceTypeAngle;

/// Operator to serialize xiiVarianceTypeAngle objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeAngle& value);

/// Operator to serialize xiiVarianceTypeAngle objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeAngle& ref_value);

struct xiiVarianceTypeAngled;

/// Operator to serialize xiiVarianceTypeAngled objects.
XII_FOUNDATION_DLL void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeAngled& value);

/// Operator to serialize xiiVarianceTypeAngled objects.
XII_FOUNDATION_DLL void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeAngled& ref_value);
