#pragma once

#include <Foundation/Memory/StackAllocator.h>

/// \brief A double buffered stack allocator
class XII_FOUNDATION_DLL xiiDoubleBufferedStackAllocator
{
public:
  using StackAllocatorType = xiiStackAllocator<xiiAllocatorTrackingMode::Basics>;

  xiiDoubleBufferedStackAllocator(xiiStringView sName, xiiAllocatorBase* pParent);
  ~xiiDoubleBufferedStackAllocator();

  XII_ALWAYS_INLINE xiiAllocatorBase* GetCurrentAllocator() const { return m_pCurrentAllocator; }

  void Swap();
  void Reset();

private:
  StackAllocatorType* m_pCurrentAllocator;
  StackAllocatorType* m_pOtherAllocator;
};

class XII_FOUNDATION_DLL xiiFrameAllocator
{
public:
  XII_ALWAYS_INLINE static xiiAllocatorBase* GetCurrentAllocator() { return s_pAllocator->GetCurrentAllocator(); }

  static void Swap();
  static void Reset();

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FrameAllocator);

  static void Startup();
  static void Shutdown();

  static xiiDoubleBufferedStackAllocator* s_pAllocator;
};
