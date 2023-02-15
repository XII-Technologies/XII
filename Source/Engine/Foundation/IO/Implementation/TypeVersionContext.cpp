#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

static const xiiTypeVersion s_uiTypeVersionContextVersion = 1;

XII_IMPLEMENT_SERIALIZATION_CONTEXT(xiiTypeVersionWriteContext)

xiiTypeVersionWriteContext::xiiTypeVersionWriteContext()  = default;
xiiTypeVersionWriteContext::~xiiTypeVersionWriteContext() = default;

xiiStreamWriter& xiiTypeVersionWriteContext::Begin(xiiStreamWriter& originalStream)
{
  m_pOriginalStream = &originalStream;

  XII_ASSERT_DEV(m_TempStreamStorage.GetStorageSize64() == 0, "Begin() can only be called once on a type version context.");
  m_TempStreamWriter.SetStorage(&m_TempStreamStorage);

  return m_TempStreamWriter;
}

xiiResult xiiTypeVersionWriteContext::End()
{
  XII_ASSERT_DEV(m_pOriginalStream != nullptr, "End() called before Begin()");

  WriteTypeVersions(*m_pOriginalStream);

  // Now append the original stream
  XII_SUCCEED_OR_RETURN(m_TempStreamStorage.CopyToStream(*m_pOriginalStream));

  return XII_SUCCESS;
}

void xiiTypeVersionWriteContext::AddType(const xiiRTTI* pRtti)
{
  if (m_KnownTypes.Insert(pRtti) == false)
  {
    if (const xiiRTTI* pParentRtti = pRtti->GetParentType())
    {
      AddType(pParentRtti);
    }
  }
}

void xiiTypeVersionWriteContext::WriteTypeVersions(xiiStreamWriter& stream) const
{
  stream.WriteVersion(s_uiTypeVersionContextVersion);

  const xiiUInt32 uiNumTypes = m_KnownTypes.GetCount();
  stream << uiNumTypes;

  xiiMap<xiiString, const xiiRTTI*> sortedTypes;
  for (auto pType : m_KnownTypes)
  {
    sortedTypes.Insert(pType->GetTypeName(), pType);
  }

  for (const auto& it : sortedTypes)
  {
    stream << it.Key();
    stream << it.Value()->GetTypeVersion();
  }
}

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_SERIALIZATION_CONTEXT(xiiTypeVersionReadContext)

xiiTypeVersionReadContext::xiiTypeVersionReadContext(xiiStreamReader& stream)
{
  auto version = stream.ReadVersion(s_uiTypeVersionContextVersion);

  xiiUInt32 uiNumTypes = 0;
  stream >> uiNumTypes;

  xiiStringBuilder sTypeName;
  xiiUInt32        uiTypeVersion;

  for (xiiUInt32 i = 0; i < uiNumTypes; ++i)
  {
    stream >> sTypeName;
    stream >> uiTypeVersion;

    if (const xiiRTTI* pType = xiiRTTI::FindTypeByName(sTypeName))
    {
      m_TypeVersions.Insert(pType, uiTypeVersion);
    }
    else
    {
      xiiLog::Warning("Ignoring unknown type '{}'", sTypeName);
    }
  }
}

xiiTypeVersionReadContext::~xiiTypeVersionReadContext() = default;

xiiUInt32 xiiTypeVersionReadContext::GetTypeVersion(const xiiRTTI* pRtti) const
{
  xiiUInt32 uiVersion = xiiInvalidIndex;
  m_TypeVersions.TryGetValue(pRtti, uiVersion);

  return uiVersion;
}


XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_TypeVersionContext);
