/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>

/// \brief Defines the SI units of time that can be used with xiiTimestamp.
struct xiiSIUnitOfTime
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Nanosecond = 0U, ///< SI-unit of time (10^-9 second).
    Microsecond,     ///< SI-unit of time (10^-6 second).
    Millisecond,     ///< SI-unit of time (10^-3 second).
    Second,          ///< SI-unit of time (base unit).
  };
};

/// \brief The timestamp class encapsulates a date in time as microseconds since Unix epoch.
///
/// The value is represented by a xiiInt64 and allows storing time stamps from roughly -291030 BC to 293970 AC.
///
/// Use this class to efficiently store a timestamp that is valid across platforms.
class XII_FOUNDATION_DLL xiiTimestamp
{
public:
  struct CompareMode
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      FileTimeEqual = 0U, ///< Uses a resolution that guarantees that a file's timestamp is considered equal on all platforms.
      Identical,          ///< Uses maximal stored resolution.
      Newer,              ///< Just compares values and returns true if the left-hand side is larger than the right hand side.
    };
  };

  /// \brief Returns the current timestamp. Returned value will always be valid.
  ///
  /// Depending on the platform the precision varies between seconds and nanoseconds.
  static const xiiTimestamp CurrentTimestamp(); // [tested]

  XII_DECLARE_POD_TYPE();

  // *** Constructors ***
public:
  /// \brief Creates an invalidated timestamp.
  xiiTimestamp(); // [tested]

  /// \brief Returns an invalid timestamp
  [[nodiscard]] static xiiTimestamp MakeInvalid() { return xiiTimestamp(); }

  /// \brief Returns a timestamp initialized from 'iTimeValue' in 'unitOfTime' since Unix epoch.
  [[nodiscard]] static xiiTimestamp MakeFromInt(xiiInt64 iTimeValue, xiiSIUnitOfTime::Enum unitOfTime);

  // *** Public Functions ***
public:
  /// \brief Returns whether the timestamp is valid.
  bool IsValid() const; // [tested]

  /// \brief Returns the number of 'unitOfTime' since Unix epoch.
  xiiInt64 GetInt64(xiiSIUnitOfTime::Enum unitOfTime) const; // [tested]

  /// \brief Returns whether this timestamp is considered equal to 'rhs' in the given mode.
  ///
  /// Use CompareMode::FileTime when working with file time stamps across platforms.
  /// It will use the lowest resolution supported by all platforms to make sure the
  /// timestamp of a file is considered equal regardless on which platform it was retrieved.
  bool Compare(const xiiTimestamp& rhs, CompareMode::Enum mode) const; // [tested]

  // *** Operators ***
public:
  /// \brief Adds the time value of "timeSpan" to this data value.
  void operator+=(const xiiTime& timeSpan); // [tested]

  /// \brief Subtracts the time value of "timeSpan" from this date value.
  void operator-=(const xiiTime& timeSpan); // [tested]

  /// \brief Returns the time span between this timestamp and "other".
  const xiiTime operator-(const xiiTimestamp& other) const; // [tested]

  /// \brief Returns a timestamp that is "timeSpan" further into the future from this timestamp.
  const xiiTimestamp operator+(const xiiTime& timeSpan) const; // [tested]

  /// \brief Returns a timestamp that is "timeSpan" further into the past from this timestamp.
  const xiiTimestamp operator-(const xiiTime& timeSpan) const; // [tested]


private:
  static constexpr const xiiInt64 XII_INVALID_TIME_STAMP = xiiMath::MinValue<xiiInt64>();

  XII_ALLOW_PRIVATE_PROPERTIES(xiiTimestamp);

  /// \brief The date is stored as microseconds since Unix epoch.
  xiiInt64 m_iTimestamp = XII_INVALID_TIME_STAMP;
};

/// \brief Returns a timestamp that is "timeSpan" further into the future from "timestamp".
const xiiTimestamp operator+(xiiTime& ref_timeSpan, const xiiTimestamp& timestamp);

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTimestamp);

/// \brief The xiiDateTime class can be used to convert xiiTimestamp into a human readable form.
///
/// Note: As xiiTimestamp is microseconds since Unix epoch, the values in this class will always be
/// in UTC.
class XII_FOUNDATION_DLL xiiDateTime
{
public:
  /// \brief Creates an empty date time instance with an invalid date.
  ///
  /// Day, Month and Year will be invalid and must be set.
  xiiDateTime(); // [tested]
  ~xiiDateTime();

  /// \brief Checks whether all values are within valid ranges.
  bool IsValid() const;

  /// \brief Returns a date time that is all zero.
  [[nodiscard]] static xiiDateTime MakeZero() { return xiiDateTime(); }

  /// \brief Sets this instance to the given timestamp.
  ///
  /// This calls SetFromTimestamp() internally and asserts that the conversion succeeded.
  /// Use SetFromTimestamp() directly, if you need to be able to react to invalid data.
  [[nodiscard]] static xiiDateTime MakeFromTimestamp(xiiTimestamp timestamp);

  /// \brief Converts this instance' values into a xiiTimestamp.
  ///
  /// The conversion is done via the OS and can fail for values that are outside the supported range.
  /// In this case, the returned value will be invalid. Anything after 1970 and before the not so distant future should be safe.
  [[nodiscard]] const xiiTimestamp GetTimestamp() const; // [tested]

  /// \brief Sets this instance to the given timestamp.
  ///
  /// The conversion is done via the OS and will fail for invalid dates and values outside the supported range, in which case XII_FAILURE will be returned.
  /// Anything after 1970 and before the not so distant future should be safe.
  xiiResult SetFromTimestamp(xiiTimestamp timestamp);

  // *** Accessors ***
public:
  /// \brief Returns the currently set year.
  xiiUInt32 GetYear() const; // [tested]

  /// \brief Sets the year to the given value.
  void SetYear(xiiInt16 iYear); // [tested]

  /// \brief Returns the currently set month.
  xiiUInt8 GetMonth() const; // [tested]

  /// \brief Sets the month to the given value. Asserts that the value is in the valid range [1, 12].
  void SetMonth(xiiUInt8 uiMonth); // [tested]

  /// \brief Returns the currently set day.
  xiiUInt8 GetDay() const; // [tested]

  /// \brief Sets the day to the given value. Asserts that the value is in the valid range [1, 31].
  void SetDay(xiiUInt8 uiDay); // [tested]

  /// \brief Returns the currently set day of week.
  xiiUInt8 GetDayOfWeek() const;

  /// \brief Sets the day of week to the given value. Asserts that the value is in the valid range [0, 6].
  void SetDayOfWeek(xiiUInt8 uiDayOfWeek);

  /// \brief Returns the currently set hour.
  xiiUInt8 GetHour() const; // [tested]

  /// \brief Sets the hour to the given value. Asserts that the value is in the valid range [0, 23].
  void SetHour(xiiUInt8 uiHour); // [tested]

  /// \brief Returns the currently set minute.
  xiiUInt8 GetMinute() const; // [tested]

  /// \brief Sets the minute to the given value. Asserts that the value is in the valid range [0, 59].
  void SetMinute(xiiUInt8 uiMinute); // [tested]

  /// \brief Returns the currently set second.
  xiiUInt8 GetSecond() const; // [tested]

  /// \brief Sets the second to the given value. Asserts that the value is in the valid range [0, 59].
  void SetSecond(xiiUInt8 uiSecond); // [tested]

  /// \brief Returns the currently set microseconds.
  xiiUInt32 GetMicroseconds() const; // [tested]

  /// \brief Sets the microseconds to the given value. Asserts that the value is in the valid range [0, 999999].
  void SetMicroseconds(xiiUInt32 uiMicroSeconds); // [tested]

private:
  xiiUInt32 m_uiMicroseconds = 0; ///< The fraction of a second in microseconds of this date [0, 999999].
  xiiInt16  m_iYear          = 0; ///< The year of this date [-32k, +32k].
  xiiUInt8  m_uiMonth        = 0; ///< The month of this date [1, 12].
  xiiUInt8  m_uiDay          = 0; ///< The day of this date [1, 31].
  xiiUInt8  m_uiDayOfWeek    = 0; ///< The day of week of this date [0, 6].
  xiiUInt8  m_uiHour         = 0; ///< The hour of this date [0, 23].
  xiiUInt8  m_uiMinute       = 0; ///< The number of minutes of this date [0, 59].
  xiiUInt8  m_uiSecond       = 0; ///< The number of seconds of this date [0, 59].
};

XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiDateTime& arg);

XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiTimestamp& arg);

struct xiiArgDateTime
{
  using StorageType = xiiUInt16;

  enum FormattingFlags : StorageType
  {
    ShowDate         = XII_BIT(0),
    TextualDate      = ShowDate | XII_BIT(1),
    ShowWeekday      = XII_BIT(2),
    ShowTime         = XII_BIT(3),
    ShowSeconds      = ShowTime | XII_BIT(4),
    ShowMilliseconds = ShowSeconds | XII_BIT(5),
    ShowTimeZone     = XII_BIT(6),

    Default        = ShowDate | ShowSeconds,
    DefaultTextual = TextualDate | ShowSeconds,
  };

  /// \brief Initialized a formatting object for a xiiDateTime instance.
  ///
  /// \param dateTime The xiiDateTime instance to format.
  /// \param bUseNames Indicates whether to use names for days of week and months (true) or a purely numerical representation (false).
  /// \param bShowTimeZoneIndicator Whether to indicate the timezone of the xiiDateTime object.
  inline explicit xiiArgDateTime(const xiiDateTime& dateTime, xiiUInt32 uiFormattingFlags = Default) :
    m_Value(dateTime), m_uiFormattingFlags(uiFormattingFlags)
  {
  }

  xiiDateTime m_Value;
  xiiUInt32   m_uiFormattingFlags;
};

XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgDateTime& arg);

#include <Foundation/Time/Implementation/Timestamp_inl.h>
