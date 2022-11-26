#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>

xiiDoubleBufferedStackAllocator::xiiDoubleBufferedStackAllocator(const char* szName, xiiAllocatorBase* pParent)
{
  xiiStringBuilder sName = szName;
  sName.Append("0");

  m_pCurrentAllocator = XII_DEFAULT_NEW(StackAllocatorType, sName, pParent);

  sName = szName;
  sName.Append("1");

  m_pOtherAllocator = XII_DEFAULT_NEW(StackAllocatorType, sName, pParent);
}

xiiDoubleBufferedStackAllocator::~xiiDoubleBufferedStackAllocator()
{
  XII_DEFAULT_DELETE(m_pCurrentAllocator);
  XII_DEFAULT_DELETE(m_pOtherAllocator);
}

void xiiDoubleBufferedStackAllocator::Swap()
{
  xiiMath::Swap(m_pCurrentAllocator, m_pOtherAllocator);

  m_pCurrentAllocator->Reset();
}

void xiiDoubleBufferedStackAllocator::Reset()
{
  m_pCurrentAllocator->Reset();
  m_pOtherAllocator->Reset();
}


// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FrameAllocator)

  ON_CORESYSTEMS_STARTUP
  {
    xiiFrameAllocator::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiFrameAllocator::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiDoubleBufferedStackAllocator* xiiFrameAllocator::s_pAllocator;

// static
void xiiFrameAllocator::Swap()
{
  XII_PROFILE_SCOPE("FrameAllocator.Swap");

  s_pAllocator->Swap();
}

// static
void xiiFrameAllocator::Reset()
{
  if (s_pAllocator)
  {
    s_pAllocator->Reset();
  }
}

// static
void xiiFrameAllocator::Startup()
{
  s_pAllocator = XII_DEFAULT_NEW(xiiDoubleBufferedStackAllocator, "FrameAllocator", xiiFoundation::GetAlignedAllocator());
}

// static
void xiiFrameAllocator::Shutdown()
{
  XII_DEFAULT_DELETE(s_pAllocator);
}

XII_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_FrameAllocator);
