#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Strings/String.h>

template <typename STRING>
void TestConstruction(const STRING& value, const char* szStart, const char* szEnd)
{
  xiiStringUtf8 sUtf8(L"A単語F");
  XII_TEST_BOOL(value.IsEqual(sUtf8.GetData()));
  const bool bEqualForwardItTypes = xiiConversionTest<typename STRING::iterator, typename STRING::const_iterator>::sameType == 1;
  static_assert(bEqualForwardItTypes, "As the string iterator is read-only, both const and non-const versions should be the same type.");
  const bool bEqualReverseItTypes = xiiConversionTest<typename STRING::reverse_iterator, typename STRING::const_reverse_iterator>::sameType == 1;
  static_assert(bEqualReverseItTypes, "As the reverse string iterator is read-only, both const and non-const versions should be the same type.");

  typename STRING::iterator itInvalid;
  XII_TEST_BOOL(!itInvalid.IsValid());
  typename STRING::reverse_iterator itInvalidR;
  XII_TEST_BOOL(!itInvalidR.IsValid());

  // Begin
  const typename STRING::iterator itBegin = begin(value);
  XII_TEST_BOOL(itBegin == value.GetIteratorFront());
  XII_TEST_BOOL(itBegin.IsValid());
  XII_TEST_BOOL(itBegin == itBegin);
  XII_TEST_BOOL(itBegin.GetData() == szStart);
  XII_TEST_BOOL(itBegin.GetCharacter() == xiiUnicodeUtils::ConvertUtf8ToUtf32("A"));
  XII_TEST_BOOL(*itBegin == xiiUnicodeUtils::ConvertUtf8ToUtf32("A"));

  // End
  const typename STRING::iterator itEnd = end(value);
  XII_TEST_BOOL(!itEnd.IsValid());
  XII_TEST_BOOL(itEnd == itEnd);
  XII_TEST_BOOL(itBegin != itEnd);
  XII_TEST_BOOL(itEnd.GetData() == szEnd);
  XII_TEST_BOOL(itEnd.GetCharacter() == 0);
  XII_TEST_BOOL(*itEnd == 0);

  // RBegin
  const typename STRING::reverse_iterator itBeginR = rbegin(value);
  XII_TEST_BOOL(itBeginR == value.GetIteratorBack());
  XII_TEST_BOOL(itBeginR.IsValid());
  XII_TEST_BOOL(itBeginR == itBeginR);
  const char* szEndPrior = szEnd;
  xiiUnicodeUtils::MoveToPriorUtf8(szEndPrior, szStart).AssertSuccess();
  XII_TEST_BOOL(itBeginR.GetData() == szEndPrior);
  XII_TEST_BOOL(itBeginR.GetCharacter() == xiiUnicodeUtils::ConvertUtf8ToUtf32("F"));
  XII_TEST_BOOL(*itBeginR == xiiUnicodeUtils::ConvertUtf8ToUtf32("F"));

  // REnd
  const typename STRING::reverse_iterator itEndR = rend(value);
  XII_TEST_BOOL(!itEndR.IsValid());
  XII_TEST_BOOL(itEndR == itEndR);
  XII_TEST_BOOL(itBeginR != itEndR);
  XII_TEST_BOOL(itEndR.GetData() == nullptr); // Position before first character is not a valid ptr, so it is set to nullptr.
  XII_TEST_BOOL(itEndR.GetCharacter() == 0);
  XII_TEST_BOOL(*itEndR == 0);
}

template <typename STRING, typename IT>
void TestIteratorBegin(const STRING& value, const IT& it)
{
  // It is safe to try to move beyond the iterator's range.
  IT itBegin = it;
  --itBegin;
  itBegin -= 4;
  XII_TEST_BOOL(itBegin == it);
  XII_TEST_BOOL(itBegin - 2 == it);

  // Prefix / Postfix
  XII_TEST_BOOL(itBegin + 2 != it);
  XII_TEST_BOOL(itBegin++ == it);
  XII_TEST_BOOL(itBegin-- != it);
  itBegin = it;
  XII_TEST_BOOL(++itBegin != it);
  XII_TEST_BOOL(--itBegin == it);

  // Misc
  itBegin = it;
  XII_TEST_BOOL(it + 2 == ++(++itBegin));
  itBegin -= 1;
  XII_TEST_BOOL(itBegin == it + 1);
  itBegin -= 0;
  XII_TEST_BOOL(itBegin == it + 1);
  itBegin += 0;
  XII_TEST_BOOL(itBegin == it + 1);
  itBegin += -1;
  XII_TEST_BOOL(itBegin == it);
}

template <typename STRING, typename IT>
void TestIteratorEnd(const STRING& value, const IT& it)
{
  // It is safe to try to move beyond the iterator's range.
  IT itEnd = it;
  ++itEnd;
  itEnd += 4;
  XII_TEST_BOOL(itEnd == it);
  XII_TEST_BOOL(itEnd + 2 == it);

  // Prefix / Postfix
  XII_TEST_BOOL(itEnd - 2 != it);
  XII_TEST_BOOL(itEnd-- == it);
  XII_TEST_BOOL(itEnd++ != it);
  itEnd = it;
  XII_TEST_BOOL(--itEnd != it);
  XII_TEST_BOOL(++itEnd == it);

  // Misc
  itEnd = it;
  XII_TEST_BOOL(it - 2 == --(--itEnd));
  itEnd += 1;
  XII_TEST_BOOL(itEnd == it - 1);
  itEnd += 0;
  XII_TEST_BOOL(itEnd == it - 1);
  itEnd -= 0;
  XII_TEST_BOOL(itEnd == it - 1);
  itEnd -= -1;
  XII_TEST_BOOL(itEnd == it);
}

template <typename STRING>
void TestOperators(const STRING& value, const char* szStart, const char* szEnd)
{
  xiiStringUtf8 sUtf8(L"A単語F");
  XII_TEST_BOOL(value.IsEqual(sUtf8.GetData()));

  // Begin
  typename STRING::iterator itBegin = begin(value);
  TestIteratorBegin(value, itBegin);

  // End
  typename STRING::iterator itEnd = end(value);
  TestIteratorEnd(value, itEnd);

  // RBegin
  typename STRING::reverse_iterator itBeginR = rbegin(value);
  TestIteratorBegin(value, itBeginR);

  // REnd
  typename STRING::reverse_iterator itEndR = rend(value);
  TestIteratorEnd(value, itEndR);
}

template <typename STRING>
void TestLoops(const STRING& value, const char* szStart, const char* szEnd)
{
  xiiStringUtf8 sUtf8(L"A単語F");
  xiiUInt32     characters[] = {xiiUnicodeUtils::ConvertUtf8ToUtf32(xiiStringUtf8(L"A").GetData()),
                            xiiUnicodeUtils::ConvertUtf8ToUtf32(xiiStringUtf8(L"単").GetData()), xiiUnicodeUtils::ConvertUtf8ToUtf32(xiiStringUtf8(L"語").GetData()),
                            xiiUnicodeUtils::ConvertUtf8ToUtf32(xiiStringUtf8(L"F").GetData())};

  // Forward
  xiiInt32 iIndex = 0;
  for (xiiUInt32 character : value)
  {
    XII_TEST_INT(characters[iIndex], character);
    ++iIndex;
  }
  XII_TEST_INT(iIndex, 4);

  typename STRING::iterator itBegin = begin(value);
  typename STRING::iterator itEnd   = end(value);
  iIndex                            = 0;
  for (auto it = itBegin; it != itEnd; ++it)
  {
    XII_TEST_BOOL(it.IsValid());
    XII_TEST_INT(characters[iIndex], it.GetCharacter());
    XII_TEST_INT(characters[iIndex], *it);
    XII_TEST_BOOL(it.GetData() >= szStart);
    XII_TEST_BOOL(it.GetData() < szEnd);
    ++iIndex;
  }
  XII_TEST_INT(iIndex, 4);

  // Reverse
  typename STRING::reverse_iterator itBeginR = rbegin(value);
  typename STRING::reverse_iterator itEndR   = rend(value);
  iIndex                                     = 3;
  for (auto it = itBeginR; it != itEndR; ++it)
  {
    XII_TEST_BOOL(it.IsValid());
    XII_TEST_INT(characters[iIndex], it.GetCharacter());
    XII_TEST_INT(characters[iIndex], *it);
    XII_TEST_BOOL(it.GetData() >= szStart);
    XII_TEST_BOOL(it.GetData() < szEnd);
    --iIndex;
  }
  XII_TEST_INT(iIndex, -1);
}

XII_CREATE_SIMPLE_TEST(Strings, StringIterator)
{
  xiiStringUtf8    sUtf8(L"_A単語F_");
  xiiStringBuilder sTestStringBuilder = sUtf8.GetData();
  sTestStringBuilder.Shrink(1, 1);
  xiiString sTextString = sTestStringBuilder.GetData();

  xiiStringView view(sUtf8.GetData());
  view.Shrink(1, 1);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Construction")
  {
    TestConstruction<xiiString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestConstruction<xiiStringBuilder>(sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestConstruction<xiiStringView>(view, view.GetStartPointer(), view.GetEndPointer());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    TestOperators<xiiString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestOperators<xiiStringBuilder>(sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestOperators<xiiStringView>(view, view.GetStartPointer(), view.GetEndPointer());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Loops")
  {
    TestLoops<xiiString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestLoops<xiiStringBuilder>(sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestLoops<xiiStringView>(view, view.GetStartPointer(), view.GetEndPointer());
  }
}
