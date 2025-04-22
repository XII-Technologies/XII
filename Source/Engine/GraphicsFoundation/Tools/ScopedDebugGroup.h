#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

/// \class xiiGALScopedDebugGroup
/// \brief A helper class for managing debug groups in a command list.
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
class XII_GRAPHICSFOUNDATION_DLL xiiGALScopedDebugGroup : public xiiReflectedClass
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALScopedDebugGroup);

  XII_ADD_DYNAMIC_REFLECTION(xiiGALScopedDebugGroup, xiiReflectedClass);

public:
  /// \brief Default constructor.
  ///
  /// \note Initializes an empty debug group.
  xiiGALScopedDebugGroup() noexcept;

  /// \brief Constructs a debug group with the specified name and color.
  ///
  /// \param pCommandList - Pointer to the command list where the debug group is applied.
  /// \param sName        - Name of the debug group for debugging and profiling purposes.
  /// \param color        - Color used for visual representation (defaults to black).
  xiiGALScopedDebugGroup(xiiGALCommandList* pCommandList, xiiStringView sName, xiiColor color = xiiColor::Black);

  /// \brief Destructor, automatically ends the debug group.
  ~xiiGALScopedDebugGroup();

  /// \brief Move constructor.
  ///
  /// \param rhs - Another debug group to move from.
  xiiGALScopedDebugGroup(xiiGALScopedDebugGroup&& rhs) noexcept;

  /// \brief Move assignment operator.
  ///
  /// \param rhs - Another debug group to move from.
  ///
  /// \return Reference to this object.
  xiiGALScopedDebugGroup& operator=(xiiGALScopedDebugGroup&& rhs) noexcept;

private:
  xiiGALCommandList* m_pCommandList; ///< Pointer to the command list associated with this debug group.
};

#include <GraphicsFoundation/Tools/Implementation/ScopedDebugGroup_inl.h>
