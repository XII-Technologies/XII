/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Memory/LinearAllocator.h>

/// A double buffered linear allocator for temporary per-frame allocations.
///
/// This allocator maintains two linear allocators and swaps between them each frame.
/// One allocator is used for the current frame while the previous frame's allocator is reset.
/// This pattern ensures that allocations from the previous frame remain valid until the
/// next frame begins, which is useful for data that needs to persist across frame boundaries.
class XII_FOUNDATION_DLL xiiDoubleBufferedLinearAllocator
{
public:
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  static constexpr bool OverwriteMemoryOnReset = true;
#else
  static constexpr bool OverwriteMemoryOnReset = false;
#endif
  using LinearAllocatorType = xiiLinearAllocator<xiiAllocatorTrackingMode::Basics, OverwriteMemoryOnReset>;

  xiiDoubleBufferedLinearAllocator(xiiStringView sName, xiiAllocator* pParent);
  ~xiiDoubleBufferedLinearAllocator();

  XII_ALWAYS_INLINE xiiAllocator* GetCurrentAllocator() const { return m_pCurrentAllocator; }

  void Swap();
  void Reset();

private:
  LinearAllocatorType* m_pCurrentAllocator;
  LinearAllocatorType* m_pOtherAllocator;
};

/// Global frame allocator for temporary allocations that are reset each frame.
///
/// This is a convenience wrapper around xiiDoubleBufferedLinearAllocator that provides a global
/// instance for frame-based allocations. Very efficient for temporary data that only needs to
/// live for one frame (UI layouts, temporary strings, intermediate calculations, etc.).
///
/// Usage pattern:
/// - GetCurrentAllocator() - get allocator for current frame
/// - Swap() - called once per frame to switch buffers (usually by the engine)
/// - Reset() - resets both buffers (usually called during shutdown)
///
/// Performance characteristics:
/// - Allocation: O(1) - just advances a pointer
/// - Deallocation: Not supported individually, entire frame is reset at once
/// - Memory overhead: Minimal, just stack pointers
class XII_FOUNDATION_DLL xiiFrameAllocator
{
public:
  /// Returns the allocator for the current frame.
  ///
  /// All allocations from this allocator will be automatically freed when the frame ends.
  XII_ALWAYS_INLINE static xiiAllocator* GetCurrentAllocator() { return s_pAllocator->GetCurrentAllocator(); }

  /// Swaps the active buffer, should be called once per frame.
  ///
  /// This makes the previous frame's allocations invalid and resets the allocator for new allocations.
  static void Swap();

  /// Resets both buffers, typically called during shutdown.
  static void Reset();

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FrameAllocator);

  static void Startup();
  static void Shutdown();

  static xiiDoubleBufferedLinearAllocator* s_pAllocator;
};
