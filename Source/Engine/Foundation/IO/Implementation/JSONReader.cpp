/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONReader.h>

xiiJSONReader::xiiJSONReader()
{
  m_bParsingError = false;
}

xiiResult xiiJSONReader::Parse(xiiStreamReader& ref_inputStream, xiiUInt32 uiFirstLineOffset)
{
  m_bParsingError = false;
  m_Stack.Clear();
  m_sLastName.Clear();

  SetInputStream(ref_inputStream, uiFirstLineOffset);

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
  m_sLastName = sVarName;

  return true;
}

void xiiJSONReader::OnReadValue(xiiStringView sValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(std::move(xiiString(sValue)));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = std::move(xiiString(sValue));

  m_sLastName.Clear();
}

void xiiJSONReader::OnReadValue(double fValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(xiiVariant(fValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = xiiVariant(fValue);

  m_sLastName.Clear();
}

void xiiJSONReader::OnReadValue(bool bValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(xiiVariant(bValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = xiiVariant(bValue);

  m_sLastName.Clear();
}

void xiiJSONReader::OnReadValueNULL()
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(xiiVariant());
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = xiiVariant();

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

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_JSONReader);
