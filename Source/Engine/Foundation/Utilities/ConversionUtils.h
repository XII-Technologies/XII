/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>

/// This namespace contains functions to convert between different types.
///
/// Contains helper functions to convert from strings to numerical values.
/// To convert from numerical values to strings, use xiiStringBuilder::Format, which provides a rich set of formatting options.
namespace xiiConversionUtils
{
  /// Parses szString and converts it to an integer value. Returns XII_FAILURE if the string contains no parsable integer value.
  ///
  /// \param szString
  ///   If szString is nullptr or an empty string or starts with an some non-whitespace and non-sign character, XII_FAILURE is returned.
  ///   All whitespace at the start of the string are skipped. Each minus sign flips the sign of the output value, so --3 will return 3.
  ///   Plus signs are skipped and have no effect, so -+3 will still return -3.
  /// \param out_Res
  ///   The parsed value is returned in out_Res on success. On failure out_Res is not modified at all, so you can store a default value
  ///   in it, before calling StringToInt() and always use that value, even without checking for success or failure.
  /// \param out_LastParsePosition
  ///   On success out_LastParsePosition will contain the address of the character in szString that stopped the parser. This might point
  ///   to the zero terminator of szString, or to some non-digit character, since for example "5+6" will parse as '5' and '+' will be the
  ///   position where the parser stopped (returning XII_SUCCESS).
  ///   If szString is supposed to only contain one full integer and nothing else, then out_LastParsePosition should always point to a zero
  ///   terminator (otherwise the string was malformed).
  /// \return
  ///   XII_SUCCESS if any integer value could get properly extracted from szString (including 0). This includes that only some part of the
  ///   string was parsed until a non-digit character was encountered.
  ///   XII_FAILURE if the string starts with something that can not be interpreted as an integer.
  XII_FOUNDATION_DLL xiiResult StringToInt(xiiStringView sText, xiiInt32& out_iRes, const char** out_pLastParsePosition = nullptr); // [tested]

  /// Same as StringToInt() but expects the string to be a uint32.
  ///
  /// If the parsed value is a valid int but outside the uint32 value range, the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult StringToUInt(xiiStringView sText, xiiUInt32& out_uiRes, const char** out_pLastParsePosition = nullptr); // [tested]

  /// Same as StringToInt but converts to a 64bit integer value instead.
  XII_FOUNDATION_DLL xiiResult StringToInt64(xiiStringView sText, xiiInt64& out_iRes, const char** out_pLastParsePosition = nullptr); // [tested]

  /// Parses szString and converts it to a double value. Returns XII_FAILURE if the string contains no parseable floating point value.
  ///
  /// \param szString
  ///   If szString is nullptr or an empty string or starts with some non-whitespace and non-sign character, XII_FAILURE is returned.
  ///   All whitespace at the start of the string are skipped. Each minus sign flips the sign of the output value, so --3 will return 3.
  ///   Plus signs are skipped and have no effect, so -+3 will still return -3.
  ///   The value string may contain one '.' to separate integer and fractional part. It may also contain an 'e' for scientific notation
  ///   followed by an integer exponent value. No '.' may follow after an 'e' anymore.
  ///   Additionally the value may be terminated by an 'f' to indicate a floating point value. The 'f' will be skipped (as can be observed
  ///   through out_LastParsePosition, and it will terminate further parsing, but it will not affect the precision of the result.
  ///   Commas (',') are never treated as fractional part separators (as in the German locale).
  /// \param out_Res
  ///   If XII_SUCCESS is returned, out_Res will contain the result. Otherwise it stays unmodified.
  ///   The result may have rounding errors, i.e. even though a double may be able to represent the string value exactly, there is no
  ///   guarantee that out_Res will be 100% identical. If that is required, use the C lib atof() function.
  /// \param out_LastParsePosition
  ///   On success out_LastParsePosition will contain the address of the character in szString that stopped the parser. This might point
  ///   to the zero terminator of szString, or to some unexpected character, since for example "5+6" will parse as '5' and '+' will be the
  ///   position where the parser stopped (returning XII_SUCCESS).
  ///   If you want to parse strictly (e.g. you do not want to parse "5+6" as "5") you can use this result to check that only certain
  ///   characters were encountered after the float (e.g. only '\0' or ',').
  /// \return
  ///   XII_SUCCESS if any text was encountered that can be interpreted as a floating point value.
  ///   XII_FAILURE otherwise.
  ///
  /// \note
  ///   The difference to the C lib atof() function is that this function properly returns whether something could get parsed as a floating
  ///   point value, at all. atof() just returns zero in such a case. Also the way whitespace and signs at the beginning of the string are
  ///   handled is different and StringToFloat will return 'success' if it finds anything that can be parsed as a float, even if the string
  ///   continues with invalid text. So you can parse "2.54f+3.5" as "2.54f" and out_LastParsePosition will tell you where the parser
  ///   stopped. On the down-side StringToFloat() is probably not as precise as atof(), because of a very simplistic conversion algorithm.
  ///   If you require the features of StringToFloat() and the precision of atof(), you can let StringToFloat() handle the cases for
  ///   detecting the validity, the sign and where the value ends and then use atof to parse only that substring with maximum precision.
  XII_FOUNDATION_DLL xiiResult StringToFloat(xiiStringView sText, double& out_fRes, const char** out_pLastParsePosition = nullptr); // [tested]

  /// Parses szString and checks that the first word it finds starts with a phrase that can be interpreted as a boolean value.
  ///
  /// \param szString
  ///   If szString starts with whitespace characters, they are skipped. XII_SUCCESS is returned (and out_Res is filled with true/false),
  ///   if the string then starts with any of the following phrases: "true", "false", "on", "off", "yes", "no", "1", "0", "enable",
  ///   "disable". XII_FAILURE is returned if none of those is encountered (or the string is empty). It does not matter, whether the string
  ///   continues with some other text, e.g. "nolf" is still interpreted as "no". That means you can pass strings such as "true, a = false"
  ///   into this function to just parse the next piece of a command line.
  /// \param out_Res
  ///   If XII_SUCCESS is returned, out_Res contains a valid value. Otherwise it is not modified. That means you can initialize it with a
  ///   default value that can be used even if XII_FAILURE is returned.
  /// \param out_LastParsePosition
  ///   On success out_LastParsePosition will contain the address of the character in szString that stopped the parser. This might point
  ///   to the zero terminator of szString, or to the next character after the phrase "true", "false", "on", "off", etc. which was
  ///   interpreted as a boolean value. If you want to parse strictly (e.g. you do not want to parse "Nolf" as "no") you can use this result
  ///   to check that only certain characters were encountered after the boolean phrase (such as '\0' or ',' etc.).
  /// \return
  ///   XII_SUCCESS if any phrase was encountered that can be interpreted as a boolean value.
  ///   XII_FAILURE otherwise.
  XII_FOUNDATION_DLL xiiResult StringToBool(xiiStringView sText, bool& out_bRes, const char** out_pLastParsePosition = nullptr); // [tested]


  /// Parses \a szText and tries to find up to \a uiNumFloats float values to extract. Skips all characters that cannot be
  /// interpreted as numbers.
  ///
  /// This function can be used to convert string representations of vectors or other more complex numbers. It will parse the string from
  /// front to back and convert anything that looks like a number and add it to the given float array. For example a text like '(1, 2, 3)'
  /// will result in up to three floats. Since any invalid character is skipped, the parenthesis and commas will be ignored (though they act
  /// as delimiters, of course).
  ///
  /// \param szText
  ///   The null terminated string to parse.
  /// \param uiNumFloats
  ///   The maximum number of floats to extract.
  /// \param out_pFloats
  ///   An array of floats that can hold at least uiNumFloats. The results are written to this array.
  /// \param out_LastParsePosition
  ///   The position in szText where the function stopped parsing. It will stop either because the end of the string was reached,
  ///   or uiNumFloats values were successfully extracted.
  /// \return
  ///   The number of successfully extracted values (and thus valid values in out_pFloats).
  XII_FOUNDATION_DLL xiiUInt32 ExtractFloatsFromString(xiiStringView sText, xiiUInt32 uiNumFloats, float* out_pFloats, const char** out_pLastParsePosition = nullptr); // [tested]

  /// Converts a hex character ('0', '1', ... '9', 'A'/'a', ... 'F'/'f') to the corresponding int value 0 - 15.
  ///
  /// \note Returns -1 for invalid HEX characters.
  XII_FOUNDATION_DLL xiiInt8 HexCharacterToIntValue(xiiUInt32 uiCharacter); // [tested]

  /// Same as ConvertHexStringToUInt() with uiMaxHexCharacters set to 8.
  XII_FOUNDATION_DLL xiiResult ConvertHexStringToUInt32(xiiStringView sHex, xiiUInt32& out_uiResult); // [tested]

  /// Same as ConvertHexStringToUInt() with uiMaxHexCharacters set to 16.
  XII_FOUNDATION_DLL xiiResult ConvertHexStringToUInt64(xiiStringView sHex, xiiUInt64& out_uiResult); // [tested]

  /// Converts a hex string (i.e. 0xAABBCCDD) into its uint64 value.
  ///
  /// "0x" at the beginning is ignored.
  /// Empty strings are interpreted as 'valid', representing the value 0 (returns XII_SUCCESS).
  /// If the xiiStringView is shorter than uiMaxHexCharacters, this is interpreted as a valid HEX value with a smaller value.
  /// If the string is longer than uiMaxHexCharacters (after the '0x'), the additional characters are not parsed, at all.
  /// If the first uiMaxHexCharacters (after the '0x') contain any non-HEX characters, parsing is interrupted and XII_FAILURE is returned.
  XII_FOUNDATION_DLL xiiResult ConvertHexStringToUInt(xiiStringView sHex, xiiUInt64& out_uiResult, xiiUInt32 uiMaxHexCharacters, xiiUInt32* pTotalCharactersParsed); // [tested]

  /// Converts a HEX string to a binary value.
  ///
  /// "0x" or "0X" at the start is allowed and will be skipped.
  /// A maximum of \a uiBinaryBuffer bytes is written to \a pBinary.
  /// If the string contains fewer HEX values than fit into \a pBinary, the remaining bytes will not be touched, so make sure all data is
  /// properly initialized! The hex values are read 2 characters at a time to form a single byte value. If at the end a single character is
  /// left (so an odd number of characters in total) that character is ignored entirely! Values are started to be written at \a pBinary and
  /// then the pointer is increased, so the first values in szHEX represent to least significant bytes in \a pBinary.
  ///
  /// \note This function does not validate that the incoming string is actually valid HEX. If an invalid character is used, the result will
  /// be invalid and there is no error reported.
  XII_FOUNDATION_DLL void ConvertHexToBinary(xiiStringView sText, xiiUInt8* pBinary, xiiUInt32 uiBinaryBuffer); // [tested]

  /// Converts a binary stream to a HEX string.
  ///
  /// The result is returned by calling a lambda to append to an output container.
  /// The lambda signature must be:
  /// void Append(const char* twoChars)
  /// The given string will contain exactly two characters and will be zero terminated.
  template <typename APPEND_CONTAINER_LAMBDA>
  inline void ConvertBinaryToHex(const void* pBinaryData, xiiUInt32 uiBytes, APPEND_CONTAINER_LAMBDA append); // [tested]

  /// Converts a string that was written with xiiConversionUtils::ToString(xiiUuid) back to a xiiUuid object.
  XII_FOUNDATION_DLL xiiUuid ConvertStringToUuid(xiiStringView sText); // [tested]

  /// Returns true when the given string is in the exact format "{ 05af8d07-0b38-44a6-8d50-49731ae2625d }"
  /// This includes braces, whitespaces and dashes. This is the format that ToString produces.
  XII_FOUNDATION_DLL bool IsStringUuid(xiiStringView sText); // [tested]

  /// Converts a bool to a string.
  XII_ALWAYS_INLINE const xiiStringBuilder& ToString(bool value, xiiStringBuilder& out_sResult) // [tested]
  {
    out_sResult = value ? "true" : "false";
    return out_sResult;
  }

  /// Converts a 8bit signed integer to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(xiiInt8 value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a 8bit unsigned integer to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(xiiUInt8 value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a 16bit signed integer to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(xiiInt16 value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a 16bit unsigned integer to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(xiiUInt16 value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a 32bit signed integer to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(xiiInt32 value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a 32bit unsigned integer to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(xiiUInt32 value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a 64bit signed integer to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(xiiInt64 value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a 64bit unsigned integer to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(xiiUInt64 value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a float to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(float value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a double to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(double value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a color to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiColor& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a color to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiColorGammaUB& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec2 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec2& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec2d to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec2d& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec3 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec3& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec3d to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec3d& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec4 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec4& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec4d to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec4d& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec2I32 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec2I32& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec2I64 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec2I64& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec3I32 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec3I32& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec3I64 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec3I64& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec4I32 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec4I32& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec4I64 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec4I64& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec2U32 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec2U32& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec2U64 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec2U64& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec3U32 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec3U32& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec3U64 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec3U64& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec4U32 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec4U32& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a vec4U64 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiVec4U64& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a quat to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiQuat& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a quatd to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiQuatd& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a mat3 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiMat3& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a mat3d to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiMat3d& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a mat4 to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiMat4& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a mat4d to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiMat4d& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a transform to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiTransform& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a transformd to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiTransformd& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a Uuid to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiUuid& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts an angle to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiAngle& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts an angle to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiAngled& value, xiiStringBuilder& out_sResult); // [tested]

  /// Converts a time to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiTime& value, xiiStringBuilder& out_sResult);

  /// Converts a xiiStringView to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiStringView& value, xiiStringBuilder& out_sResult);

  /// Converts a hashed string to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiHashedString& value, xiiStringBuilder& out_sResult);

  /// Converts a temp hashed string to a string. Will print the hash value since the original string can't be restored from a temp hashed string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiTempHashedString& value, xiiStringBuilder& out_sResult);

  /// Converts a xiiVariantArray to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiDynamicArray<xiiVariant>& value, xiiStringBuilder& out_sResult);

  /// Converts a xiiVariantDictionary to a string.
  XII_FOUNDATION_DLL const xiiStringBuilder& ToString(const xiiHashTable<xiiString, xiiVariant>& value, xiiStringBuilder& out_sResult);

  /// Fallback ToString implementation for all types that don't have one.
  template <typename T>
  XII_ALWAYS_INLINE const xiiStringBuilder& ToString(const T& value, xiiStringBuilder& out_sResult)
  {
    XII_IGNORE_UNUSED(value);
    out_sResult = "N/A";
    return out_sResult;
  }

  /// Parses a string in the form "#RRGGBBAA" as a (gamma space) color.
  ///
  /// The # at the start is optional.
  /// If fewer characters are given, e.g. only "RRGGBB" or "RRGG" or even just "R" or "RRG", the remaining values are default initialized
  /// with black (alpha = FF).
  XII_FOUNDATION_DLL xiiResult ConvertHexStringToColor(xiiStringView sText, xiiColorGammaUB& ref_color);

  /// Returns the color with the given name.
  ///
  /// Allowed are all predefined color names (case-insensitive), as well as Hex-Values in the form '#RRGGBB' and '#RRGGBBAA'
  /// If out_ValidColorName is a valid pointer, it contains true if the color name was known, otherwise false.
  XII_FOUNDATION_DLL xiiColor GetColorByName(xiiStringView sText, bool* out_pValidColorName = nullptr); // [tested]

  /// The inverse of GetColorByName.
  XII_FOUNDATION_DLL xiiString GetColorName(const xiiColor& col); // [tested]
}; // namespace xiiConversionUtils

template <typename APPEND_CONTAINER_LAMBDA>
inline void xiiConversionUtils::ConvertBinaryToHex(const void* pBinaryData, xiiUInt32 uiBytes, APPEND_CONTAINER_LAMBDA append) // [tested]
{
  char tmp[4];

  xiiUInt8* pBytes = (xiiUInt8*)pBinaryData;

  for (xiiUInt32 i = 0; i < uiBytes; ++i)
  {
    xiiStringUtils::snprintf(tmp, 4, "%02X", (xiiUInt32)*pBytes);
    ++pBytes;

    append(tmp);
  }
}
