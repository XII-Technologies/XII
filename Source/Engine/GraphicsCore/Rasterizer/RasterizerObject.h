#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Rasterizer/Thirdparty/Occluder.h>

class xiiGeometry;

class XII_RENDERERCORE_DLL xiiRasterizerObject : public xiiRefCounted
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRasterizerObject);

public:
  xiiRasterizerObject();
  ~xiiRasterizerObject();

  /// \brief If an object with the given name has been created before, it is returned, otherwise nullptr is returned.
  ///
  /// Use this to quickly query for an existing object. Call CreateMesh() in case the object doesn't exist yet.
  static xiiSharedPtr<const xiiRasterizerObject> GetObject(xiiStringView sUniqueName);

  /// \brief Creates a box object with the specified dimensions. If such a box was created before, the same pointer is returned.
  static xiiSharedPtr<const xiiRasterizerObject> CreateBox(const xiiVec3& vFullExtents);

  /// \brief Creates an object with the given geometry. If an object with the same name was created before, that pointer is returned instead.
  ///
  /// It is assumed that the same name will only be used for identical geometry.
  static xiiSharedPtr<const xiiRasterizerObject> CreateMesh(xiiStringView sUniqueName, const xiiGeometry& geometry);

private:
  void CreateMesh(const xiiGeometry& geometry);

  friend class xiiRasterizerView;
  Occluder m_Occluder;

  static xiiMutex                                             s_Mutex;
  static xiiMap<xiiString, xiiSharedPtr<xiiRasterizerObject>> s_Objects;
};
