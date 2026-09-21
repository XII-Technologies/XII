/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Utilities/UtilitiesDLL.h>

/// A loader class for OBJ/MTL files.
///
/// The LoadOBJ() and LoadMTL() functions will parse the given files and add all information to the existing mesh data.
/// You can load multiple OBJ and MTL files into this object, all information will be merged.
/// Afterwards faces can be sorted by material and tangents and bi-tangents can be computed.
///
/// All shared information (positions, normals, texcoords) is stored using indices, so the information what is shared
/// is preserved in the xiiOBJLoader object. For upload into a GPU the vertex information must be duplicated manually.
class XII_UTILITIES_DLL xiiOBJLoader
{
public:
  /// Stores the information for a vertex in a face.
  struct FaceVertex
  {
    FaceVertex();

    xiiUInt32 m_uiPositionID; ///< Index into the m_Positions array
    xiiUInt32 m_uiNormalID;   ///< Index into the m_Normals array
    xiiUInt32 m_uiTexCoordID; ///< Index into the m_TexCoords array
  };

  /// Holds the information about one Material.
  ///
  /// Only the diffuse texture is actually read and stored by this loader, but if needed this can easily be extended.
  ///  The MaterialID is the ID of the Material itself, this is only needed by the loader.
  struct Material
  {
    /// The path to the diffuse texture of this material.
    xiiString m_sDiffuseTexture;

    /// The ID of this material.
    xiiUInt32 m_uiMaterialID;
  };

  /// Holds all data about one face (ie. polygon, not only triangles).
  struct Face
  {
    Face();

    /// The ID of the material, that this face uses.
    xiiUInt32 m_uiMaterialID;

    /// The face-normal, automatically computed
    xiiVec3 m_vNormal;

    // These are only calculated on demand (through ComputeTangentSpaceVectors) and only if texture-coordinates are available.
    // Useful, when doing normal-mapping in tangent-space.
    xiiVec3 m_vTangent;
    xiiVec3 m_vBiTangent;

    /// All vertices of the face.
    xiiHybridArray<FaceVertex, 4> m_Vertices;

    /// Less-than operator is needed for sorting faces by material.
    XII_ALWAYS_INLINE bool operator<(const Face& rhs) const { return (m_uiMaterialID < rhs.m_uiMaterialID); }
  };

  /// Clears all data. Call this before LoadOBJ() / LoadMTL(), if you want to reuse the loader object to load another OBJ file,
  /// without merging them.
  void Clear();

  /// Returns whether texture-coordinates are available for this mesh.
  bool HasTextureCoordinates() const { return (!m_TexCoords.IsEmpty()); }

  /// Returns whether vertex-normals are available for this mesh. Otherwise only face-normals are available.
  bool HasVertexNormals() const { return (!m_Normals.IsEmpty()); }

  /// Updates the tangent and bi-tangent vectors of the faces.
  void ComputeTangentSpaceVectors();

  /// Sorts all faces by their material.
  void SortFacesByMaterial();

  /// Loads an OBJ file into this object. Adds all information to the existing data, so multiple OBJ files can be merged.
  ///
  /// Returns XII_FAILURE if the given file could not be found.
  xiiResult LoadOBJ(const char* szFile, bool bIgnoreMaterials = false);

  /// Loads and MTL file for material information.
  ///
  /// You can load multiple MTL files to merge them into one object. You can load an MTL file before or after loading OBJ files
  /// the missing information will be filled out whenever it is available.
  ///
  /// Returns XII_FAILURE when the given file could not be found.
  xiiResult LoadMTL(const char* szFile, const char* szMaterialBasePath = "");


  xiiMap<xiiString, Material> m_Materials;

  xiiDeque<xiiVec3> m_Positions;
  xiiDeque<xiiVec3> m_Normals;
  xiiDeque<xiiVec3> m_TexCoords;
  xiiDeque<Face>    m_Faces;
};
