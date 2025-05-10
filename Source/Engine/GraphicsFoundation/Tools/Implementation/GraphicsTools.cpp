#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>
#include <GraphicsFoundation/Tools/VertexBufferPool.h>

// clang-format off

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALScopedDebugGroup, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on


// Forward declaration of a user-defined vertex type.
// In a real application, VertexType would be a structure with position, normal, texture coordinates, etc.
struct Vertex
{
  XII_DECLARE_POD_TYPE();

  float x, y, z;
  // Additional attributes (normals, colors, texture coordinates) can be added here.
};

XII_GRAPHICSFOUNDATION_DLL void temp()
{
  xiiDynamicArray<int>          t;
  xiiGALVertexBufferPool<Vertex> pool;
}
