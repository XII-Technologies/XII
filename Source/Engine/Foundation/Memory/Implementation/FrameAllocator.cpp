/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>

xiiDoubleBufferedLinearAllocator::xiiDoubleBufferedLinearAllocator(xiiStringView sName, xiiAllocator* pParent)
{
  constexpr xiiUInt32 uiInitialSize = 1024 * 1024; // 1 MB

  xiiStringBuilder sb = sName;
  sb.Append("0");

  m_pCurrentAllocator = XII_DEFAULT_NEW(LinearAllocatorType, sb, pParent, uiInitialSize);

  sb = sName;
  sb.Append("1");

  m_pOtherAllocator = XII_DEFAULT_NEW(LinearAllocatorType, sb, pParent, uiInitialSize);
}

xiiDoubleBufferedLinearAllocator::~xiiDoubleBufferedLinearAllocator()
{
  XII_DEFAULT_DELETE(m_pCurrentAllocator);
  XII_DEFAULT_DELETE(m_pOtherAllocator);
}

void xiiDoubleBufferedLinearAllocator::Swap()
{
  xiiMath::Swap(m_pCurrentAllocator, m_pOtherAllocator);

  m_pCurrentAllocator->Reset();
}

void xiiDoubleBufferedLinearAllocator::Reset()
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

xiiDoubleBufferedLinearAllocator* xiiFrameAllocator::s_pAllocator;

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
  s_pAllocator = XII_DEFAULT_NEW(xiiDoubleBufferedLinearAllocator, "FrameAllocator", xiiFoundation::GetAlignedAllocator());
}

// static
void xiiFrameAllocator::Shutdown()
{
  XII_DEFAULT_DELETE(s_pAllocator);
}

XII_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_FrameAllocator);
