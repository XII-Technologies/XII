/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Utilities/UtilitiesPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Utilities/FileFormats/OBJLoader.h>

xiiOBJLoader::FaceVertex::FaceVertex()
{
  m_uiPositionID = 0;
  m_uiNormalID   = 0;
  m_uiTexCoordID = 0;
}

xiiOBJLoader::Face::Face()
{
  m_uiMaterialID = 0;
}

void xiiOBJLoader::Clear()
{
  m_Positions.Clear();
  m_Normals.Clear();
  m_TexCoords.Clear();
  m_Faces.Clear();
  m_Materials.Clear();
}

static xiiStringView ReadLine(xiiStringView& ref_sPos)
{
  while (ref_sPos.GetCharacter() != '\0' && xiiStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  const char* szStart = ref_sPos.GetStartPointer();

  while (ref_sPos.GetCharacter() != '\0' && ref_sPos.GetCharacter() != '\r' && ref_sPos.GetCharacter() != '\n')
    ++ref_sPos;

  const char* szEnd = ref_sPos.GetStartPointer();

  while (ref_sPos.GetCharacter() != '\0' && xiiStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  return xiiStringView(szStart, szEnd);
}

static xiiStringView ReadString(xiiStringView& ref_sPos)
{
  while (ref_sPos.GetCharacter() != '\0' && xiiStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  const char* szStart = ref_sPos.GetStartPointer();

  while (ref_sPos.GetCharacter() != '\0' && !xiiStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  const char* szEnd = ref_sPos.GetStartPointer();

  while (ref_sPos.GetCharacter() != '\0' && xiiStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  return xiiStringView(szStart, szEnd);
}

static bool SkipSlash(xiiStringView& ref_sPos)
{
  if (ref_sPos.GetCharacter() != '/')
    return false;

  ++ref_sPos;

  return (ref_sPos.GetCharacter() != ' ' && ref_sPos.GetCharacter() != '\t');
}

xiiResult xiiOBJLoader::LoadOBJ(const char* szFile, bool bIgnoreMaterials)
{
  xiiFileReader File;
  if (File.Open(szFile).Failed())
    return XII_FAILURE;

  xiiString sContent;
  sContent.ReadAll(File);

  // which data has been found in the file
  bool bContainsTexCoords = false;
  bool bContainsNormals   = false;

  xiiUInt32 uiCurMaterial = 0xFFFFFFFF;

  xiiStringView sText = sContent;

  xiiUInt32 uiPositionOffset = m_Positions.GetCount();
  xiiUInt32 uiNormalOffset   = m_Normals.GetCount();
  xiiUInt32 uiTexCoordOffset = m_TexCoords.GetCount();

  while (sText.IsValid())
  {
    xiiStringView       sLine  = ReadLine(sText);
    const xiiStringView sFirst = ReadString(sLine);

    if (sFirst.IsEqual_NoCase("v")) // line declares a vertex
    {
      xiiVec3 v(0.0f);
      xiiConversionUtils::ExtractFloatsFromString(sLine, 3, &v.x);

      m_Positions.PushBack(v);
    }
    else if (sFirst.IsEqual_NoCase("vt")) // line declares a texture coordinate
    {
      bContainsTexCoords = true;

      xiiVec3 v(0.0f);
      xiiConversionUtils::ExtractFloatsFromString(sLine, 3, &v.x); // reads up to three texture-coordinates

      m_TexCoords.PushBack(v);
    }
    else if (sFirst.IsEqual_NoCase("vn")) // line declares a normal
    {
      bContainsNormals = true;

      xiiVec3 v(0.0f);
      xiiConversionUtils::ExtractFloatsFromString(sLine, 3, &v.x);
      v.Normalize(); // make sure normals are indeed normalized

      m_Normals.PushBack(v);
    }
    else if (sFirst.IsEqual_NoCase("f")) // line declares a face
    {
      Face face;
      face.m_uiMaterialID = uiCurMaterial;

      const char* szCurPos;

      // loop through all vertices, that are found
      while (sLine.IsValid())
      {
        xiiInt32 id;

        // read the position index
        if (xiiConversionUtils::StringToInt(sLine, id, &szCurPos).Failed())
          break; // nothing found, face-declaration is finished

        sLine.SetStartPosition(szCurPos);

        FaceVertex Vertex;
        Vertex.m_uiPositionID = uiPositionOffset + id - 1; // OBJ indices start at 1, so decrement them to start at 0

        // tex-coords were declared, so they will be used in the faces
        if (bContainsTexCoords)
        {
          if (!SkipSlash(sLine))
            break;

          if (xiiConversionUtils::StringToInt(sLine, id, &szCurPos).Failed())
            break;

          sLine.SetStartPosition(szCurPos);

          Vertex.m_uiTexCoordID = uiTexCoordOffset + id - 1; // OBJ indices start at 1, so decrement them to start at 0
        }

        // normals were declared, so they will be used in the faces
        if (bContainsNormals)
        {
          if (!SkipSlash(sLine))
            break;

          if (xiiConversionUtils::StringToInt(sLine, id, &szCurPos).Failed())
            break;

          sLine.SetStartPosition(szCurPos);

          Vertex.m_uiNormalID = uiNormalOffset + id - 1; // OBJ indices start at 1, so decrement them to start at 0
        }

        // stores the next vertex of the face
        face.m_Vertices.PushBack(Vertex);
      }

      // only allow faces with at least 3 vertices
      if (face.m_Vertices.GetCount() >= 3)
      {
        xiiVec3 v1, v2, v3;
        v1 = m_Positions[face.m_Vertices[0].m_uiPositionID];
        v2 = m_Positions[face.m_Vertices[1].m_uiPositionID];
        v3 = m_Positions[face.m_Vertices[2].m_uiPositionID];

        face.m_vNormal.CalculateNormal(v1, v2, v3).IgnoreResult();

        // done reading the face, store it
        m_Faces.PushBack(face);
      }
    }
    else if (sFirst.IsEqual_NoCase("usemtl")) // next material to be used for the following faces
    {
      if (bIgnoreMaterials)
        uiCurMaterial = 0xFFFFFFFF;
      else
      {
        // look-up the ID of this material

        bool bExisted = false;
        auto mat      = m_Materials.FindOrAdd(sLine, &bExisted).Value();

        if (!bExisted)
          mat.m_uiMaterialID = m_Materials.GetCount() - 1;

        uiCurMaterial = mat.m_uiMaterialID;
      }
    }
  }

  return XII_SUCCESS;
}

void xiiOBJLoader::SortFacesByMaterial()
{
  // sort all faces by material-ID
  m_Faces.Sort();
}

void xiiOBJLoader::ComputeTangentSpaceVectors()
{
  // cannot compute tangents without texture-coordinates
  if (!HasTextureCoordinates())
    return;

  for (xiiUInt32 f = 0; f < m_Faces.GetCount(); ++f)
  {
    Face& face = m_Faces[f];

    const xiiVec3 p1 = m_Positions[face.m_Vertices[0].m_uiPositionID];
    const xiiVec3 p2 = m_Positions[face.m_Vertices[1].m_uiPositionID];
    const xiiVec3 p3 = m_Positions[face.m_Vertices[2].m_uiPositionID];

    const xiiVec3 tc1 = m_TexCoords[face.m_Vertices[0].m_uiTexCoordID];
    const xiiVec3 tc2 = m_TexCoords[face.m_Vertices[1].m_uiTexCoordID];
    const xiiVec3 tc3 = m_TexCoords[face.m_Vertices[2].m_uiTexCoordID];

    xiiVec3 v2v1 = p2 - p1;
    xiiVec3 v3v1 = p3 - p1;

    float c2c1_T = tc2.x - tc1.x;
    float c2c1_B = tc2.y - tc1.y;

    float c3c1_T = tc3.x - tc1.x;
    float c3c1_B = tc3.y - tc1.y;

    float fDenominator = c2c1_T * c3c1_B - c3c1_T * c2c1_B;

    float fScale1 = 1.0f / fDenominator;

    xiiVec3 T, B;
    T = xiiVec3(
      (c3c1_B * v2v1.x - c2c1_B * v3v1.x) * fScale1, (c3c1_B * v2v1.y - c2c1_B * v3v1.y) * fScale1, (c3c1_B * v2v1.z - c2c1_B * v3v1.z) * fScale1);

    B = xiiVec3(
      (-c3c1_T * v2v1.x + c2c1_T * v3v1.x) * fScale1, (-c3c1_T * v2v1.y + c2c1_T * v3v1.y) * fScale1, (-c3c1_T * v2v1.z + c2c1_T * v3v1.z) * fScale1);

    T.Normalize();
    B.Normalize();

    face.m_vTangent   = T;
    face.m_vBiTangent = face.m_vNormal.CrossRH(face.m_vTangent).GetNormalized();
  }
}

xiiResult xiiOBJLoader::LoadMTL(const char* szFile, const char* szMaterialBasePath)
{
  xiiFileReader File;
  if (File.Open(szFile).Failed())
    return XII_FAILURE;

  xiiString sContent;
  sContent.ReadAll(File);

  xiiStringView sText = sContent;

  xiiString        sCurMatName;
  xiiStringBuilder sTemp;

  while (sText.IsValid())
  {
    xiiStringView       sLine  = ReadLine(sText);
    const xiiStringView sFirst = ReadString(sLine);

    if (sFirst.IsEqual_NoCase("newmtl")) // declares a new material with a given name
    {
      sCurMatName = sLine;

      bool bExisted = false;
      auto mat      = m_Materials.FindOrAdd(sCurMatName, &bExisted).Value();

      if (!bExisted)
      {
        mat.m_uiMaterialID = m_Materials.GetCount() - 1;
      }
    }
    else if (sFirst.IsEqual_NoCase("map_Kd"))
    {
      sTemp = szMaterialBasePath;
      sTemp.AppendPath(sLine);

      m_Materials[sCurMatName].m_sDiffuseTexture = sTemp;
    }
  }

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(Utilities, Utilities_FileFormats_Implementation_OBJLoader);
