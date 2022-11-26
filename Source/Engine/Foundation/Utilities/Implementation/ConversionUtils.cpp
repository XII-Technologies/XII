#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace xiiConversionUtils
{

  static bool IsWhitespace(char c) { return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f' || c == '\a'); }

  static void SkipWhitespace(const char*& szString)
  {
    if (szString == nullptr)
      return;

    while (*szString != '\0' && IsWhitespace(*szString))
    {
      ++szString;
    }
  }

  static xiiResult FindFirstDigit(const char*& inout_szString, bool& out_bSignIsPositive)
  {
    out_bSignIsPositive = true;

    // we are only looking at ASCII characters here, so no need to decode Utf8 sequences

    while (*inout_szString != '\0')
    {
      const char c = *inout_szString;

      // found a digit
      if (c >= '0' && c <= '9')
        break;

      // skip all whitespace
      if (IsWhitespace(c))
      {
        ++inout_szString;
        continue;
      }

      // NO change sign, just ignore + signs
      if (c == '+')
      {
        ++inout_szString;
        continue;
      }

      // change sign
      if (c == '-')
      {
        out_bSignIsPositive = !out_bSignIsPositive;
        ++inout_szString;
        continue;
      }

      return XII_FAILURE;
    }

    // not a single digit found
    if (xiiStringUtils::IsNullOrEmpty(inout_szString))
      return XII_FAILURE;

    // remove all leading zeros
    while (inout_szString[0] == '0' && inout_szString[1] == '0')
      ++inout_szString;

    // if it is a leading zero before a non-zero digit, remove it (otherwise keep the zero)
    if (inout_szString[0] == '0' && inout_szString[1] >= '1' && inout_szString[1] <= '9')
      ++inout_szString;

    return XII_SUCCESS;
  }

  xiiResult StringToInt(const char* szString, xiiInt32& out_Res, const char** out_LastParsePosition)
  {
    xiiInt64 tmp = out_Res;
    if (StringToInt64(szString, tmp, out_LastParsePosition) == XII_SUCCESS && tmp <= (xiiInt32)0x7FFFFFFF && tmp >= (xiiInt32)0x80000000)
    {
      out_Res = (xiiInt32)tmp;
      return XII_SUCCESS;
    }

    return XII_FAILURE;
  }

  xiiResult StringToUInt(const char* szString, xiiUInt32& out_Res, const char** out_LastParsePosition)
  {
    xiiInt64 tmp = out_Res;
    if (StringToInt64(szString, tmp, out_LastParsePosition) == XII_SUCCESS && tmp <= (xiiUInt32)0xFFFFFFFF && tmp >= 0)
    {
      out_Res = (xiiUInt32)tmp;
      return XII_SUCCESS;
    }

    return XII_FAILURE;
  }

  xiiResult StringToInt64(const char* szString, xiiInt64& out_Res, const char** out_LastParsePosition)
  {
    if (xiiStringUtils::IsNullOrEmpty(szString))
      return XII_FAILURE;

    bool bSignIsPos = true;

    if (FindFirstDigit(szString, bSignIsPos) == XII_FAILURE)
      return XII_FAILURE;

    xiiInt64       iCurRes = 0;
    xiiInt64       iSign   = bSignIsPos ? 1 : -1;
    const xiiInt64 iMax    = 0x7FFFFFFFFFFFFFFF;
    const xiiInt64 iMin    = 0x8000000000000000;

    while (*szString != '\0')
    {
      const char c = *szString;

      // end of digits reached -> return success (allows to write something like "239*4" -> parses first part as 239)
      if (c < '0' || c > '9')
        break;

      const xiiInt64 iLastDigit = c - '0';

      if ((iCurRes > iMax / 10) || (iCurRes == iMax / 10 && iLastDigit > 7)) // going to overflow
        return XII_FAILURE;

      if ((iCurRes < iMin / 10) || (iCurRes == iMin / 10 && iLastDigit > 8)) // going to underflow
        return XII_FAILURE;

      iCurRes = iCurRes * 10 + iLastDigit * iSign; // shift all previously read digits to the left and add the last digit

      ++szString;
    }

    out_Res = iCurRes;

    if (out_LastParsePosition != nullptr)
      *out_LastParsePosition = szString;

    return XII_SUCCESS;
  }

  xiiResult StringToFloat(const char* szString, double& out_Res, const char** out_LastParsePosition)
  {
    if (xiiStringUtils::IsNullOrEmpty(szString))
      return XII_FAILURE;

    bool bSignIsPos = true;

    if (FindFirstDigit(szString, bSignIsPos) == XII_FAILURE)
    {
      // if it is a '.' continue (this is valid)
      if (*szString != '.')
        return XII_FAILURE;
    }

    enum NumberPart
    {
      Integer,
      Fraction,
      Exponent,
    };

    NumberPart Part = Integer;

    xiiUInt64 uiIntegerPart       = 0; // with 64 Bit to represent the values a 32 Bit float value can be stored, but a 64 Bit double cannot
    xiiUInt64 uiFractionalPart    = 0; // lets just assume we won't have such large or precise values stored in text form
    xiiUInt64 uiFractionDivisor   = 1;
    xiiUInt64 uiExponentPart      = 0;
    bool      bExponentIsPositive = true;

    while (*szString != '\0')
    {
      const char c = *szString;

      // allow underscores in floats for improved readability
      if (c == '_')
      {
        ++szString;
        continue;
      }

      if (Part == Integer)
      {
        if (c == '.')
        {
          Part = Fraction;
          ++szString;
          continue;
        }

        if (c >= '0' && c <= '9')
        {
          uiIntegerPart *= 10;
          uiIntegerPart += c - '0';
          ++szString;
          continue;
        }

        if ((c == 'e') || (c == 'E'))
        {
          Part = Exponent;
          ++szString;

          if (*szString == '-')
          {
            bExponentIsPositive = false;
            ++szString;
          }
          else if (*szString == '+')
          {
            bExponentIsPositive = true;
            ++szString;
          }

          continue;
        }
      }
      else if (Part == Fraction)
      {
        if (c >= '0' && c <= '9')
        {
          uiFractionalPart *= 10;
          uiFractionalPart += c - '0';
          uiFractionDivisor *= 10;
          ++szString;
          continue;
        }

        if ((c == 'e') || (c == 'E'))
        {
          Part = Exponent;
          ++szString;

          if (*szString == '-')
          {
            bExponentIsPositive = false;
            ++szString;
          }
          else if (*szString == '+')
          {
            bExponentIsPositive = true;
            ++szString;
          }

          continue;
        }
      }
      else if (Part == Exponent)
      {
        if (c >= '0' && c <= '9')
        {
          uiExponentPart *= 10;
          uiExponentPart += c - '0';
          ++szString;
          continue;
        }
      }

      // found something that is not part of a float value -> stop parsing here
      break;
    }

    // we might lose some precision here, but at least up to this point no precision loss was accumulated yet
    out_Res = (double)uiIntegerPart + (double)uiFractionalPart / (double)uiFractionDivisor;

    if (!bSignIsPos)
      out_Res = -out_Res;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString;

    if (Part == Exponent)
    {
      if (bExponentIsPositive)
        out_Res *= xiiMath::Pow(10.0, (double)uiExponentPart);
      else
        out_Res /= xiiMath::Pow(10.0, (double)uiExponentPart);
    }

    return XII_SUCCESS;
  }

  xiiResult StringToBool(const char* szString, bool& out_Res, const char** out_LastParsePosition)
  {
    SkipWhitespace(szString);

    if (xiiStringUtils::IsNullOrEmpty(szString))
      return XII_FAILURE;

    // we are only looking at ASCII characters here, so no need to decode Utf8 sequences

    if (xiiStringUtils::StartsWith(szString, "1"))
    {
      out_Res = true;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 1;

      return XII_SUCCESS;
    }

    if (xiiStringUtils::StartsWith(szString, "0"))
    {
      out_Res = false;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 1;

      return XII_SUCCESS;
    }

    if (xiiStringUtils::StartsWith_NoCase(szString, "true"))
    {
      out_Res = true;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 4;

      return XII_SUCCESS;
    }

    if (xiiStringUtils::StartsWith_NoCase(szString, "false"))
    {
      out_Res = false;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 5;

      return XII_SUCCESS;
    }

    if (xiiStringUtils::StartsWith_NoCase(szString, "on"))
    {
      out_Res = true;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 2;

      return XII_SUCCESS;
    }

    if (xiiStringUtils::StartsWith_NoCase(szString, "off"))
    {
      out_Res = false;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 3;

      return XII_SUCCESS;
    }

    if (xiiStringUtils::StartsWith_NoCase(szString, "yes"))
    {
      out_Res = true;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 3;

      return XII_SUCCESS;
    }

    if (xiiStringUtils::StartsWith_NoCase(szString, "no"))
    {
      out_Res = false;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 2;

      return XII_SUCCESS;
    }

    if (xiiStringUtils::StartsWith_NoCase(szString, "enable"))
    {
      out_Res = true;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 6;

      return XII_SUCCESS;
    }

    if (xiiStringUtils::StartsWith_NoCase(szString, "disable"))
    {
      out_Res = false;

      if (out_LastParsePosition)
        *out_LastParsePosition = szString + 7;

      return XII_SUCCESS;
    }

    return XII_FAILURE;
  }

  xiiUInt32 ExtractFloatsFromString(const char* szText, xiiUInt32 uiNumFloats, float* out_pFloats, const char** out_LastParsePosition)
  {
    xiiUInt32 uiFloatsFound = 0;

    // just try to extract n floats from the given text
    // if n floats were extracted, or the text end is reached, stop

    while (*szText != '\0' && uiFloatsFound < uiNumFloats)
    {
      double      res;
      const char* szPos;

      // if successful, store the float, otherwise advance the string by one, to skip invalid characters
      if (StringToFloat(szText, res, &szPos) == XII_SUCCESS)
      {
        out_pFloats[uiFloatsFound] = (float)res;
        ++uiFloatsFound;

        szText = szPos;
      }
      else
        ++szText;
    }

    if (out_LastParsePosition != nullptr)
      *out_LastParsePosition = szText;

    return uiFloatsFound;
  }

  xiiInt8 HexCharacterToIntValue(xiiUInt32 Character)
  {
    if (Character >= '0' && Character <= '9')
      return static_cast<xiiInt8>(Character - '0');

    if (Character >= 'a' && Character <= 'f')
      return static_cast<xiiInt8>(Character - 'a' + 10);

    if (Character >= 'A' && Character <= 'F')
      return static_cast<xiiInt8>(Character - 'A' + 10);

    return -1;
  }

  xiiUInt32 ConvertHexStringToUInt32(const char* szHEX)
  {
    if (xiiStringUtils::IsNullOrEmpty(szHEX))
      return 0;

    xiiUInt32 uiResult = 0;

    // skip 0x
    if (szHEX[0] == '0' && szHEX[1] == 'x')
      szHEX += 2;

    // convert two characters to one byte, at a time
    // try not to run out of buffer space
    while (*szHEX != '\0')
    {
      xiiUInt8 uiValue = xiiConversionUtils::HexCharacterToIntValue(*szHEX);

      uiResult <<= 4; // 4 Bits, ie. half a byte
      uiResult += uiValue;

      szHEX += 1;
    }

    return uiResult;
  }


  xiiResult ConvertHexStringToUInt64(xiiStringView sHex, xiiUInt64& out_uiResult)
  {
    out_uiResult = 0;

    // skip 0x
    if (sHex.StartsWith_NoCase("0x"))
      sHex.Shrink(2, 0);

    // convert two characters to one byte, at a time
    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      if (sHex.IsEmpty())
      {
        // a shorter/empty string is valid and is just interpreted as a smaller value (e.g. a 32 Bit HEX value)
        break;
      }

      const xiiInt8 iValue = xiiConversionUtils::HexCharacterToIntValue(sHex.GetCharacter());

      if (iValue < 0)
      {
        // invalid HEX character
        out_uiResult = 0;
        return XII_FAILURE;
      }

      out_uiResult <<= 4; // 4 Bits, ie. half a byte
      out_uiResult += iValue;

      sHex.Shrink(1, 0);
    }

    return XII_SUCCESS;
  }

  void ConvertHexToBinary(const char* szHEX, xiiUInt8* pBinary, xiiUInt32 uiBinaryBuffer)
  {
    if (xiiStringUtils::IsNullOrEmpty(szHEX))
      return;

    // skip 0x
    if (szHEX[0] == '0' && (szHEX[1] == 'x' || szHEX[1] == 'X'))
      szHEX += 2;

    // convert two characters to one byte, at a time
    // try not to run out of buffer space
    while (szHEX[0] != '\0' && szHEX[1] != '\0' && uiBinaryBuffer >= 1)
    {
      xiiUInt8 uiValue1 = xiiConversionUtils::HexCharacterToIntValue(szHEX[0]);
      xiiUInt8 uiValue2 = xiiConversionUtils::HexCharacterToIntValue(szHEX[1]);
      xiiUInt8 uiValue  = 16 * uiValue1 + uiValue2;
      *pBinary          = uiValue;

      pBinary += 1;
      szHEX += 2;

      uiBinaryBuffer -= 1;
    }
  }

  const xiiStringBuilder& ToString(xiiInt8 value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", (xiiInt32)value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(xiiUInt8 value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", (xiiUInt32)value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(xiiInt16 value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", (xiiInt32)value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(xiiUInt16 value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", (xiiUInt32)value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(xiiInt32 value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(xiiUInt32 value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(xiiInt64 value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(xiiUInt64 value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(float value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(double value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiColor& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{ r={0}, g={1}, b={2}, a={3} }", value.r, value.g, value.b, value.a);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiColorGammaUB& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{ r={0}, g={1}, b={2}, a={3} }", value.r, value.g, value.b, value.a);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiVec2& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{ x={0}, y={1} }", value.x, value.y);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiVec3& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{ x={0}, y={1}, z={2} }", value.x, value.y, value.z);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiVec4& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{ x={0}, y={1}, z={2}, w={3} }", value.x, value.y, value.z, value.w);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiVec2I32& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{ x={0}, y={1} }", value.x, value.y);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiVec3I32& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{ x={0}, y={1}, z={2} }", value.x, value.y, value.z);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiVec4I32& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{ x={0}, y={1}, z={2}, w={3} }", value.x, value.y, value.z, value.w);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiQuat& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{ x={0}, y={1}, z={2}, w={3} }", value.v.x, value.v.y, value.v.z, value.w);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiMat3& value, xiiStringBuilder& out_Result)
  {
    out_Result.Printf("{ c1r1=%f, c2r1=%f, c3r1=%f, "
                      "c1r2=%f, c2r2=%f, c3r2=%f, "
                      "c1r3=%f, c2r3=%f, c3r3=%f }",
                      value.Element(0, 0), value.Element(1, 0), value.Element(2, 0), value.Element(0, 1), value.Element(1, 1), value.Element(2, 1),
                      value.Element(0, 2), value.Element(1, 2), value.Element(2, 2));
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiMat4& value, xiiStringBuilder& out_Result)
  {
    out_Result.Printf("{ c1r1=%f, c2r1=%f, c3r1=%f, c4r1=%f, "
                      "c1r2=%f, c2r2=%f, c3r2=%f, c4r2=%f, "
                      "c1r3=%f, c2r3=%f, c3r3=%f, c4r3=%f, "
                      "c1r4=%f, c2r4=%f, c3r4=%f, c4r4=%f }",
                      value.Element(0, 0), value.Element(1, 0), value.Element(2, 0), value.Element(3, 0), value.Element(0, 1), value.Element(1, 1),
                      value.Element(2, 1), value.Element(3, 1), value.Element(0, 2), value.Element(1, 2), value.Element(2, 2), value.Element(3, 2),
                      value.Element(0, 3), value.Element(1, 3), value.Element(2, 3), value.Element(3, 3));
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiTransform& value, xiiStringBuilder& out_Result)
  {
    xiiStringBuilder tmp1, tmp2, tmp3;
    out_Result.Format("{ position={0}, rotation={1}, scale={2} }", ToString(value.m_vPosition, tmp1), ToString(value.m_qRotation, tmp2),
                      ToString(value.m_vScale, tmp3));
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiAngle& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiTime& value, xiiStringBuilder& out_Result)
  {
    out_Result.Format("{0}", value);
    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiDynamicArray<xiiVariant>& value, xiiStringBuilder& out_Result)
  {
    out_Result.Append("[");
    for (const xiiVariant& var : value)
    {
      out_Result.Append(var.ConvertTo<xiiString>(), ", ");
    }
    if (!value.IsEmpty())
      out_Result.Shrink(0, 2);
    out_Result.Append("]");
    return out_Result;
  }

  xiiUuid ConvertStringToUuid(const xiiStringView& sText);

  const xiiStringBuilder& ToString(const xiiUuid& value, xiiStringBuilder& out_Result)
  {
    // Windows GUID formatting.
    struct GUID
    {
      xiiUInt32 Data1;
      xiiUInt16 Data2;
      xiiUInt16 Data3;
      xiiUInt8  Data4[8];
    };

    const GUID* pGuid = reinterpret_cast<const GUID*>(&value);

    out_Result.Printf("{ %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x }", pGuid->Data1, pGuid->Data2, pGuid->Data3, pGuid->Data4[0],
                      pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3], pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7]);

    return out_Result;
  }

  const xiiStringBuilder& ToString(const xiiStringView& value, xiiStringBuilder& out_Result)
  {
    out_Result = value;
    return out_Result;
  }

  bool IsStringUuid(const xiiStringView& sText)
  {
    if (sText.IsEmpty())
      return false;

    const char* szText = sText.GetStartPointer();

    if (szText[0] != '{')
      return false;

    if (sText.GetElementCount() != 40)
      return false;

    if ((szText[1] != ' ') || (szText[10] != '-') || (szText[15] != '-') || (szText[20] != '-') || (szText[25] != '-') || (szText[38] != ' ') ||
        (szText[39] != '}'))
      return false;

    return true;
  }

  xiiUuid ConvertStringToUuid(const xiiStringView& sText)
  {
    XII_ASSERT_DEBUG(IsStringUuid(sText), "The given string is not in the correct Uuid format: '{0}'", sText);

    const char* szText = sText.GetStartPointer();

    while (*szText == '{' || xiiStringUtils::IsWhiteSpace(*szText))
      ++szText;

    struct GUID
    {
      xiiUInt32 Data1;
      xiiUInt16 Data2;
      xiiUInt16 Data3;
      xiiUInt8  Data4[8];
    };

    GUID guid;
    guid.Data1 = 0;
    guid.Data2 = 0;
    guid.Data3 = 0;

    for (int i = 0; i < 8; ++i)
    {
      guid.Data4[i] = 0;
      guid.Data1    = (guid.Data1 << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    // -
    ++szText;
    for (int i = 0; i < 4; ++i)
    {
      guid.Data2 = (guid.Data2 << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    // -
    ++szText;
    for (int i = 0; i < 4; ++i)
    {
      guid.Data3 = (guid.Data3 << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    // -
    ++szText;

    for (int i = 0; i < 2; ++i)
    {
      guid.Data4[i] = (guid.Data4[i] << 4) | HexCharacterToIntValue(*szText);
      ++szText;
      guid.Data4[i] = (guid.Data4[i] << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    // -
    ++szText;

    for (int i = 2; i < 8; ++i)
    {
      guid.Data4[i] = (guid.Data4[i] << 4) | HexCharacterToIntValue(*szText);
      ++szText;
      guid.Data4[i] = (guid.Data4[i] << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    xiiUuid result;
    xiiMemoryUtils::Copy<xiiUuid>(&result, reinterpret_cast<xiiUuid*>(&guid), 1);

    return result;
  }

#define Check(name)                                                     \
  if (xiiStringUtils::IsEqual_NoCase(szColorName, XII_STRINGIZE(name))) \
  return xiiColor::name

  xiiColor GetColorByName(const char* szColorName, bool* out_ValidColorName)
  {
    if (out_ValidColorName)
      *out_ValidColorName = false;

    if (xiiStringUtils::IsNullOrEmpty(szColorName))
      return xiiColor::Black; // considered not to be a valid color name

    const xiiUInt32 uiLen = xiiStringUtils::GetStringElementCount(szColorName);

    if (szColorName[0] == '#')
    {
      if (uiLen == 7 || uiLen == 9) // #RRGGBB or #RRGGBBAA
      {
        xiiUInt8 cv[4] = {0, 0, 0, 255};

        cv[0] = static_cast<xiiUInt8>((HexCharacterToIntValue(*(szColorName + 1)) << 4) | HexCharacterToIntValue(*(szColorName + 2)));
        cv[1] = static_cast<xiiUInt8>((HexCharacterToIntValue(*(szColorName + 3)) << 4) | HexCharacterToIntValue(*(szColorName + 4)));
        cv[2] = static_cast<xiiUInt8>((HexCharacterToIntValue(*(szColorName + 5)) << 4) | HexCharacterToIntValue(*(szColorName + 6)));

        if (uiLen == 9)
          cv[3] = static_cast<xiiUInt8>((HexCharacterToIntValue(*(szColorName + 7)) << 4) | HexCharacterToIntValue(*(szColorName + 8)));

        if (out_ValidColorName)
          *out_ValidColorName = true;

        return xiiColorGammaUB(cv[0], cv[1], cv[2], cv[3]);
      }

      // else RebeccaPurple !
    }
    else
    {
      if (out_ValidColorName)
        *out_ValidColorName = true;

      Check(AliceBlue);
      Check(AntiqueWhite);
      Check(Aqua);
      Check(Aquamarine);
      Check(Azure);
      Check(Beige);
      Check(Bisque);
      Check(Black);
      Check(BlanchedAlmond);
      Check(Blue);
      Check(BlueViolet);
      Check(Brown);
      Check(BurlyWood);
      Check(CadetBlue);
      Check(Chartreuse);
      Check(Chocolate);
      Check(Coral);
      Check(CornflowerBlue); // The Original!
      Check(Cornsilk);
      Check(Crimson);
      Check(Cyan);
      Check(DarkBlue);
      Check(DarkCyan);
      Check(DarkGoldenRod);
      Check(DarkGray);
      Check(DarkGrey);
      Check(DarkGreen);
      Check(DarkKhaki);
      Check(DarkMagenta);
      Check(DarkOliveGreen);
      Check(DarkOrange);
      Check(DarkOrchid);
      Check(DarkRed);
      Check(DarkSalmon);
      Check(DarkSeaGreen);
      Check(DarkSlateBlue);
      Check(DarkSlateGray);
      Check(DarkSlateGrey);
      Check(DarkTurquoise);
      Check(DarkViolet);
      Check(DeepPink);
      Check(DeepSkyBlue);
      Check(DimGray);
      Check(DimGrey);
      Check(DodgerBlue);
      Check(FireBrick);
      Check(FloralWhite);
      Check(ForestGreen);
      Check(Fuchsia);
      Check(Gainsboro);
      Check(GhostWhite);
      Check(Gold);
      Check(GoldenRod);
      Check(Gray);
      Check(Grey);
      Check(Green);
      Check(GreenYellow);
      Check(HoneyDew);
      Check(HotPink);
      Check(IndianRed);
      Check(Indigo);
      Check(Ivory);
      Check(Khaki);
      Check(Lavender);
      Check(LavenderBlush);
      Check(LawnGreen);
      Check(LemonChiffon);
      Check(LightBlue);
      Check(LightCoral);
      Check(LightCyan);
      Check(LightGoldenRodYellow);
      Check(LightGray);
      Check(LightGrey);
      Check(LightGreen);
      Check(LightPink);
      Check(LightSalmon);
      Check(LightSeaGreen);
      Check(LightSkyBlue);
      Check(LightSlateGray);
      Check(LightSlateGrey);
      Check(LightSteelBlue);
      Check(LightYellow);
      Check(Lime);
      Check(LimeGreen);
      Check(Linen);
      Check(Magenta);
      Check(Maroon);
      Check(MediumAquaMarine);
      Check(MediumBlue);
      Check(MediumOrchid);
      Check(MediumPurple);
      Check(MediumSeaGreen);
      Check(MediumSlateBlue);
      Check(MediumSpringGreen);
      Check(MediumTurquoise);
      Check(MediumVioletRed);
      Check(MidnightBlue);
      Check(MintCream);
      Check(MistyRose);
      Check(Moccasin);
      Check(NavajoWhite);
      Check(Navy);
      Check(OldLace);
      Check(Olive);
      Check(OliveDrab);
      Check(Orange);
      Check(OrangeRed);
      Check(Orchid);
      Check(PaleGoldenRod);
      Check(PaleGreen);
      Check(PaleTurquoise);
      Check(PaleVioletRed);
      Check(PapayaWhip);
      Check(PeachPuff);
      Check(Peru);
      Check(Pink);
      Check(Plum);
      Check(PowderBlue);
      Check(Purple);
      Check(RebeccaPurple);
      Check(Red);
      Check(RosyBrown);
      Check(RoyalBlue);
      Check(SaddleBrown);
      Check(Salmon);
      Check(SandyBrown);
      Check(SeaGreen);
      Check(SeaShell);
      Check(Sienna);
      Check(Silver);
      Check(SkyBlue);
      Check(SlateBlue);
      Check(SlateGray);
      Check(SlateGrey);
      Check(Snow);
      Check(SpringGreen);
      Check(SteelBlue);
      Check(Tan);
      Check(Teal);
      Check(Thistle);
      Check(Tomato);
      Check(Turquoise);
      Check(Violet);
      Check(Wheat);
      Check(White);
      Check(WhiteSmoke);
      Check(Yellow);
      Check(YellowGreen);
    }

    if (out_ValidColorName)
      *out_ValidColorName = false;

    return xiiColor::RebeccaPurple;
  }

#undef Check

#define Check(name)          \
  if (xiiColor::name == col) \
  return #name

  xiiString GetColorName(const xiiColor& col)
  {
    Check(AliceBlue);
    Check(AntiqueWhite);
    Check(Aqua);
    Check(Aquamarine);
    Check(Azure);
    Check(Beige);
    Check(Bisque);
    Check(Black);
    Check(BlanchedAlmond);
    Check(Blue);
    Check(BlueViolet);
    Check(Brown);
    Check(BurlyWood);
    Check(CadetBlue);
    Check(Chartreuse);
    Check(Chocolate);
    Check(Coral);
    Check(CornflowerBlue); // The Original!
    Check(Cornsilk);
    Check(Crimson);
    Check(Cyan);
    Check(DarkBlue);
    Check(DarkCyan);
    Check(DarkGoldenRod);
    Check(DarkGray);
    Check(DarkGreen);
    Check(DarkKhaki);
    Check(DarkMagenta);
    Check(DarkOliveGreen);
    Check(DarkOrange);
    Check(DarkOrchid);
    Check(DarkRed);
    Check(DarkSalmon);
    Check(DarkSeaGreen);
    Check(DarkSlateBlue);
    Check(DarkSlateGray);
    Check(DarkTurquoise);
    Check(DarkViolet);
    Check(DeepPink);
    Check(DeepSkyBlue);
    Check(DimGray);
    Check(DodgerBlue);
    Check(FireBrick);
    Check(FloralWhite);
    Check(ForestGreen);
    Check(Fuchsia);
    Check(Gainsboro);
    Check(GhostWhite);
    Check(Gold);
    Check(GoldenRod);
    Check(Gray);
    Check(Green);
    Check(GreenYellow);
    Check(HoneyDew);
    Check(HotPink);
    Check(IndianRed);
    Check(Indigo);
    Check(Ivory);
    Check(Khaki);
    Check(Lavender);
    Check(LavenderBlush);
    Check(LawnGreen);
    Check(LemonChiffon);
    Check(LightBlue);
    Check(LightCoral);
    Check(LightCyan);
    Check(LightGoldenRodYellow);
    Check(LightGray);
    Check(LightGreen);
    Check(LightPink);
    Check(LightSalmon);
    Check(LightSeaGreen);
    Check(LightSkyBlue);
    Check(LightSlateGray);
    Check(LightSteelBlue);
    Check(LightYellow);
    Check(Lime);
    Check(LimeGreen);
    Check(Linen);
    Check(Magenta);
    Check(Maroon);
    Check(MediumAquaMarine);
    Check(MediumBlue);
    Check(MediumOrchid);
    Check(MediumPurple);
    Check(MediumSeaGreen);
    Check(MediumSlateBlue);
    Check(MediumSpringGreen);
    Check(MediumTurquoise);
    Check(MediumVioletRed);
    Check(MidnightBlue);
    Check(MintCream);
    Check(MistyRose);
    Check(Moccasin);
    Check(NavajoWhite);
    Check(Navy);
    Check(OldLace);
    Check(Olive);
    Check(OliveDrab);
    Check(Orange);
    Check(OrangeRed);
    Check(Orchid);
    Check(PaleGoldenRod);
    Check(PaleGreen);
    Check(PaleTurquoise);
    Check(PaleVioletRed);
    Check(PapayaWhip);
    Check(PeachPuff);
    Check(Peru);
    Check(Pink);
    Check(Plum);
    Check(PowderBlue);
    Check(Purple);
    Check(RebeccaPurple);
    Check(Red);
    Check(RosyBrown);
    Check(RoyalBlue);
    Check(SaddleBrown);
    Check(Salmon);
    Check(SandyBrown);
    Check(SeaGreen);
    Check(SeaShell);
    Check(Sienna);
    Check(Silver);
    Check(SkyBlue);
    Check(SlateBlue);
    Check(SlateGray);
    Check(Snow);
    Check(SpringGreen);
    Check(SteelBlue);
    Check(Tan);
    Check(Teal);
    Check(Thistle);
    Check(Tomato);
    Check(Turquoise);
    Check(Violet);
    Check(Wheat);
    Check(White);
    Check(WhiteSmoke);
    Check(Yellow);
    Check(YellowGreen);

    xiiColorGammaUB cg = col;

    xiiStringBuilder s;

    if (cg.a == 255)
    {
      s.Format("#{0}{1}{2}", xiiArgU(cg.r, 2, true, 16, true), xiiArgU(cg.g, 2, true, 16, true), xiiArgU(cg.b, 2, true, 16, true));
    }
    else
    {
      s.Format("#{0}{1}{2}{3}", xiiArgU(cg.r, 2, true, 16, true), xiiArgU(cg.g, 2, true, 16, true), xiiArgU(cg.b, 2, true, 16, true),
               xiiArgU(cg.a, 2, true, 16, true));
    }

    return s;
  }

#undef Check

} // namespace xiiConversionUtils


XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_ConversionUtils);
