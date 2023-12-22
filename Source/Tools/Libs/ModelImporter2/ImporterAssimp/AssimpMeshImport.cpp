#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <GraphicsCore/Meshes/MeshResourceDescriptor.h>
#include <assimp/scene.h>
#include <mikktspace/mikktspace.h>

namespace xiiModelImporter2
{
  struct StreamIndices
  {
    xiiUInt32 uiPositions = xiiInvalidIndex;
    xiiUInt32 uiNormals   = xiiInvalidIndex;
    xiiUInt32 uiUV0       = xiiInvalidIndex;
    xiiUInt32 uiUV1       = xiiInvalidIndex;
    xiiUInt32 uiTangents  = xiiInvalidIndex;
    xiiUInt32 uiBoneIdx   = xiiInvalidIndex;
    xiiUInt32 uiBoneWgt   = xiiInvalidIndex;
    xiiUInt32 uiColor0    = xiiInvalidIndex;
    xiiUInt32 uiColor1    = xiiInvalidIndex;
  };

  xiiResult ImporterAssimp::ProcessAiMesh(aiMesh* pMesh, const xiiMat4& transform)
  {
    if ((pMesh->mPrimitiveTypes & aiPrimitiveType::aiPrimitiveType_TRIANGLE) == 0) // no triangles in there ?
      return XII_SUCCESS;

    {
      auto& mi             = m_MeshInstances[pMesh->mMaterialIndex].ExpandAndGetRef();
      mi.m_GlobalTransform = transform;
      mi.m_pMesh           = pMesh;

      m_uiTotalMeshVertices += pMesh->mNumVertices;
      m_uiTotalMeshTriangles += pMesh->mNumFaces;
    }

    return XII_SUCCESS;
  }

  static void SetMeshTriangleIndices(xiiMeshBufferResourceDescriptor& ref_mb, const aiMesh* pMesh, xiiUInt32 uiTriangleIndexOffset, xiiUInt32 uiVertexIndexOffset, bool bFlipTriangles)
  {
    if (bFlipTriangles)
    {
      for (xiiUInt32 triIdx = 0; triIdx < pMesh->mNumFaces; ++triIdx)
      {
        const xiiUInt32 finalTriIdx = uiTriangleIndexOffset + triIdx;

        const xiiUInt32 f0 = pMesh->mFaces[triIdx].mIndices[0];
        const xiiUInt32 f1 = pMesh->mFaces[triIdx].mIndices[1];
        const xiiUInt32 f2 = pMesh->mFaces[triIdx].mIndices[2];

        ref_mb.SetTriangleIndices(finalTriIdx, uiVertexIndexOffset + f0, uiVertexIndexOffset + f2, uiVertexIndexOffset + f1);
      }
    }
    else
    {
      for (xiiUInt32 triIdx = 0; triIdx < pMesh->mNumFaces; ++triIdx)
      {
        const xiiUInt32 finalTriIdx = uiTriangleIndexOffset + triIdx;

        const xiiUInt32 f0 = pMesh->mFaces[triIdx].mIndices[0];
        const xiiUInt32 f1 = pMesh->mFaces[triIdx].mIndices[1];
        const xiiUInt32 f2 = pMesh->mFaces[triIdx].mIndices[2];

        ref_mb.SetTriangleIndices(finalTriIdx, uiVertexIndexOffset + f0, uiVertexIndexOffset + f1, uiVertexIndexOffset + f2);
      }
    }
  }

  static void SetMeshBoneData(xiiMeshBufferResourceDescriptor& ref_mb, xiiMeshResourceDescriptor& ref_mrd, float& inout_fMaxBoneOffset, const aiMesh* pMesh, xiiUInt32 uiVertexIndexOffset, const StreamIndices& streams, bool b8BitBoneIndices, xiiMeshBoneWeigthPrecision::Enum weightsPrecision, bool bNormalizeWeights)
  {
    if (!pMesh->HasBones())
      return;

    xiiHashedString hs;

    for (xiiUInt32 b = 0; b < pMesh->mNumBones; ++b)
    {
      const aiBone* pBone = pMesh->mBones[b];

      hs.Assign(pBone->mName.C_Str());
      const xiiUInt32 uiBoneIndex = ref_mrd.m_Bones[hs].m_uiBoneIndex;

      for (xiiUInt32 w = 0; w < pBone->mNumWeights; ++w)
      {
        const auto& weight = pBone->mWeights[w];

        const xiiUInt32 finalVertIdx = uiVertexIndexOffset + weight.mVertexId;

        xiiUInt8*       pBoneIndices8  = reinterpret_cast<xiiUInt8*>(ref_mb.GetVertexData(streams.uiBoneIdx, finalVertIdx).GetPtr());
        xiiUInt16*      pBoneIndices16 = reinterpret_cast<xiiUInt16*>(pBoneIndices8);
        xiiByteArrayPtr pBoneWeights   = ref_mb.GetVertexData(streams.uiBoneWgt, finalVertIdx);

        xiiVec4 wgt;
        xiiMeshBufferUtils::DecodeToVec4(pBoneWeights, xiiMeshBoneWeigthPrecision::ToResourceFormat(weightsPrecision), wgt).AssertSuccess();

        // pBoneWeights are initialized with 0
        // so for the first 4 bones we always assign to one slot that is 0 (least weight)
        // if we have 5 bones or more, we then replace the currently lowest value each time

        xiiUInt32 uiLeastWeightIdx = 0;

        for (int i = 1; i < 4; ++i)
        {
          if (wgt.GetData()[i] < wgt.GetData()[uiLeastWeightIdx])
          {
            uiLeastWeightIdx = i;
          }
        }

        const float fEncodedWeight = weight.mWeight;

        if (wgt.GetData()[uiLeastWeightIdx] < fEncodedWeight)
        {
          wgt.GetData()[uiLeastWeightIdx] = fEncodedWeight;

          xiiMeshBufferUtils::EncodeBoneWeights(wgt, pBoneWeights, xiiMeshBoneWeigthPrecision::ToResourceFormat(weightsPrecision)).AssertSuccess();

          if (b8BitBoneIndices)
            pBoneIndices8[uiLeastWeightIdx] = static_cast<xiiUInt8>(uiBoneIndex);
          else
            pBoneIndices16[uiLeastWeightIdx] = static_cast<xiiUInt16>(uiBoneIndex);
        }
      }
    }

    for (xiiUInt32 vtx = 0; vtx < ref_mb.GetVertexCount(); ++vtx)
    {
      xiiByteArrayPtr pBoneWeights = ref_mb.GetVertexData(streams.uiBoneWgt, vtx);

      xiiVec4 wgt;
      xiiMeshBufferUtils::DecodeToVec4(pBoneWeights, xiiMeshBoneWeigthPrecision::ToResourceFormat(weightsPrecision), wgt).AssertSuccess();

      // normalize the bone weights
      if (bNormalizeWeights)
      {
        // NOTE: This is absolutely crucial for some meshes to work right
        // On the other hand, it is also possible that some meshes don't like this
        // if we come across meshes where normalization breaks them, we may need to add a user-option to select whether bone weights should be normalized

        const float len = wgt.x + wgt.y + wgt.z + wgt.w;
        if (len > 1.0f)
        {
          wgt /= len;

          xiiMeshBufferUtils::EncodeBoneWeights(wgt, pBoneWeights, weightsPrecision).AssertSuccess();
        }
      }

      const xiiVec3    vVertexPos     = *reinterpret_cast<const xiiVec3*>(ref_mb.GetVertexData(streams.uiPositions, vtx).GetPtr());
      const xiiUInt8*  pBoneIndices8  = reinterpret_cast<const xiiUInt8*>(ref_mb.GetVertexData(streams.uiBoneIdx, vtx).GetPtr());
      const xiiUInt16* pBoneIndices16 = reinterpret_cast<const xiiUInt16*>(ref_mb.GetVertexData(streams.uiBoneIdx, vtx).GetPtr());

      // also find the maximum distance of any vertex to its influencing bones
      // this is used to adjust the bounding box for culling at runtime
      // ie. we can compute the bounding box from a pose, but that only covers the skeleton, not the full mesh
      // so we then grow the bbox by this maximum distance
      // that usually creates a far larger bbox than necessary, but means there are no culling artifacts

      xiiUInt16 uiBoneId[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

      if (b8BitBoneIndices)
      {
        uiBoneId[0] = pBoneIndices8[0];
        uiBoneId[1] = pBoneIndices8[1];
        uiBoneId[2] = pBoneIndices8[2];
        uiBoneId[3] = pBoneIndices8[3];
      }
      else
      {
        uiBoneId[0] = pBoneIndices16[0];
        uiBoneId[1] = pBoneIndices16[1];
        uiBoneId[2] = pBoneIndices16[2];
        uiBoneId[3] = pBoneIndices16[3];
      }

      for (const auto& bone : ref_mrd.m_Bones)
      {
        for (int b = 0; b < 4; ++b)
        {
          if (wgt.GetData()[b] < 0.2f) // only look at bones that have a proper weight
            continue;

          if (bone.Value().m_uiBoneIndex == uiBoneId[b])
          {
            // move the vertex into local space of the bone, then determine how far it is away from the bone
            const xiiVec3 vOffPos = bone.Value().m_GlobalInverseRestPoseMatrix * vVertexPos;
            const float   length  = vOffPos.GetLength();

            if (length > inout_fMaxBoneOffset)
            {
              inout_fMaxBoneOffset = length;
            }
          }
        }
      }
    }
  }

  static void SetMeshVertexData(xiiMeshBufferResourceDescriptor& ref_mb, const aiMesh* pMesh, const xiiMat4& mGlobalTransform, xiiUInt32 uiVertexIndexOffset, const StreamIndices& streams, xiiEnum<xiiMeshNormalPrecision> meshNormalsPrecision, xiiEnum<xiiMeshTexCoordPrecision> meshTexCoordsPrecision)
  {
    xiiMat3 normalsTransform = mGlobalTransform.GetRotationalPart();
    if (normalsTransform.Invert(0.0f).Failed())
    {
      xiiLog::Warning("Couldn't invert a mesh's transform matrix.");
      normalsTransform.SetIdentity();
    }

    normalsTransform.Transpose();

    for (xiiUInt32 vertIdx = 0; vertIdx < pMesh->mNumVertices; ++vertIdx)
    {
      const xiiUInt32 finalVertIdx = uiVertexIndexOffset + vertIdx;

      const xiiVec3 position = mGlobalTransform * ConvertAssimpType(pMesh->mVertices[vertIdx]);
      ref_mb.SetVertexData(streams.uiPositions, finalVertIdx, position);

      if (streams.uiNormals != xiiInvalidIndex && pMesh->HasNormals())
      {
        xiiVec3 normal = normalsTransform * ConvertAssimpType(pMesh->mNormals[vertIdx]);
        normal.NormalizeIfNotZero(xiiVec3::ZeroVector()).IgnoreResult();

        xiiMeshBufferUtils::EncodeNormal(normal, ref_mb.GetVertexData(streams.uiNormals, finalVertIdx), meshNormalsPrecision).IgnoreResult();
      }

      if (streams.uiUV0 != xiiInvalidIndex && pMesh->HasTextureCoords(0))
      {
        const xiiVec2 texcoord = ConvertAssimpType(pMesh->mTextureCoords[0][vertIdx]).GetAsVec2();

        xiiMeshBufferUtils::EncodeTexCoord(texcoord, ref_mb.GetVertexData(streams.uiUV0, finalVertIdx), meshTexCoordsPrecision).IgnoreResult();
      }

      if (streams.uiUV1 != xiiInvalidIndex && pMesh->HasTextureCoords(1))
      {
        const xiiVec2 texcoord = ConvertAssimpType(pMesh->mTextureCoords[1][vertIdx]).GetAsVec2();

        xiiMeshBufferUtils::EncodeTexCoord(texcoord, ref_mb.GetVertexData(streams.uiUV1, finalVertIdx), meshTexCoordsPrecision).IgnoreResult();
      }

      if (streams.uiColor0 != xiiInvalidIndex && pMesh->HasVertexColors(0))
      {
        const xiiColorLinearUB color = ConvertAssimpType(pMesh->mColors[0][vertIdx]);
        ref_mb.SetVertexData(streams.uiColor0, finalVertIdx, color);
      }

      if (streams.uiColor1 != xiiInvalidIndex && pMesh->HasVertexColors(1))
      {
        const xiiColorLinearUB color = ConvertAssimpType(pMesh->mColors[1][vertIdx]);
        ref_mb.SetVertexData(streams.uiColor1, finalVertIdx, color);
      }

      if (streams.uiTangents != xiiInvalidIndex && pMesh->HasTangentsAndBitangents())
      {
        xiiVec3 normal    = normalsTransform * ConvertAssimpType(pMesh->mNormals[vertIdx]);
        xiiVec3 tangent   = normalsTransform * ConvertAssimpType(pMesh->mTangents[vertIdx]);
        xiiVec3 bitangent = normalsTransform * ConvertAssimpType(pMesh->mBitangents[vertIdx]);

        normal.NormalizeIfNotZero(xiiVec3::ZeroVector()).IgnoreResult();
        tangent.NormalizeIfNotZero(xiiVec3::ZeroVector()).IgnoreResult();
        bitangent.NormalizeIfNotZero(xiiVec3::ZeroVector()).IgnoreResult();

        const float fBitangentSign = xiiMath::Abs(tangent.CrossRH(bitangent).Dot(normal));

        xiiMeshBufferUtils::EncodeTangent(tangent, fBitangentSign, ref_mb.GetVertexData(streams.uiTangents, finalVertIdx), meshNormalsPrecision).IgnoreResult();
      }
    }
  }

  static void AllocateMeshStreams(xiiMeshBufferResourceDescriptor& ref_mb, xiiArrayPtr<aiMesh*> referenceMeshes, StreamIndices& inout_streams, xiiUInt32 uiTotalMeshVertices, xiiUInt32 uiTotalMeshTriangles, xiiEnum<xiiMeshNormalPrecision> meshNormalsPrecision, xiiEnum<xiiMeshTexCoordPrecision> meshTexCoordsPrecision, bool bImportSkinningData, bool b8BitBoneIndices, xiiEnum<xiiMeshBoneWeigthPrecision> meshWeightsPrecision)
  {
    inout_streams.uiPositions = ref_mb.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALTextureFormat::RGB32Float);
    inout_streams.uiNormals   = ref_mb.AddStream(xiiGALInputLayoutSemantic::Normal, xiiMeshNormalPrecision::ToResourceFormatNormal(meshNormalsPrecision));
    inout_streams.uiUV0       = ref_mb.AddStream(xiiGALInputLayoutSemantic::TexCoord0, xiiMeshTexCoordPrecision::ToResourceFormat(meshTexCoordsPrecision));
    inout_streams.uiTangents  = ref_mb.AddStream(xiiGALInputLayoutSemantic::Tangent, xiiMeshNormalPrecision::ToResourceFormatTangent(meshNormalsPrecision));

    if (bImportSkinningData)
    {
      if (b8BitBoneIndices)
        inout_streams.uiBoneIdx = ref_mb.AddStream(xiiGALInputLayoutSemantic::BoneIndices0, xiiGALTextureFormat::RGBA8UInt);
      else
        inout_streams.uiBoneIdx = ref_mb.AddStream(xiiGALInputLayoutSemantic::BoneIndices0, xiiGALTextureFormat::RGBA16UInt);

      inout_streams.uiBoneWgt = ref_mb.AddStream(xiiGALInputLayoutSemantic::BoneWeights0, xiiMeshBoneWeigthPrecision::ToResourceFormat(meshWeightsPrecision));
    }

    bool bTexCoords1    = false;
    bool bVertexColors0 = false;
    bool bVertexColors1 = false;

    for (auto pMesh : referenceMeshes)
    {
      if (pMesh->HasTextureCoords(1))
        bTexCoords1 = true;
      if (pMesh->HasVertexColors(0))
        bVertexColors0 = true;
      if (pMesh->HasVertexColors(1))
        bVertexColors1 = true;
    }

    if (bTexCoords1)
    {
      inout_streams.uiUV1 = ref_mb.AddStream(xiiGALInputLayoutSemantic::TexCoord1, xiiMeshTexCoordPrecision::ToResourceFormat(meshTexCoordsPrecision));
    }

    if (bVertexColors0)
    {
      inout_streams.uiColor0 = ref_mb.AddStream(xiiGALInputLayoutSemantic::Color0, xiiGALTextureFormat::RGBA8UNormalized);
    }
    if (bVertexColors1)
    {
      inout_streams.uiColor1 = ref_mb.AddStream(xiiGALInputLayoutSemantic::Color1, xiiGALTextureFormat::RGBA8UNormalized);
    }

    ref_mb.AllocateStreams(uiTotalMeshVertices, xiiGALPrimitiveTopology::TriangleList, uiTotalMeshTriangles, true);
  }

  static void SetMeshBindPoseData(xiiMeshResourceDescriptor& ref_mrd, const aiMesh* pMesh, const xiiMat4& mGlobalTransform)
  {
    if (!pMesh->HasBones())
      return;

    xiiHashedString hs;

    for (xiiUInt32 b = 0; b < pMesh->mNumBones; ++b)
    {
      auto pBone = pMesh->mBones[b];

      auto invPose = ConvertAssimpType(pBone->mOffsetMatrix);
      XII_VERIFY(invPose.Invert(0.0f).Succeeded(), "Inverting the bind pose matrix failed");
      invPose = mGlobalTransform * invPose;
      XII_VERIFY(invPose.Invert(0.0f).Succeeded(), "Inverting the bind pose matrix failed");

      hs.Assign(pBone->mName.C_Str());
      ref_mrd.m_Bones[hs].m_GlobalInverseRestPoseMatrix = invPose;
    }
  }

  struct MikkData
  {
    xiiMeshBufferResourceDescriptor* m_pMeshBuffer  = nullptr;
    xiiUInt32                        m_uiVertexSize = 0;
    const xiiUInt16*                 m_pIndices16   = nullptr;
    const xiiUInt32*                 m_pIndices32   = nullptr;
    const xiiUInt8*                  m_pPositions   = nullptr;
    const xiiUInt8*                  m_pNormals     = nullptr;
    const xiiUInt8*                  m_pTexCoords   = nullptr;
    xiiUInt8*                        m_pTangents    = nullptr;
    xiiGALTextureFormat::Enum        m_NormalsFormat;
    xiiGALTextureFormat::Enum        m_TexCoordsFormat;
    xiiGALTextureFormat::Enum        m_TangentsFormat;
  };

  static int MikkGetNumFaces(const SMikkTSpaceContext* pContext)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);
    return pMikkData->m_pMeshBuffer->GetPrimitiveCount();
  }

  static int MikkGetNumVerticesOfFace(const SMikkTSpaceContext* pContext, int iFace)
  { //
    return 3;
  }

  static void MikkGetPosition16(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData*       pMikkData   = static_cast<MikkData*>(pContext->m_pUserData);
    const xiiUInt32 uiVertexIdx = pMikkData->m_pIndices16[iFace * 3 + iVert];

    const xiiVec3* pSrcData = reinterpret_cast<const xiiVec3*>(pMikkData->m_pPositions + (uiVertexIdx * pMikkData->m_uiVertexSize));
    pData[0]                = pSrcData->x;
    pData[1]                = pSrcData->y;
    pData[2]                = pSrcData->z;
  }

  static void MikkGetPosition32(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);

    const xiiUInt32 uiVertexIdx = pMikkData->m_pIndices32[iFace * 3 + iVert];

    const xiiVec3* pSrcData = reinterpret_cast<const xiiVec3*>(pMikkData->m_pPositions + (uiVertexIdx * pMikkData->m_uiVertexSize));
    pData[0]                = pSrcData->x;
    pData[1]                = pSrcData->y;
    pData[2]                = pSrcData->z;
  }

  static void MikkGetNormal16(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData*       pMikkData   = static_cast<MikkData*>(pContext->m_pUserData);
    const xiiUInt32 uiVertexIdx = pMikkData->m_pIndices16[iFace * 3 + iVert];

    xiiVec3* pDest = reinterpret_cast<xiiVec3*>(pData);
    xiiMeshBufferUtils::DecodeNormal(xiiConstByteArrayPtr(pMikkData->m_pNormals + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_NormalsFormat, *pDest).IgnoreResult();
  }

  static void MikkGetNormal32(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData*       pMikkData   = static_cast<MikkData*>(pContext->m_pUserData);
    const xiiUInt32 uiVertexIdx = pMikkData->m_pIndices32[iFace * 3 + iVert];

    xiiVec3* pDest = reinterpret_cast<xiiVec3*>(pData);
    xiiMeshBufferUtils::DecodeNormal(xiiConstByteArrayPtr(pMikkData->m_pNormals + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_NormalsFormat, *pDest).IgnoreResult();
  }

  static void MikkGetTexCoord16(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData*       pMikkData   = static_cast<MikkData*>(pContext->m_pUserData);
    const xiiUInt32 uiVertexIdx = pMikkData->m_pIndices16[iFace * 3 + iVert];

    xiiVec2* pDest = reinterpret_cast<xiiVec2*>(pData);
    xiiMeshBufferUtils::DecodeTexCoord(xiiConstByteArrayPtr(pMikkData->m_pTexCoords + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_TexCoordsFormat, *pDest).IgnoreResult();
  }

  static void MikkGetTexCoord32(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData*       pMikkData   = static_cast<MikkData*>(pContext->m_pUserData);
    const xiiUInt32 uiVertexIdx = pMikkData->m_pIndices32[iFace * 3 + iVert];

    xiiVec2* pDest = reinterpret_cast<xiiVec2*>(pData);
    xiiMeshBufferUtils::DecodeTexCoord(xiiConstByteArrayPtr(pMikkData->m_pTexCoords + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_TexCoordsFormat, *pDest).IgnoreResult();
  }

  static void MikkSetTangents16(const SMikkTSpaceContext* pContext, const float pTangent[], const float fSign, const int iFace, const int iVert)
  {
    MikkData*       pMikkData   = static_cast<MikkData*>(pContext->m_pUserData);
    const xiiUInt32 uiVertexIdx = pMikkData->m_pIndices16[iFace * 3 + iVert];

    const xiiVec3 tangent = *reinterpret_cast<const xiiVec3*>(pTangent);

    xiiMeshBufferUtils::EncodeTangent(tangent, fSign, xiiByteArrayPtr(pMikkData->m_pTangents + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_TangentsFormat).IgnoreResult();
  }

  static void MikkSetTangents32(const SMikkTSpaceContext* pContext, const float pTangent[], const float fSign, const int iFace, const int iVert)
  {
    MikkData*       pMikkData   = static_cast<MikkData*>(pContext->m_pUserData);
    const xiiUInt32 uiVertexIdx = pMikkData->m_pIndices32[iFace * 3 + iVert];

    const xiiVec3 tangent = *reinterpret_cast<const xiiVec3*>(pTangent);

    xiiMeshBufferUtils::EncodeTangent(tangent, fSign, xiiByteArrayPtr(pMikkData->m_pTangents + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_TangentsFormat).IgnoreResult();
  }

  xiiResult ImporterAssimp::RecomputeTangents()
  {
    auto& md = m_Options.m_pMeshOutput->MeshBufferDesc();

    MikkData mikkd;
    mikkd.m_pMeshBuffer  = &md;
    mikkd.m_uiVertexSize = md.GetVertexDataSize();
    mikkd.m_pIndices16   = reinterpret_cast<const xiiUInt16*>(md.GetIndexBufferData().GetData());
    mikkd.m_pIndices32   = reinterpret_cast<const xiiUInt32*>(md.GetIndexBufferData().GetData());

    for (xiiUInt32 i = 0; i < md.GetInputLayout().m_VertexStreams.GetCount(); ++i)
    {
      const auto& stream = md.GetInputLayout().m_VertexStreams[i];

      if (stream.m_Semantic == xiiGALInputLayoutSemantic::Position)
      {
        mikkd.m_pPositions = md.GetVertexData(i, 0).GetPtr();
      }
      else if (stream.m_Semantic == xiiGALInputLayoutSemantic::Normal)
      {
        mikkd.m_pNormals      = md.GetVertexData(i, 0).GetPtr();
        mikkd.m_NormalsFormat = stream.m_Format;
      }
      else if (stream.m_Semantic == xiiGALInputLayoutSemantic::TexCoord0)
      {
        mikkd.m_pTexCoords      = md.GetVertexData(i, 0).GetPtr();
        mikkd.m_TexCoordsFormat = stream.m_Format;
      }
      else if (stream.m_Semantic == xiiGALInputLayoutSemantic::Tangent)
      {
        mikkd.m_pTangents      = md.GetVertexData(i, 0).GetPtr();
        mikkd.m_TangentsFormat = stream.m_Format;
      }
    }

    if (mikkd.m_pPositions == nullptr || mikkd.m_pTexCoords == nullptr || mikkd.m_pNormals == nullptr || mikkd.m_pTangents == nullptr)
      return XII_FAILURE;

    // Use Morton S. Mikkelsen's tangent calculation.
    SMikkTSpaceContext   context;
    SMikkTSpaceInterface functions;
    context.m_pUserData  = &mikkd;
    context.m_pInterface = &functions;

    functions.m_setTSpace            = nullptr;
    functions.m_getNumFaces          = MikkGetNumFaces;
    functions.m_getNumVerticesOfFace = MikkGetNumVerticesOfFace;

    if (md.Uses32BitIndices())
    {
      functions.m_getPosition    = MikkGetPosition32;
      functions.m_getNormal      = MikkGetNormal32;
      functions.m_getTexCoord    = MikkGetTexCoord32;
      functions.m_setTSpaceBasic = MikkSetTangents32;
    }
    else
    {
      functions.m_getPosition    = MikkGetPosition16;
      functions.m_getNormal      = MikkGetNormal16;
      functions.m_getTexCoord    = MikkGetTexCoord16;
      functions.m_setTSpaceBasic = MikkSetTangents16;
    }

    if (!genTangSpaceDefault(&context))
      return XII_FAILURE;

    return XII_SUCCESS;
  }

  xiiResult ImporterAssimp::PrepareOutputMesh()
  {
    if (m_Options.m_pMeshOutput == nullptr)
      return XII_SUCCESS;

    auto& mb = m_Options.m_pMeshOutput->MeshBufferDesc();

    if (m_Options.m_bImportSkinningData)
    {
      for (auto itMesh : m_MeshInstances)
      {
        for (const auto& mi : itMesh.Value())
        {
          SetMeshBindPoseData(*m_Options.m_pMeshOutput, mi.m_pMesh, mi.m_GlobalTransform);
        }
      }

      xiiUInt16 uiBoneCounter = 0;
      for (auto itBone : m_Options.m_pMeshOutput->m_Bones)
      {
        itBone.Value().m_uiBoneIndex = uiBoneCounter;
        ++uiBoneCounter;
      }
    }

    const bool b8BitBoneIndices = m_Options.m_pMeshOutput->m_Bones.GetCount() <= 255;

    // TODO: m_uiTotalMeshTriangles and m_uiTotalMeshVertices are too high if we skip non-skinned meshes

    StreamIndices streams;
    AllocateMeshStreams(mb, xiiArrayPtr<aiMesh*>(m_pScene->mMeshes, m_pScene->mNumMeshes), streams, m_uiTotalMeshVertices, m_uiTotalMeshTriangles, m_Options.m_MeshNormalsPrecision, m_Options.m_MeshTexCoordsPrecision, m_Options.m_bImportSkinningData, b8BitBoneIndices, m_Options.m_MeshBoneWeightPrecision);

    xiiUInt32 uiMeshPrevTriangleIdx = 0;
    xiiUInt32 uiMeshCurVertexIdx    = 0;
    xiiUInt32 uiMeshCurTriangleIdx  = 0;
    xiiUInt32 uiMeshCurSubmeshIdx   = 0;

    const bool bFlipTriangles = xiiGraphicsUtils::IsTriangleFlipRequired(m_Options.m_RootTransform);

    for (auto itMesh : m_MeshInstances)
    {
      const xiiUInt32 uiMaterialIdx = itMesh.Key();

      for (const auto& mi : itMesh.Value())
      {
        if (m_Options.m_bImportSkinningData && !mi.m_pMesh->HasBones())
        {
          // skip meshes that have no bones
          continue;
        }

        SetMeshVertexData(mb, mi.m_pMesh, mi.m_GlobalTransform, uiMeshCurVertexIdx, streams, m_Options.m_MeshNormalsPrecision, m_Options.m_MeshTexCoordsPrecision);

        if (m_Options.m_bImportSkinningData)
        {
          SetMeshBoneData(mb, *m_Options.m_pMeshOutput, m_Options.m_pMeshOutput->m_fMaxBoneVertexOffset, mi.m_pMesh, uiMeshCurVertexIdx, streams, b8BitBoneIndices, m_Options.m_MeshBoneWeightPrecision, m_Options.m_bNormalizeWeights);
        }

        SetMeshTriangleIndices(mb, mi.m_pMesh, uiMeshCurTriangleIdx, uiMeshCurVertexIdx, bFlipTriangles);

        uiMeshCurTriangleIdx += mi.m_pMesh->mNumFaces;
        uiMeshCurVertexIdx += mi.m_pMesh->mNumVertices;
      }

      if (uiMeshCurTriangleIdx - uiMeshPrevTriangleIdx == 0)
      {
        // skip empty submeshes
        continue;
      }

      if (uiMaterialIdx >= m_OutputMaterials.GetCount())
      {
        m_Options.m_pMeshOutput->SetMaterial(uiMeshCurSubmeshIdx, "");
      }
      else
      {
        m_OutputMaterials[uiMaterialIdx].m_iReferencedByMesh = static_cast<xiiInt32>(uiMeshCurSubmeshIdx);
        m_Options.m_pMeshOutput->SetMaterial(uiMeshCurSubmeshIdx, m_OutputMaterials[uiMaterialIdx].m_sName);
      }

      m_Options.m_pMeshOutput->AddSubMesh(uiMeshCurTriangleIdx - uiMeshPrevTriangleIdx, uiMeshPrevTriangleIdx, uiMeshCurSubmeshIdx);

      uiMeshPrevTriangleIdx = uiMeshCurTriangleIdx;
      ++uiMeshCurSubmeshIdx;
    }

    m_Options.m_pMeshOutput->ComputeBounds();

    return XII_SUCCESS;
  }
} // namespace xiiModelImporter2
