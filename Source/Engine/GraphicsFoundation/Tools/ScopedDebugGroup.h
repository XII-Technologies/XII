/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

/// \class xiiGALScopedDebugGroup
/// A helper class for managing debug groups in a command list.
///
/// This class allows developers to create scoped debug groups, which can be useful
/// for debugging and profiling graphics commands. When instantiated, it marks the
/// beginning of a debug group, and when destroyed, it marks the end.
///
/// \note This class ensures that debug groups are properly created and closed within
/// the scope they are used.
///
/// Usage example:
/// \code{.cpp}
/// {
///   xiiGALScopedDebugGroup debugGroup(pCommandList, "Rendering Pass", xiiColor::Red);
///   // Commands within this scope belong to the "Rendering Pass" debug group.
/// }
/// \endcode
class XII_GRAPHICSFOUNDATION_DLL xiiGALScopedDebugGroup
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALScopedDebugGroup);

public:
  /// Default constructor.
  ///
  /// \note Initializes an empty debug group.
  xiiGALScopedDebugGroup() noexcept;

  /// Constructs a debug group with the specified name and color.
  ///
  /// \param pCommandList - Pointer to the command list where the debug group is applied.
  /// \param sName        - Name of the debug group for debugging and profiling purposes.
  /// \param color        - Color used for visual representation (defaults to black).
  xiiGALScopedDebugGroup(xiiGALCommandList* pCommandList, xiiStringView sName, xiiColor color = xiiColor::White);

  /// Constructs a debug group with the specified name and color.
  ///
  /// \param commandList - Reference to the command list where the debug group is applied.
  /// \param sName       - Name of the debug group for debugging and profiling purposes.
  /// \param color       - Color used for visual representation (defaults to black).
  xiiGALScopedDebugGroup(xiiGALCommandList& commandList, xiiStringView sName, xiiColor color = xiiColor::White);

  /// Destructor, automatically ends the debug group.
  ~xiiGALScopedDebugGroup();

  /// Move constructor.
  ///
  /// \param rhs - Another debug group to move from.
  xiiGALScopedDebugGroup(xiiGALScopedDebugGroup&& rhs) noexcept;

  /// Move assignment operator.
  ///
  /// \param rhs - Another debug group to move from.
  ///
  /// \return Reference to this object.
  xiiGALScopedDebugGroup& operator=(xiiGALScopedDebugGroup&& rhs) noexcept;

private:
  xiiGALCommandList* m_pCommandList;
};

#define XII_COMMANDLIST_SCOPE(pCommandList, szName) \
  xiiGALScopedDebugGroup XII_PP_CONCAT(_xiiDebugGroupScope, XII_SOURCE_LINE)(pCommandList, szName)

#define XII_COMMANDLIST_SCOPE_COLOR(pCommandList, szName, color) \
  xiiGALScopedDebugGroup XII_PP_CONCAT(_xiiDebugGroupScope, XII_SOURCE_LINE)(pCommandList, szName, color)

#define XII_COMMANDLIST_SCOPE_FUNCTION(pCommandList) \
  xiiGALScopedDebugGroup XII_PP_CONCAT(_xiiDebugGroupScope, XII_SOURCE_LINE)(pCommandList, XII_SOURCE_FUNCTION)

#define XII_COMMANDLIST_SCOPE_FUNCTION_COLOR(pCommandList, color) \
  xiiGALScopedDebugGroup XII_PP_CONCAT(_xiiDebugGroupScope, XII_SOURCE_LINE)(pCommandList, XII_SOURCE_FUNCTION, color)

#include <GraphicsFoundation/Tools/Implementation/ScopedDebugGroup_inl.h>
