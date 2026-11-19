/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONReader.h>
#include <Foundation/Math/Math.h>

xiiJSONReader::xiiJSONReader()
{
  m_bParsingError = false;
}

xiiResult xiiJSONReader::Parse(xiiStreamReader& ref_inputStream, xiiUInt32 uiFirstLineOffset)
{
  m_bStrict = false;
  SetMaximumStrictDepth(0);
  return ParseDocument(ref_inputStream, uiFirstLineOffset);
}

xiiResult xiiJSONReader::ParseStrict(xiiStreamReader& ref_input, xiiUInt32 uiMaximumDepth)
{
  m_bStrict = true;
  SetMaximumStrictDepth(xiiMath::Max(uiMaximumDepth, 1u));
  return ParseDocument(ref_input, 0);
}

xiiResult xiiJSONReader::ParseDocument(xiiStreamReader& ref_input, xiiUInt32 uiFirstLineOffset)
{
  m_bParsingError = false;
  m_Stack.Clear();
  m_sLastName.Clear();
  m_Scalar = xiiVariant();

  SetInputStream(ref_input, uiFirstLineOffset);

  while (!m_bParsingError && ContinueParsing())
  {
  }

  if (m_bParsingError)
  {
    m_Stack.Clear();
    m_Stack.PushBack(Element());

    return XII_FAILURE;
  }

  // make sure there is one top level element
  if (m_Stack.IsEmpty())
  {
    Element& e = m_Stack.ExpandAndGetRef();
    e.m_Mode   = ElementType::None;
  }

  return XII_SUCCESS;
}

bool xiiJSONReader::OnVariable(xiiStringView sVarName)
{
  if (m_bStrict && m_Stack.PeekBack().m_Dictionary.Contains(sVarName))
  {
    m_bParsingError = true;
  }

  m_sLastName = sVarName;

  return true;
}

void xiiJSONReader::OnReadValue(xiiStringView sValue)
{
  AddValue(xiiString(sValue));
}

void xiiJSONReader::OnReadValue(double fValue)
{
  AddValue(fValue);
}

void xiiJSONReader::OnReadInteger(xiiInt64 iValue)
{
  AddValue(iValue);
}

void xiiJSONReader::OnReadUnsignedInteger(xiiUInt64 uiValue)
{
  AddValue(uiValue);
}

void xiiJSONReader::OnReadValue(bool bValue)
{
  AddValue(bValue);
}

void xiiJSONReader::OnReadValueNULL()
{
  AddValue(xiiVariant());
}

void xiiJSONReader::AddValue(xiiVariant value)
{
  if (m_Stack.IsEmpty())
  {
    m_Stack.ExpandAndGetRef().m_Mode = ElementType::Scalar;
    m_Scalar                         = std::move(value);
  }
  else if (m_Stack.PeekBack().m_Mode == ElementType::Array)
  {
    m_Stack.PeekBack().m_Array.PushBack(std::move(value));
  }
  else
  {
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = std::move(value);
  }

  m_sLastName.Clear();
}

void xiiJSONReader::OnBeginObject()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode  = ElementType::Dictionary;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void xiiJSONReader::OnEndObject()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  if (m_Stack.GetCount() > 1)
  {
    Element& Parent = m_Stack[m_Stack.GetCount() - 2];

    if (Parent.m_Mode == ElementType::Array)
    {
      Parent.m_Array.PushBack(Child.m_Dictionary);
    }
    else
    {
      Parent.m_Dictionary[Child.m_sName] = std::move(Child.m_Dictionary);
    }

    m_Stack.PopBack();
  }
  else
  {
    // do nothing, keep the top-level dictionary
  }
}

void xiiJSONReader::OnBeginArray()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode  = ElementType::Array;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void xiiJSONReader::OnEndArray()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  if (m_Stack.GetCount() > 1)
  {
    Element& Parent = m_Stack[m_Stack.GetCount() - 2];

    if (Parent.m_Mode == ElementType::Array)
    {
      Parent.m_Array.PushBack(Child.m_Array);
    }
    else
    {
      Parent.m_Dictionary[Child.m_sName] = std::move(Child.m_Array);
    }

    m_Stack.PopBack();
  }
  else
  {
    // do nothing, keep the top-level array
  }
}

void xiiJSONReader::OnParsingError(xiiStringView sMessage, bool bFatal, xiiUInt32 uiLine, xiiUInt32 uiColumn)
{
  XII_IGNORE_UNUSED(sMessage);
  XII_IGNORE_UNUSED(bFatal);
  XII_IGNORE_UNUSED(uiLine);
  XII_IGNORE_UNUSED(uiColumn);

  m_bParsingError = true;
}
