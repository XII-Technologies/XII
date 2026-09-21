/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>

class xiiTempHashedString;

/// This class is optimized to take nearly no memory (sizeof(void*)) and to allow very fast checks whether two strings are identical.
///
/// Internally only a reference to the string data is stored. The data itself is stored in a central location, where no duplicates are
/// possible. Thus two identical strings will result in identical xiiHashedString objects, which makes equality comparisons very easy
/// (it's a pointer comparison).\n
/// Copying xiiHashedString objects around and assigning between them is very fast as well.\n
/// \n
/// Assigning from some other string type is rather slow though, as it requires thread synchronization.\n
/// You can also get access to the actual string data via GetString().\n
/// \n
/// You should use xiiHashedString whenever the size of the encapsulating object is important and when changes to the string itself
/// are rare, but checks for equality might be frequent (e.g. in a system where objects are identified via their name).\n
/// At runtime when you need to compare xiiHashedString objects with some temporary string object, used xiiTempHashedString,
/// as it will only use the string's hash value for comparison, but will not store the actual string anywhere.
class XII_FOUNDATION_DLL xiiHashedString
{
public:
  struct HashedData
  {
#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
    xiiAtomicInteger32 m_iRefCount;
#endif
    xiiString m_sString;
  };

  // Do NOT use a hash-table! The map does not relocate memory when it resizes, which is a vital aspect for the hashed strings to work.
  using StringStorage = xiiMap<xiiUInt64, HashedData, xiiCompareHelper<xiiUInt64>, xiiStaticAllocatorWrapper>;
  using HashedType    = StringStorage::Iterator;

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  /// This will remove all hashed strings from the central storage, that are not referenced anymore.
  ///
  /// All hashed string values are stored in a central location and xiiHashedString just references them. Those strings are then
  /// reference counted. Once some string is not referenced anymore, its ref count reaches zero, but it will not be removed from
  /// the storage, as it might be reused later again.
  /// This function will clean up all unused strings. It should typically not be necessary to call this function at all, unless lots of
  /// strings get stored in xiiHashedString that are not really used throughout the applications life time.
  ///
  /// Returns the number of unused strings that were removed.
  static xiiUInt32 ClearUnusedStrings();
#endif

  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Initializes this string to the empty string.
  xiiHashedString(); // [tested]

  /// Copies the given xiiHashedString.
  xiiHashedString(const xiiHashedString& rhs); // [tested]

  /// Moves the given xiiHashedString.
  xiiHashedString(xiiHashedString&& rhs); // [tested]

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  /// Releases the reference to the internal data. Does NOT deallocate any data, even if this held the last reference to some string.
  ~xiiHashedString();
#endif

  /// Copies the given xiiHashedString.
  void operator=(const xiiHashedString& rhs); // [tested]

  /// Moves the given xiiHashedString.
  void operator=(xiiHashedString&& rhs); // [tested]

  /// Assigning a new string from a string constant is a slow operation, but the hash computation can happen at compile time.
  ///
  /// If you need to create an object to compare xiiHashedString objects against, prefer to use xiiTempHashedString. It will only compute
  /// the strings hash value, but does not require any thread synchronization.
  template <size_t N>
  void Assign(const char (&string)[N]); // [tested]

  template <size_t N>
  void Assign(char (&string)[N]) = delete;

  /// Assigning a new string from a non-hashed string is a very slow operation, this should be used rarely.
  ///
  /// If you need to create an object to compare xiiHashedString objects against, prefer to use xiiTempHashedString. It will only compute
  /// the strings hash value, but does not require any thread synchronization.
  void Assign(xiiStringView sString); // [tested]

  /// Comparing whether two hashed strings are identical is just a pointer comparison. This operation is what xiiHashedString is
  /// optimized for.
  ///
  /// \note Comparing between xiiHashedString objects is always error-free, so even if two string had the same hash value, although they are
  /// different, this comparison function will not report they are the same.
  bool operator==(const xiiHashedString& rhs) const; // [tested]

  /// Compares this string object to a xiiTempHashedString object. This should be used whenever some object needs to be found
  /// and the string to compare against is not yet a xiiHashedString object.
  bool operator==(const xiiTempHashedString& rhs) const; // [tested]

  /// This operator allows sorting objects by hash value, not by alphabetical order.
  bool operator<(const xiiHashedString& rhs) const; // [tested]

  /// This operator allows sorting objects by hash value, not by alphabetical order.
  bool operator<(const xiiTempHashedString& rhs) const; // [tested]

  /// Gives access to the actual string data, so you can do all the typical (read-only) string operations on it.
  const xiiString& GetString() const; // [tested]

  /// Gives access to the actual string data, so you can do all the typical (read-only) string operations on it.
  const char* GetData() const;

  /// Returns the hash of the stored string.
  xiiUInt64 GetHash() const; // [tested]

  /// Returns whether the string is empty.
  bool IsEmpty() const;

  /// Resets the string to the empty string.
  void Clear();

  /// Returns a string view to this string's data.
  XII_ALWAYS_INLINE operator xiiStringView() const { return GetString().GetView(); }

  /// Returns a string view to this string's data.
  XII_ALWAYS_INLINE xiiStringView GetView() const { return GetString().GetView(); }

  /// Returns a pointer to the internal Utf8 string.
  XII_ALWAYS_INLINE operator const char*() const { return GetData(); }

  // Since we allow to cast implicitly to const char*, we need these overloads to not do a pure pointer comparison.
  XII_ALWAYS_INLINE bool operator==(const char* szString) const { return GetString().GetView() == xiiStringView(szString); }

private:
  static void       InitHashedString();
  static HashedType AddHashedString(xiiStringView sString, xiiUInt64 uiHash);

  HashedType m_Data;
};

// Since we allow to cast implicitly to const char*, we need these overloads to not do a pure pointer comparison.
XII_ALWAYS_INLINE bool operator==(const char* szString, const xiiHashedString& rhs)
{
  return rhs.GetView() == xiiStringView(szString);
}

/// Helper function to create a xiiHashedString. This can be used to initialize static hashed string variables.
template <size_t N>
xiiHashedString xiiMakeHashedString(const char (&string)[N]);

/// Helper function to create a xiiHashedString. This can be used to initialize static hashed string variables.
xiiHashedString xiiMakeHashedString(xiiStringView sString);


/// A class to use together with xiiHashedString for quick comparisons with temporary strings that need not be stored further.
///
/// Whenever you have objects that use xiiHashedString members and you need to compare against them with some temporary string,
/// prefer to use xiiTempHashedString instead of xiiHashedString, as the latter requires thread synchronization to actually set up the
/// object.
class XII_FOUNDATION_DLL xiiTempHashedString
{
  friend class xiiHashedString;

public:
  xiiTempHashedString(); // [tested]

  /// Creates a xiiTempHashedString object from the given string constant. The hash can be computed at compile time.
  template <size_t N>
  constexpr xiiTempHashedString(const char (&string)[N]); // [tested]

  template <size_t N>
  xiiTempHashedString(char (&string)[N]) = delete;

  /// Creates a xiiTempHashedString object from the given string. Computes the hash of the given string during runtime, which might be slow.
  explicit xiiTempHashedString(xiiStringView sString); // [tested]

  /// Copies the hash from rhs.
  xiiTempHashedString(const xiiTempHashedString& rhs); // [tested]

  /// Copies the hash from the xiiHashedString.
  xiiTempHashedString(const xiiHashedString& rhs); // [tested]

  explicit xiiTempHashedString(xiiUInt32 uiHash) = delete;

  /// Copies the hash from the 64 bit integer.
  explicit xiiTempHashedString(xiiUInt64 uiHash);

  /// The hash of the given string can be computed at compile time.
  template <size_t N>
  void operator=(const char (&string)[N]); // [tested]

  /// Computes and stores the hash of the given string during runtime, which might be slow.
  void operator=(xiiStringView sString); // [tested]

  /// Copies the hash from rhs.
  void operator=(const xiiTempHashedString& rhs); // [tested]

  /// Copies the hash from the xiiHashedString.
  void operator=(const xiiHashedString& rhs); // [tested]

  /// Compares the two objects by their hash value. Might report incorrect equality, if two strings have the same hash value.
  bool operator==(const xiiTempHashedString& rhs) const; // [tested]

  /// This operator allows soring objects by hash value, not by alphabetical order.
  bool operator<(const xiiTempHashedString& rhs) const; // [tested]

  /// Checks whether the xiiTempHashedString represents the empty string.
  bool IsEmpty() const; // [tested]

  /// Resets the string to the empty string.
  void Clear(); // [tested]

  /// Returns the hash of the stored string.
  xiiUInt64 GetHash() const; // [tested]

private:
  xiiUInt64 m_uiHash;
};

// For xiiFormatString
XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiHashedString& sArg);

#include <Foundation/Strings/Implementation/HashedString_inl.h>
