#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONReader.h>


xiiJSONReader::xiiJSONReader()
{
  m_bParsingError = false;
}

xiiResult xiiJSONReader::Parse(xiiStreamReader& InputStream, xiiUInt32 uiFirstLineOffset)
{
  m_bParsingError = false;
  m_Stack.Clear();
  m_sLastName.Clear();

  SetInputStream(InputStream, uiFirstLineOffset);

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
    m_Stack.PushBack(Element());

  return XII_SUCCESS;
}

bool xiiJSONReader::OnVariable(const char* szVarName)
{
  m_sLastName = szVarName;

  return true;
}

void xiiJSONReader::OnReadValue(const char* szValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(xiiVariant(szValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = xiiVariant(szValue);

  m_sLastName.Clear();
}

void xiiJSONReader::OnReadValue(double fValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(xiiVariant(fValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = xiiVariant(fValue);

  m_sLastName.Clear();
}

void xiiJSONReader::OnReadValue(bool bValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(xiiVariant(bValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = xiiVariant(bValue);

  m_sLastName.Clear();
}

void xiiJSONReader::OnReadValueNULL()
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(xiiVariant());
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = xiiVariant();

  m_sLastName.Clear();
}

void xiiJSONReader::OnBeginObject()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode  = ElementMode::Dictionary;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void xiiJSONReader::OnEndObject()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  if (m_Stack.GetCount() > 1)
  {
    Element& Parent = m_Stack[m_Stack.GetCount() - 2];

    if (Parent.m_Mode == ElementMode::Array)
    {
      Parent.m_Array.PushBack(Child.m_Dictionary);
    }
    else
    {
      Parent.m_Dictionary[Child.m_sName] = Child.m_Dictionary;
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
  m_Stack.PeekBack().m_Mode  = ElementMode::Array;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void xiiJSONReader::OnEndArray()
{
  Element& Child  = m_Stack[m_Stack.GetCount() - 1];
  Element& Parent = m_Stack[m_Stack.GetCount() - 2];

  if (Parent.m_Mode == ElementMode::Array)
  {
    Parent.m_Array.PushBack(Child.m_Array);
  }
  else
  {
    Parent.m_Dictionary[Child.m_sName] = Child.m_Array;
  }

  m_Stack.PopBack();
}



void xiiJSONReader::OnParsingError(const char* szMessage, bool bFatal, xiiUInt32 uiLine, xiiUInt32 uiColumn)
{
  m_bParsingError = true;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_JSONReader);
