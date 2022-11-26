#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <ModelImporter2/ImporterMagicaVoxel/ImporterMagicaVoxel.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Logging/Log.h>


#define OGT_VOX_IMPLEMENTATION
#include <ModelImporter2/ImporterMagicaVoxel/ogt_vox.h>

#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include <ModelImporter2/ImporterMagicaVoxel/ogt_voxel_meshify.h>


namespace xiiModelImporter2
{
  ImporterMagicaVoxel::ImporterMagicaVoxel()  = default;
  ImporterMagicaVoxel::~ImporterMagicaVoxel() = default;

  xiiResult ImporterMagicaVoxel::DoImport()
  {
    const char* szFileName = m_Options.m_sSourceFile;

    xiiDynamicArray<xiiUInt8> fileContent;
    fileContent.Reserve(1024 * 1024);

    // Read the whole file into memory since we map BSP data structures directly to memory content
    {
      xiiFileReader fileReader;

      if (fileReader.Open(szFileName, 1024 * 1024).Failed())
      {
        xiiLog::Error("Couldn't open '{}' for voxel import.", szFileName);
        return XII_FAILURE;
      }

      xiiUInt8 Temp[1024 * 4];

      while (xiiUInt64 uiRead = fileReader.ReadBytes(Temp, XII_ARRAY_SIZE(Temp)))
      {
        fileContent.PushBackRange(xiiArrayPtr<xiiUInt8>(Temp, (xiiUInt32)uiRead));
      }
    }

    if (fileContent.IsEmpty())
    {
      return XII_FAILURE;
    }

    const ogt_vox_scene* scene = ogt_vox_read_scene(fileContent.GetData(), fileContent.GetCount());
    if (!scene)
    {
      xiiLog::Error("Couldn't open '{}' for voxel import, read_scene failed.", szFileName);
      return XII_FAILURE;
    }

    XII_SCOPE_EXIT(ogt_vox_destroy_scene(scene));

    // Temp storage buffers to build the mesh streams out of
    xiiDynamicArray<xiiVec3> positions;
    positions.Reserve(4096);

    xiiDynamicArray<xiiVec3> normals;
    normals.Reserve(4096);

    xiiDynamicArray<xiiColorGammaUB> colors;
    colors.Reserve(4096);

    xiiDynamicArray<xiiUInt32> indices;
    indices.Reserve(8192);

    xiiUInt32 uiIndexOffset = 0;

    for (uint32_t modelIdx = 0; modelIdx < scene->num_models; ++modelIdx)
    {
      const ogt_vox_model* model = scene->models[modelIdx];

      ogt_voxel_meshify_context ctx;
      memset(&ctx, 0, sizeof(ctx));

      ogt_mesh* mesh = ogt_mesh_from_paletted_voxels_greedy(&ctx, model->voxel_data, model->size_x, model->size_y, model->size_z, (const ogt_mesh_rgba*)&scene->palette.color[0]);
      XII_SCOPE_EXIT(ogt_mesh_destroy(&ctx, mesh));

      if (!mesh)
      {
        xiiLog::Error("Couldn't generate mesh for voxels in file '{}'.", szFileName);
        return XII_FAILURE;
      }

      ogt_mesh_remove_duplicate_vertices(&ctx, mesh);

      // offset mesh vertices so that the center of the mesh (center of the voxel grid) is at (0,0,0)
      // also apply the root transform in the same go
      {
        const xiiVec3 originOffset = xiiVec3(-(float)(model->size_x >> 1), (float)(model->size_z >> 1), (float)(model->size_y >> 1));

        for (uint32_t i = 0; i < mesh->vertex_count; ++i)
        {
          xiiVec3 pos = xiiVec3(-mesh->vertices[i].pos.x, mesh->vertices[i].pos.z, mesh->vertices[i].pos.y);
          pos -= originOffset;
          positions.ExpandAndGetRef() = m_Options.m_RootTransform * pos;

          xiiVec3 norm              = xiiVec3(-mesh->vertices[i].normal.x, mesh->vertices[i].normal.z, mesh->vertices[i].normal.y);
          normals.ExpandAndGetRef() = m_Options.m_RootTransform.TransformDirection(norm);

          colors.ExpandAndGetRef() = xiiColorGammaUB(mesh->vertices[i].color.r, mesh->vertices[i].color.g, mesh->vertices[i].color.b, mesh->vertices[i].color.a);
        }

        for (uint32_t i = 0; i < mesh->index_count; ++i)
        {
          indices.PushBack(mesh->indices[i] + uiIndexOffset);
        }
      }

      uiIndexOffset += mesh->vertex_count;
    }


    xiiMeshBufferResourceDescriptor& mb = m_Options.m_pMeshOutput->MeshBufferDesc();

    const xiiUInt32 uiPosStream = mb.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);
    const xiiUInt32 uiNrmStream = mb.AddStream(xiiGALVertexAttributeSemantic::Normal, xiiMeshNormalPrecision::ToResourceFormatNormal(xiiMeshNormalPrecision::_10Bit));
    const xiiUInt32 uiColStream = mb.AddStream(xiiGALVertexAttributeSemantic::Color0, xiiGALResourceFormat::RGBAUByteNormalized);

    mb.AllocateStreams(positions.GetCount(), xiiGALPrimitiveTopology::Triangles, indices.GetCount() / 3);

    // Add triangles
    xiiUInt32 uiFinalTriIdx = 0;
    for (xiiUInt32 i = 0; i < indices.GetCount(); i += 3, ++uiFinalTriIdx)
    {
      mb.SetTriangleIndices(uiFinalTriIdx, indices[i + 1], indices[i + 0], indices[i + 2]);
    }

    for (xiiUInt32 i = 0; i < positions.GetCount(); ++i)
    {
      mb.SetVertexData(uiPosStream, i, positions[i]);

      xiiMeshBufferUtils::EncodeNormal(normals[i], mb.GetVertexData(uiNrmStream, i), xiiMeshNormalPrecision::_10Bit).IgnoreResult();

      mb.SetVertexData(uiColStream, i, colors[i]);
    }

    m_Options.m_pMeshOutput->SetMaterial(0, xiiMaterialResource::GetDefaultMaterialFileName(xiiMaterialResource::DefaultMaterialType::Lit));
    m_Options.m_pMeshOutput->AddSubMesh(indices.GetCount() / 3, 0, 0);
    m_Options.m_pMeshOutput->ComputeBounds();

    return XII_SUCCESS;
  }
} // namespace xiiModelImporter2
