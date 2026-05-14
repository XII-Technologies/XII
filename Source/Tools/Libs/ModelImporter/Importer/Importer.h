/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <ModelImporter2/ModelImporterDLL.h>

class xiiLogInterface;
class xiiProgress;
class xiiEditableSkeleton;
class xiiMeshResourceDescriptor;
struct xiiAnimationClipResourceDescriptor;

namespace xiiModelImporter2
{
  struct ImportOptions
  {
    xiiString m_sSourceFile;

    bool    m_bImportSkinningData = false;
    bool    m_bRecomputeNormals   = false;
    bool    m_bRecomputeTangents  = false;
    bool    m_bNormalizeWeights   = false;
    xiiMat3 m_RootTransform       = xiiMat3::MakeIdentity();

    xiiMeshResourceDescriptor*            m_pMeshOutput               = nullptr;
    xiiEnum<xiiMeshNormalPrecision>       m_MeshNormalsPrecision      = xiiMeshNormalPrecision::Default;
    xiiEnum<xiiMeshTexCoordPrecision>     m_MeshTexCoordsPrecision    = xiiMeshTexCoordPrecision::Default;
    xiiEnum<xiiMeshBoneWeigthPrecision>   m_MeshBoneWeightPrecision   = xiiMeshBoneWeigthPrecision::Default;
    xiiEnum<xiiMeshVertexColorConversion> m_MeshVertexColorConversion = xiiMeshVertexColorConversion::Default;

    xiiEditableSkeleton* m_pSkeletonOutput = nullptr;

    bool                                m_bAdditiveAnimation = false;
    xiiString                           m_sAnimationToImport; // empty = first in file; "name" = only anim with that name
    xiiAnimationClipResourceDescriptor* m_pAnimationOutput    = nullptr;
    xiiUInt32                           m_uiFirstAnimKeyframe = 0;
    xiiUInt32                           m_uiNumAnimKeyframes  = 0;

    xiiUInt8 m_uiMeshSimplification      = 0;
    xiiUInt8 m_uiMaxSimplificationError  = 5;
    bool     m_bAggressiveSimplification = false;
  };

  enum class PropertySemantic : xiiInt8
  {
    Unknown = 0,

    DiffuseColor,
    RoughnessValue,
    MetallicValue,
    EmissiveColor,
    TwosidedValue,
  };

  enum class TextureSemantic : xiiInt8
  {
    Unknown = 0,

    DiffuseMap,
    DiffuseAlphaMap,
    OcclusionMap,
    RoughnessMap,
    MetallicMap,
    OrmMap,
    DisplacementMap,
    NormalMap,
    EmissiveMap,
  };

  struct XII_MODELIMPORTER2_DLL OutputTexture
  {
    xiiString            m_sFilename;
    xiiString            m_sFileFormatExtension;
    xiiConstByteArrayPtr m_RawData;

    void GenerateFileName(xiiStringBuilder& out_sName) const;
  };

  struct XII_MODELIMPORTER2_DLL OutputMaterial
  {
    xiiString m_sName;

    xiiInt32                             m_iReferencedByMesh = -1; // if -1, no sub-mesh in the output actually references this
    xiiMap<TextureSemantic, xiiString>   m_TextureReferences;      // semantic -> path
    xiiMap<PropertySemantic, xiiVariant> m_Properties;             // semantic -> value
  };

  class XII_MODELIMPORTER2_DLL Importer
  {
  public:
    Importer();
    virtual ~Importer();

    xiiResult            Import(const ImportOptions& options, xiiLogInterface* pLogInterface = nullptr, xiiProgress* pProgress = nullptr);
    const ImportOptions& GetImportOptions() const { return m_Options; }

    xiiMap<xiiString, OutputTexture> m_OutputTextures; // path -> additional data
    xiiDeque<OutputMaterial>         m_OutputMaterials;
    xiiDynamicArray<xiiString>       m_OutputAnimationNames;

  protected:
    virtual xiiResult DoImport() = 0;

    ImportOptions m_Options;
    xiiProgress*  m_pProgress = nullptr;
  };

} // namespace xiiModelImporter2
