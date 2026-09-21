/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// Collection of helper methods when working with endianness "problems".
struct XII_FOUNDATION_DLL xiiEndianHelper
{

  /// Returns true if called on a big endian system, false otherwise.
  ///
  /// \note Note that usually the compile time decisions with the defines XII_PLATFORM_LITTLE_ENDIAN, XII_PLATFORM_BIG_ENDIAN is preferred.
  static inline bool IsBigEndian()
  {
    const int i = 1;
    return (*(char*)&i) == 0;
  }

  /// Returns true if called on a little endian system, false otherwise.
  ///
  /// \note Note that usually the compile time decisions with the defines XII_PLATFORM_LITTLE_ENDIAN, XII_PLATFORM_BIG_ENDIAN is preferred.
  static inline bool IsLittleEndian() { return !IsBigEndian(); }

  /// Switches endianness of the given array of words (16 bit values).
  static inline void SwitchWords(xiiUInt16* pWords, xiiUInt32 uiCount) // [tested]
  {
    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      pWords[i] = Switch(pWords[i]);
    }
  }

  /// Switches endianness of the given array of double words (32 bit values).
  static inline void SwitchDWords(xiiUInt32* pDWords, xiiUInt32 uiCount) // [tested]
  {
    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      pDWords[i] = Switch(pDWords[i]);
    }
  }

  /// Switches endianness of the given array of quad words (64 bit values).
  static inline void SwitchQWords(xiiUInt64* pQWords, xiiUInt32 uiCount) // [tested]
  {
    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      pQWords[i] = Switch(pQWords[i]);
    }
  }

  /// Returns a single switched word (16 bit value).
  static XII_ALWAYS_INLINE xiiUInt16 Switch(xiiUInt16 uiWord) // [tested]
  {
    return (((uiWord & 0xFF) << 8) | ((uiWord >> 8) & 0xFF));
  }

  /// Returns a single switched double word (32 bit value).
  static XII_ALWAYS_INLINE xiiUInt32 Switch(xiiUInt32 uiDWord) // [tested]
  {
    return (((uiDWord & 0xFF) << 24) | (((uiDWord >> 8) & 0xFF) << 16) | (((uiDWord >> 16) & 0xFF) << 8) | ((uiDWord >> 24) & 0xFF));
  }

  /// Returns a single switched quad word (64 bit value).
  static XII_ALWAYS_INLINE xiiUInt64 Switch(xiiUInt64 uiQWord) // [tested]
  {
    return (((uiQWord & 0xFF) << 56) | ((uiQWord & 0xFF00) << 40) | ((uiQWord & 0xFF0000) << 24) | ((uiQWord & 0xFF000000) << 8) |
            ((uiQWord & 0xFF00000000) >> 8) | ((uiQWord & 0xFF0000000000) >> 24) | ((uiQWord & 0xFF000000000000) >> 40) |
            ((uiQWord & 0xFF00000000000000) >> 56));
  }

  /// Switches a value in place (template accepts pointers for 2, 4 & 8 byte data types)
  template <typename T>
  static void SwitchInPlace(T* pValue) // [tested]
  {
    static_assert((sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8), "Switch in place only works for type equivalents of xiiUInt16, xiiUInt32, xiiUInt64!");

    if (sizeof(T) == 2)
    {
      struct TAnd16BitUnion
      {
        union
        {
          xiiUInt16 BitValue;
          T         TValue;
        };
      };

      TAnd16BitUnion Temp;
      Temp.TValue   = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
    else if (sizeof(T) == 4)
    {
      struct TAnd32BitUnion
      {
        union
        {
          xiiUInt32 BitValue;
          T         TValue;
        };
      };

      TAnd32BitUnion Temp;
      Temp.TValue   = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
    else if (sizeof(T) == 8)
    {
      struct TAnd64BitUnion
      {
        union
        {
          xiiUInt64 BitValue;
          T         TValue;
        };
      };

      TAnd64BitUnion Temp;
      Temp.TValue   = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
  }

#if XII_ENABLED(XII_PLATFORM_LITTLE_ENDIAN)

  static XII_ALWAYS_INLINE void LittleEndianToNative(xiiUInt16* /*pWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void NativeToLittleEndian(xiiUInt16* /*pWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void LittleEndianToNative(xiiUInt32* /*pDWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void NativeToLittleEndian(xiiUInt32* /*pDWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void LittleEndianToNative(xiiUInt64* /*pQWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void NativeToLittleEndian(xiiUInt64* /*pQWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void BigEndianToNative(xiiUInt16* pWords, xiiUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static XII_ALWAYS_INLINE void NativeToBigEndian(xiiUInt16* pWords, xiiUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static XII_ALWAYS_INLINE void BigEndianToNative(xiiUInt32* pDWords, xiiUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static XII_ALWAYS_INLINE void NativeToBigEndian(xiiUInt32* pDWords, xiiUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static XII_ALWAYS_INLINE void BigEndianToNative(xiiUInt64* pQWords, xiiUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

  static XII_ALWAYS_INLINE void NativeToBigEndian(xiiUInt64* pQWords, xiiUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

#elif XII_ENABLED(XII_PLATFORM_BIG_ENDIAN)

  static XII_ALWAYS_INLINE void LittleEndianToNative(xiiUInt16* pWords, xiiUInt32 uiCount)
  {
    SwitchWords(pWords, uiCount);
  }

  static XII_ALWAYS_INLINE void NativeToLittleEndian(xiiUInt16* pWords, xiiUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static XII_ALWAYS_INLINE void LittleEndianToNative(xiiUInt32* pDWords, xiiUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static XII_ALWAYS_INLINE void NativeToLittleEndian(xiiUInt32* pDWords, xiiUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static XII_ALWAYS_INLINE void LittleEndianToNative(xiiUInt64* pQWords, xiiUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

  static XII_ALWAYS_INLINE void NativeToLittleEndian(xiiUInt64* pQWords, xiiUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

  static XII_ALWAYS_INLINE void BigEndianToNative(xiiUInt16* /*pWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void NativeToBigEndian(xiiUInt16* /*pWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void BigEndianToNative(xiiUInt32* /*pWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void NativeToBigEndian(xiiUInt32* /*pWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void BigEndianToNative(xiiUInt64* /*pWords*/, xiiUInt32 /*uiCount*/) {}

  static XII_ALWAYS_INLINE void NativeToBigEndian(xiiUInt64* /*pWords*/, xiiUInt32 /*uiCount*/) {}

#endif


  /// Switches a given struct according to the layout described in the szFormat parameter
  ///
  /// The format string may contain the characters:
  ///  - c, b for a member of 1 byte
  ///  - w, s for a member of 2 bytes (word, xiiUInt16)
  ///  - d for a member of 4 bytes (DWORD, xiiUInt32)
  ///  - q for a member of 8 bytes (DWORD, xiiUInt64)
  static void SwitchStruct(void* pDataPointer, const char* szFormat);

  /// Templated helper method for SwitchStruct
  template <typename T>
  static void SwitchStruct(T* pDataPointer, const char* szFormat) // [tested]
  {
    SwitchStruct(static_cast<void*>(pDataPointer), szFormat);
  }

  /// Switches a given set of struct according to the layout described in the szFormat parameter
  ///
  /// The format string may contain the characters:
  ///  - c, b for a member of 1 byte
  ///  - w, s for a member of 2 bytes (word, xiiUInt16)
  ///  - d for a member of 4 bytes (DWORD, xiiUInt32)
  ///  - q for a member of 8 bytes (DWORD, xiiUInt64)
  static void SwitchStructs(void* pDataPointer, const char* szFormat, xiiUInt32 uiStride, xiiUInt32 uiCount); // [tested]

  /// Templated helper method for SwitchStructs
  template <typename T>
  static void SwitchStructs(T* pDataPointer, const char* szFormat, xiiUInt32 uiCount) // [tested]
  {
    SwitchStructs(static_cast<void*>(pDataPointer), szFormat, sizeof(T), uiCount);
  }
};
