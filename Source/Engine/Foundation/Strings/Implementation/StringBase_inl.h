/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

template <typename Derived>
XII_ALWAYS_INLINE const char* xiiStringBase<Derived>::InternalGetData() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData();
}

template <typename Derived>
XII_ALWAYS_INLINE const char* xiiStringBase<Derived>::InternalGetDataEnd() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData() + pDerived->GetElementCount();
}

template <typename Derived>
XII_ALWAYS_INLINE xiiUInt32 xiiStringBase<Derived>::InternalGetElementCount() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetElementCount();
}

template <typename Derived>
XII_ALWAYS_INLINE bool xiiStringBase<Derived>::IsEmpty() const
{
  return xiiStringUtils::IsNullOrEmpty(InternalGetData()) || (InternalGetData() == InternalGetDataEnd());
}

template <typename Derived>
bool xiiStringBase<Derived>::StartsWith(xiiStringView sStartsWith) const
{
  return xiiStringUtils::StartsWith(InternalGetData(), sStartsWith.GetStartPointer(), InternalGetDataEnd(), sStartsWith.GetEndPointer());
}

template <typename Derived>
bool xiiStringBase<Derived>::StartsWith_NoCase(xiiStringView sStartsWith) const
{
  return xiiStringUtils::StartsWith_NoCase(InternalGetData(), sStartsWith.GetStartPointer(), InternalGetDataEnd(), sStartsWith.GetEndPointer());
}

template <typename Derived>
bool xiiStringBase<Derived>::EndsWith(xiiStringView sEndsWith) const
{
  return xiiStringUtils::EndsWith(InternalGetData(), sEndsWith.GetStartPointer(), InternalGetDataEnd(), sEndsWith.GetEndPointer());
}

template <typename Derived>
bool xiiStringBase<Derived>::EndsWith_NoCase(xiiStringView sEndsWith) const
{
  return xiiStringUtils::EndsWith_NoCase(InternalGetData(), sEndsWith.GetStartPointer(), InternalGetDataEnd(), sEndsWith.GetEndPointer());
}

template <typename Derived>
const char* xiiStringBase<Derived>::FindSubString(xiiStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
  {
    szStartSearchAt = InternalGetData();
  }

  XII_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindSubString(szStartSearchAt, sStringToFind.GetStartPointer(), InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
const char* xiiStringBase<Derived>::FindSubString_NoCase(xiiStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
  {
    szStartSearchAt = InternalGetData();
  }

  XII_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindSubString_NoCase(szStartSearchAt, sStringToFind.GetStartPointer(), InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* xiiStringBase<Derived>::FindLastSubString(xiiStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
  {
    szStartSearchAt = InternalGetDataEnd();
  }

  XII_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindLastSubString(InternalGetData(), sStringToFind.GetStartPointer(), szStartSearchAt, InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* xiiStringBase<Derived>::FindLastSubString_NoCase(xiiStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
  {
    szStartSearchAt = InternalGetDataEnd();
  }

  XII_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindLastSubString_NoCase(InternalGetData(), sStringToFind.GetStartPointer(), szStartSearchAt, InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* xiiStringBase<Derived>::FindWholeWord(const char* szSearchFor, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
  {
    szStartSearchAt = InternalGetData();
  }

  XII_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, isDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
inline const char* xiiStringBase<Derived>::FindWholeWord_NoCase(const char* szSearchFor, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
  {
    szStartSearchAt = InternalGetData();
  }

  XII_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, isDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
xiiInt32 xiiStringBase<Derived>::Compare(xiiStringView sOther) const
{
  return xiiStringUtils::Compare(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
xiiInt32 xiiStringBase<Derived>::CompareN(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const
{
  return xiiStringUtils::CompareN(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
xiiInt32 xiiStringBase<Derived>::Compare_NoCase(xiiStringView sOther) const
{
  return xiiStringUtils::Compare_NoCase(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
xiiInt32 xiiStringBase<Derived>::CompareN_NoCase(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const
{
  return xiiStringUtils::CompareN_NoCase(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool xiiStringBase<Derived>::IsEqual(xiiStringView sOther) const
{
  return xiiStringUtils::IsEqual(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool xiiStringBase<Derived>::IsEqualN(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const
{
  return xiiStringUtils::IsEqualN(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool xiiStringBase<Derived>::IsEqual_NoCase(xiiStringView sOther) const
{
  return xiiStringUtils::IsEqual_NoCase(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool xiiStringBase<Derived>::IsEqualN_NoCase(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const
{
  return xiiStringUtils::IsEqualN_NoCase(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
const char* xiiStringBase<Derived>::ComputeCharacterPosition(xiiUInt32 uiCharacterIndex) const
{
  const char* pPosition = InternalGetData();
  if (xiiUnicodeUtils::MoveToNextUtf8(pPosition, InternalGetDataEnd(), uiCharacterIndex).Failed())
    return nullptr;

  return pPosition;
}

template <typename Derived>
typename xiiStringBase<Derived>::iterator xiiStringBase<Derived>::GetIteratorFront() const
{
  return begin(*this);
}

template <typename Derived>
typename xiiStringBase<Derived>::reverse_iterator xiiStringBase<Derived>::GetIteratorBack() const
{
  return rbegin(*this);
}

template <typename DerivedLhs, typename DerivedRhs>
XII_ALWAYS_INLINE bool operator==(const xiiStringBase<DerivedLhs>& lhs, const xiiStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.IsEqual(rhs.GetView());
}

template <typename DerivedRhs>
XII_ALWAYS_INLINE bool operator==(const char* lhs, const xiiStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.IsEqual(lhs);
}

template <typename DerivedLhs>
XII_ALWAYS_INLINE bool operator==(const xiiStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.IsEqual(rhs);
}

template <typename DerivedLhs, typename DerivedRhs>
XII_ALWAYS_INLINE std::strong_ordering operator<=>(const xiiStringBase<DerivedLhs>& lhs, const xiiStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.Compare(rhs) <=> 0;
}

template <typename DerivedRhs>
XII_ALWAYS_INLINE std::strong_ordering operator<=>(const char* lhs, const xiiStringBase<DerivedRhs>& rhs) // [tested]
{
  return -rhs.Compare(lhs) <=> 0;
}

template <typename DerivedLhs>
XII_ALWAYS_INLINE std::strong_ordering operator<=>(const xiiStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) <=> 0;
}

template <typename DerivedLhs>
XII_ALWAYS_INLINE xiiStringBase<DerivedLhs>::operator xiiStringView() const
{
  return xiiStringView(InternalGetData(), InternalGetElementCount());
}

template <typename Derived>
XII_ALWAYS_INLINE xiiStringView xiiStringBase<Derived>::GetView() const
{
  return xiiStringView(InternalGetData(), InternalGetElementCount());
}

template <typename Derived>
template <typename Container>
void xiiStringBase<Derived>::Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 /*= nullptr*/, const char* szSeparator3 /*= nullptr*/, const char* szSeparator4 /*= nullptr*/, const char* szSeparator5 /*= nullptr*/, const char* szSeparator6 /*= nullptr*/) const
{
  GetView().Split(bReturnEmptyStrings, ref_output, szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6);
}

template <typename Derived>
xiiStringView xiiStringBase<Derived>::GetRootedPathRootName() const
{
  return GetView().GetRootedPathRootName();
}

template <typename Derived>
bool xiiStringBase<Derived>::IsRootedPath() const
{
  return GetView().IsRootedPath();
}

template <typename Derived>
bool xiiStringBase<Derived>::IsRelativePath() const
{
  return GetView().IsRelativePath();
}

template <typename Derived>
bool xiiStringBase<Derived>::IsAbsolutePath() const
{
  return GetView().IsAbsolutePath();
}

template <typename Derived>
xiiStringView xiiStringBase<Derived>::GetFileDirectory() const
{
  return GetView().GetFileDirectory();
}

template <typename Derived>
xiiStringView xiiStringBase<Derived>::GetFileNameAndExtension() const
{
  return GetView().GetFileNameAndExtension();
}

template <typename Derived>
xiiStringView xiiStringBase<Derived>::GetFileName() const
{
  return GetView().GetFileName();
}

template <typename Derived>
xiiStringView xiiStringBase<Derived>::GetFileExtension(bool bFullExtension) const
{
  return GetView().GetFileExtension(bFullExtension);
}

template <typename Derived>
bool xiiStringBase<Derived>::HasExtension(xiiStringView sExtension) const
{
  return GetView().HasExtension(sExtension);
}

template <typename Derived>
bool xiiStringBase<Derived>::HasAnyExtension() const
{
  return GetView().HasAnyExtension();
}
