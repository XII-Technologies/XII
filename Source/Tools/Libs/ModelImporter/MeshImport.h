/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <ModelImporter/ModelImporterDLL.h>

/// Color conversion applied before packing imported vertex colors into native linear bytes.
struct XII_MODELIMPORTER_DLL xiiModelVertexColorConversion
{
  using StorageType = xiiUInt8;
  enum Enum
  {
    None,
    LinearToSRGB,
    SRGBToLinear,
    ENUM_COUNT,
    Default = None
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_MODELIMPORTER_DLL, xiiModelVertexColorConversion);

enum class xiiModelTextureUsage : xiiUInt8
{
  BaseColor,
  Normal,
  Roughness,
  Metallic,
  Occlusion,
  Emissive,
  Height,
  MetallicRoughness ///< Green contains roughness, blue contains metallic; occlusion is a separate input.
};

struct XII_MODELIMPORTER_DLL xiiModelMaterialTexture
{
  enum class Operation : xiiUInt8
  {
    Multiply,
    Add,
    Subtract,
    Divide,
    SmoothAdd,
    SignedAdd
  };
  xiiModelTextureUsage             m_Usage = xiiModelTextureUsage::BaseColor;
  xiiString                        m_sPath; ///< Source-relative path, or the key of an embedded texture. Owned UTF-8 text.
  xiiUInt32                        m_uiUVChannel = 0;
  xiiGALSamplerCreationDescription m_Sampler;
  xiiVec2                          m_vUVOffset   = xiiVec2::MakeZero();
  xiiVec2                          m_vUVScale    = xiiVec2(1);
  float                            m_fUVRotation = 0; ///< Radians.
  float                            m_fBlend      = 1;
  float                            m_fStrength   = 1; ///< Normal scale or occlusion strength; independent of layered blend.
  Operation                        m_Operation   = Operation::Multiply;
};

/// Imported authoring information, independent of Editor documents and runtime material ownership.
struct XII_MODELIMPORTER_DLL xiiModelMeshMaterial
{
  enum class AlphaMode : xiiUInt8
  {
    Opaque,
    Mask,
    Blend
  };

  xiiString                                m_sName;
  xiiColor                                 m_BaseColor     = xiiColor::White;
  xiiColor                                 m_EmissiveColor = xiiColor::Black;
  float                                    m_fMetallic     = 0;
  float                                    m_fRoughness    = 1;
  float                                    m_fOpacity      = 1; ///< Independent opacity multiplier; glTF alpha is already in m_BaseColor.a.
  float                                    m_fAlphaCutoff  = 0.5f;
  bool                                     m_bTwoSided     = false;
  AlphaMode                                m_AlphaMode     = AlphaMode::Opaque;
  xiiDynamicArray<xiiModelMaterialTexture> m_Textures;
};

struct XII_MODELIMPORTER_DLL xiiModelEmbeddedTexture
{
  xiiString                 m_sKey;           ///< Matches material texture paths, including named embedded textures.
  xiiString                 m_sFileExtension; ///< Format hint for encoded data, without the leading dot.
  xiiDynamicArray<xiiUInt8> m_Data;
  xiiUInt32                 m_uiWidth  = 0;
  xiiUInt32                 m_uiHeight = 0; ///< Both dimensions are zero for encoded bytes; otherwise data is tightly packed RGBA8.
};

struct XII_MODELIMPORTER_DLL xiiModelMeshImportOptions
{
  xiiMat4                                m_RootTransform        = xiiMat4::MakeIdentity();
  bool                                   m_bRecalculateNormals  = false;
  bool                                   m_bRecalculateTangents = true;
  bool                                   m_bImportSkinning      = false; ///< False imports a static snapshot of source geometry.
  bool                                   m_bNormalizeWeights    = true;
  bool                                   m_bFlipTextureV        = true; ///< Flip Assimp's canonical UVs; glTF is already flipped while decoding.
  xiiEnum<xiiModelVertexColorConversion> m_VertexColorConversion;
  float                                  m_fSimplification           = 0;     ///< Fraction of triangles to remove, in [0, 1). Zero disables simplification.
  float                                  m_fMaxSimplificationError   = 0.05f; ///< Error relative to mesh extents, in [0, 1].
  bool                                   m_bAggressiveSimplification = false;
};

/// Converts model files directly into native mesh descriptors. No scene/importer base classes or Qt.
/// Each mesh instance becomes a section; referenced source materials map to shared material slots.
/// Node/root transforms are baked into vertices, inverse-transpose normals and mirrored winding.
/// Skinning uses a named mesh-local palette, up to four strongest influences per vertex, and native
/// byte weights. The caller assigns the default skeleton resource and resolves material assets.
/// Native packed vertices retain two UV sets and one color set; additional sets are rejected.
/// Animated morph targets cannot be represented by the current native skinning path and are rejected.
/// ReadFile clears output on failure and never exposes pointers into Assimp-owned storage.
struct XII_MODELIMPORTER_DLL xiiModelMeshImport
{
  xiiMeshResourceDescriptor                m_Mesh;
  xiiDynamicArray<xiiModelMeshMaterial>    m_Materials; ///< Same indices as m_Mesh material slots.
  xiiDynamicArray<xiiModelEmbeddedTexture> m_EmbeddedTextures;

  static xiiResult ReadFile(xiiStringView sAbsoluteFile, const xiiModelMeshImportOptions& options, xiiModelMeshImport& out_import, xiiStringBuilder* out_pError = nullptr);
};
