/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Pipeline/RenderData.h>

class xiiMeshResource;
struct xiiMsgExtractRenderData;

using xiiDecalComponentManager = xiiComponentManager<class xiiDecalComponent, xiiBlockStorageType::Compact>;

/// Renderer-facing packet for both projected deferred decals and mesh decals.
class XII_GRAPHICSCORE_DLL xiiDecalRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalRenderData, xiiRenderData);

public:
  xiiEnum<xiiDecalProjectionMode> m_Mode = xiiDecalProjectionMode::Projected; ///< Whether this is a projected deferred decal or a mesh decal, used to determine how the decal should be rendered.

  xiiDecalResourceHandle      m_hDecal;   ///< The decal resource containing the textures and properties for this decal, used for rendering.
  xiiDecalAtlasResourceHandle m_hAtlas;   ///< The decal atlas resource containing the atlas textures and properties for this decal, used for rendering.
  xiiHashedString             m_sAtlasId; ///< The atlas ID for this decal, used to look up the correct atlas entry in the decal atlas resource for rendering.

  xiiTexture2DResourceHandle m_hAlbedo;   ///< The albedo texture for this decal, used for rendering. This can either come from the decal resource directly, or from the atlas entry in the decal atlas resource if an atlas is used.
  xiiTexture2DResourceHandle m_hNormal;   ///< The normal texture for this decal, used for rendering. This can either come from the decal resource directly, or from the atlas entry in the decal atlas resource if an atlas is used.
  xiiTexture2DResourceHandle m_hMaterial; ///< The material texture for this decal, used for rendering. This can either come from the decal resource directly, or from the atlas entry in the decal atlas resource if an atlas is used.
  xiiTexture2DResourceHandle m_hEmissive; ///< The emissive texture for this decal, used for rendering. This can either come from the decal resource directly, or from the atlas entry in the decal atlas resource if an atlas is used.

  xiiMeshResourceHandle       m_hMesh;       ///< The mesh resource for this decal, used for rendering mesh decals. This is only valid if the projection mode is set to Mesh, and is ignored for projected deferred decals. The mesh resource contains the geometry and materials for the mesh decal, and is used to render the decal as a mesh in the world.
  xiiMeshBufferResourceHandle m_hMeshBuffer; ///< The mesh buffer resource for this decal, used for rendering mesh decals. This is only valid if the projection mode is set to Mesh, and is ignored for projected deferred decals. The mesh buffer resource contains the vertex and index buffers for the mesh decal, and is used to render the decal geometry in the world.

  xiiVec3 m_vExtents     = xiiVec3(1.0f, 1.0f, 0.25f);      ///< The extents of the decal in world space, used for rendering projected deferred decals. This is only valid if the projection mode is set to Projected, and is ignored for mesh decals. The extents define the size of the box used to project the decal onto the scene, with the center of the box being the position of the decal component in the world.
  xiiVec2 m_vUVOffset    = xiiVec2::MakeZero();             ///< The UV offset for the decal, used for rendering. This is added to the UV coordinates of the decal textures, and can be used to animate the decal or to adjust the placement of the textures on the decal geometry.
  xiiVec2 m_vUVScale     = xiiVec2(1.0f);                   ///< The UV scale for the decal, used for rendering. This is multiplied with the UV coordinates of the decal textures, and can be used to tile the decal textures across the decal geometry or to adjust the placement of the textures on the decal geometry.
  xiiVec4 m_vAtlasUVRect = xiiVec4(0.0f, 0.0f, 1.0f, 1.0f); ///< The UV rectangle for the atlas entry in the decal atlas resource, used for rendering. This is only valid if an atlas is used, and is ignored if no atlas is used. The UV rectangle defines the portion of the atlas textures that should be used for this decal, with (x,y) being the UV coordinates of the top-left corner of the rectangle and (z,w) being the width and height of the rectangle in UV space.

  xiiColor                         m_Tint = xiiColor::White; ///< The tint color for this decal, used for rendering. This is multiplied with the albedo texture of the decal, and can be used to change the color of the decal without modifying the texture itself.
  xiiBitflags<xiiDecalChannelMask> m_ChannelMask;            ///< The channel mask for this decal, used for rendering. This is used to specify which channels of the decal textures should be used for rendering, and can be used to optimize rendering by only using the necessary channels of the textures.

  float    m_fOpacity     = 1.0f; ///< The opacity of this decal, used for rendering. This is multiplied with the alpha channel of the albedo texture of the decal, and can be used to make the decal more transparent or more opaque without modifying the texture itself.
  float    m_fNormalBlend = 1.0f; ///< The normal blend factor for this decal, used for rendering. This is used to blend the normal map of the decal with the underlying surface normals, and can be used to adjust the strength of the normal map effect of the decal.
  float    m_fRoughness   = 0.5f; ///< The roughness of this decal, used for rendering. This is used to specify the roughness value for the material of the decal, and can be used to make the decal appear more glossy or more rough without modifying the material texture itself.
  float    m_fMetallic    = 0.0f; ///< The metallic of this decal, used for rendering. This is used to specify the metallic value for the material of the decal, and can be used to make the decal appear more metallic or more non-metallic without modifying the material texture itself.
  float    m_fEmissive    = 0.0f; ///< The emissive intensity of this decal, used for rendering. This is multiplied with the emissive texture of the decal, and can be used to make the decal appear more emissive or less emissive without modifying the texture itself.
  float    m_fFadeAngle   = 1.0f; ///< The fade angle for this decal, used for rendering. This is used to specify the angle at which the decal should start to fade out when viewed from a grazing angle, and can be used to reduce the visibility of the decal when viewed from sharp angles to improve visual quality and reduce artifacts.
  xiiUInt8 m_uiPriority   = 128U; ///< The priority of this decal, used for rendering. This is used to determine the rendering order of decals when multiple decals overlap, with higher priority decals being rendered on top of lower priority decals. The priority can be used to ensure that important decals are always visible, while less important decals can be faded out or occluded by higher priority decals.

  xiiUInt32 m_uiUniqueID       = 0U; ///< The unique ID for this decal, used for rendering. This is usually generated from the component ID of the decal component and is used to identify the decal in the renderer, e.g. for sorting or for applying per-decal shader properties.
  xiiUInt32 m_uiFirstPrimitive = 0U; ///< The index of the first primitive for this decal in the mesh buffer, used for rendering. This is only valid for mesh decals, and is ignored for projected deferred decals. The first primitive index is used to determine where in the mesh buffer the geometry for this decal starts, and is used together with the primitive count to render the correct portion of the mesh buffer for this decal.
  xiiUInt32 m_uiPrimitiveCount = 0U; ///< The number of primitives for this decal in the mesh buffer, used for rendering. This is only valid for mesh decals, and is ignored for projected deferred decals. The primitive count is used to determine how many primitives in the mesh buffer belong to this decal, and is used together with the first primitive index to render the correct portion of the mesh buffer for this decal.
  xiiUInt32 m_uiFirstMeshlet   = 0U; ///< The index of the first meshlet for this decal in the mesh buffer, used for rendering. This is only valid for mesh decals, and is ignored for projected deferred decals. The first meshlet index is used to determine where in the mesh buffer the meshlets for this decal start, and is used together with the meshlet count to render the correct portion of the mesh buffer for this decal when using meshlet-based rendering.
  xiiUInt32 m_uiMeshletCount   = 0U; ///< The number of meshlets for this decal in the mesh buffer, used for rendering. This is only valid for mesh decals, and is ignored for projected deferred decals. The meshlet count is used to determine how many meshlets in the mesh buffer belong to this decal, and is used together with the first meshlet index to render the correct portion of the mesh buffer for this decal when using meshlet-based rendering.
};

/// Places a decal projector or mesh decal into the render world.
class XII_GRAPHICSCORE_DLL xiiDecalComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDecalComponent, xiiRenderComponent, xiiDecalComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent
public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiDecalComponent

public:
  xiiDecalComponent();
  ~xiiDecalComponent();

  /// Sets the projection mode for this decal, used to determine how the decal should be rendered. This can be either projected deferred decals or mesh decals, and determines whether the decal is rendered as a projected box in the world or as a mesh with its own geometry.
  void SetProjectionMode(xiiEnum<xiiDecalProjectionMode> mode); // [property]

  /// Returns the projection mode for this decal, used to determine how the decal should be rendered. This can be either projected deferred decals or mesh decals, and determines whether the decal is rendered as a projected box in the world or as a mesh with its own geometry.
  xiiEnum<xiiDecalProjectionMode> GetProjectionMode() const; // [property]

  /// Sets the decal resource for this decal, used for rendering. The decal resource contains the textures and properties for this decal, and is used to render the decal in the world. If an atlas is used, the textures from the atlas entry in the decal atlas resource will be used instead of the textures from the decal resource, but the properties from the decal resource will still be used.
  void SetDecal(const xiiDecalResourceHandle& hDecal); // [property]

  /// Returns the decal resource for this decal, used for rendering. The decal resource contains the textures and properties for this decal, and is used to render the decal in the world. If an atlas is used, the textures from the atlas entry in the decal atlas resource will be used instead of the textures from the decal resource, but the properties from the decal resource will still be used.
  const xiiDecalResourceHandle& GetDecal() const; // [property]

  /// Sets the decal atlas resource for this decal, used for rendering. The decal atlas resource contains the atlas textures and properties for this decal, and is used to render the decal in the world. If an atlas is used, the textures from the atlas entry in the decal atlas resource will be used instead of the textures from the decal resource, but the properties from the decal resource will still be used.
  void SetAtlas(const xiiDecalAtlasResourceHandle& hAtlas); // [property]

  /// Returns the decal atlas resource for this decal, used for rendering. The decal atlas resource contains the atlas textures and properties for this decal, and is used to render the decal in the world. If an atlas is used, the textures from the atlas entry in the decal atlas resource will be used instead of the textures from the decal resource, but the properties from the decal resource will still be used.
  const xiiDecalAtlasResourceHandle& GetAtlas() const; // [property]

  /// Sets the atlas ID for this decal, used to look up the correct atlas entry in the decal atlas resource for rendering. The atlas ID is used to find the corresponding entry in the decal atlas resource, which contains the UV rectangle and other properties for this decal when using an atlas. If no atlas is used, this is ignored.
  void SetAtlasId(xiiStringView sAtlasId); // [property]

  /// Returns the atlas ID for this decal, used to look up the correct atlas entry in the decal atlas resource for rendering. The atlas ID is used to find the corresponding entry in the decal atlas resource, which contains the UV rectangle and other properties for this decal when using an atlas. If no atlas is used, this is ignored.
  xiiStringView GetAtlasId() const; // [property]

  /// Sets the mesh resource for this decal, used for rendering mesh decals. The mesh resource contains the geometry and materials for the mesh decal, and is used to render the decal as a mesh in the world. This is only valid if the projection mode is set to Mesh, and is ignored for projected deferred decals.
  void SetMesh(const xiiMeshResourceHandle& hMesh);

  /// Returns the mesh resource for this decal, used for rendering mesh decals. The mesh resource contains the geometry and materials for the mesh decal, and is used to render the decal as a mesh in the world. This is only valid if the projection mode is set to Mesh, and is ignored for projected deferred decals.
  const xiiMeshResourceHandle& GetMesh() const;

  /// Sets the extents of this decal in world space, used for rendering projected deferred decals. The extents define the size of the box used to project the decal onto the scene, with the center of the box being the position of the decal component in the world. This is only valid if the projection mode is set to Projected, and is ignored for mesh decals.
  void SetExtents(xiiVec3 vExtents); // [property]

  /// Returns the extents of this decal in world space, used for rendering projected deferred decals. The extents define the size of the box used to project the decal onto the scene, with the center of the box being the position of the decal component in the world. This is only valid if the projection mode is set to Projected, and is ignored for mesh decals.
  xiiVec3 GetExtents() const; // [property]

  /// Sets the UV offset for this decal, used for rendering. The UV offset is added to the UV coordinates of the decal textures, and can be used to animate the decal or to adjust the placement of the textures on the decal geometry.
  void SetUVOffset(xiiVec2 vOffset); // [property]

  /// Returns the UV offset for this decal, used for rendering. The UV offset is added to the UV coordinates of the decal textures, and can be used to animate the decal or to adjust the placement of the textures on the decal geometry.
  xiiVec2 GetUVOffset() const; // [property]

  /// Sets the UV scale for this decal, used for rendering. The UV scale is multiplied with the UV coordinates of the decal textures, and can be used to tile the decal textures across the decal geometry or to adjust the placement of the textures on the decal geometry.
  void SetUVScale(xiiVec2 vScale); // [property]

  /// Returns the UV scale for this decal, used for rendering. The UV scale is multiplied with the UV coordinates of the decal textures, and can be used to tile the decal textures across the decal geometry or to adjust the placement of the textures on the decal geometry.
  xiiVec2 GetUVScale() const; // [property]

  /// Sets the tint color for this decal, used for rendering. The tint color is multiplied with the albedo texture of the decal, and can be used to change the color of the decal without modifying the texture itself.
  void SetTint(xiiColor tint); // [property]

  /// Returns the tint color for this decal, used for rendering. The tint color is multiplied with the albedo texture of the decal, and can be used to change the color of the decal without modifying the texture itself.
  xiiColor GetTint() const; // [property]

  /// Sets the channel mask for this decal, used for rendering. The channel mask is used to specify which channels of the decal textures should be used for rendering, and can be used to optimize rendering by only using the necessary channels of the textures.
  void SetChannelMask(xiiBitflags<xiiDecalChannelMask> mask); // [property]

  /// Returns the channel mask for this decal, used for rendering. The channel mask is used to specify which channels of the decal textures should be used for rendering, and can be used to optimize rendering by only using the necessary channels of the textures.
  xiiBitflags<xiiDecalChannelMask> GetChannelMask() const; // [property]

  /// Sets the opacity of this decal, used for rendering. The opacity is multiplied with the alpha channel of the albedo texture of the decal, and can be used to make the decal more transparent or more opaque without modifying the texture itself.
  void SetOpacity(float fOpacity); // [property]

  /// Returns the opacity of this decal, used for rendering. The opacity is multiplied with the alpha channel of the albedo texture of the decal, and can be used to make the decal more transparent or more opaque without modifying the texture itself.
  float GetOpacity() const; // [property]

  /// Sets the normal blend factor for this decal, used for rendering. The normal blend factor is used to blend the normal map of the decal with the underlying surface normals, and can be used to adjust the strength of the normal map effect of the decal.
  void SetNormalBlend(float fNormalBlend); // [property]

  /// Returns the normal blend factor for this decal, used for rendering. The normal blend factor is used to blend the normal map of the decal with the underlying surface normals, and can be used to adjust the strength of the normal map effect of the decal.
  float GetNormalBlend() const; // [property]

  /// Sets the roughness of this decal, used for rendering. The roughness is used to specify the roughness value for the material of the decal, and can be used to make the decal appear more glossy or more rough without modifying the material texture itself.
  void SetRoughness(float fRoughness); // [property]

  /// Returns the roughness of this decal, used for rendering. The roughness is used to specify the roughness value for the material of the decal, and can be used to make the decal appear more glossy or more rough without modifying the material texture itself.
  float GetRoughness() const; // [property]

  /// Sets the metallic of this decal, used for rendering. The metallic is used to specify the metallic value for the material of the decal, and can be used to make the decal appear more metallic or more non-metallic without modifying the material texture itself.
  void SetMetallic(float fMetallic); // [property]

  /// Returns the metallic of this decal, used for rendering. The metallic is used to specify the metallic value for the material of the decal, and can be used to make the decal appear more metallic or more non-metallic without modifying the material texture itself.
  float GetMetallic() const; // [property]

  /// Sets the emissive intensity of this decal, used for rendering. The emissive intensity is multiplied with the emissive texture of the decal, and can be used to make the decal appear more emissive or less emissive without modifying the texture itself.
  void SetEmissive(float fEmissive); // [property]

  /// Returns the emissive intensity of this decal, used for rendering. The emissive intensity is multiplied with the emissive texture of the decal, and can be used to make the decal appear more emissive or less emissive without modifying the texture itself.
  float GetEmissive() const; // [property]

  /// Sets the priority of this decal, used for rendering. The priority is used to determine the rendering order of decals when multiple decals overlap, with higher priority decals being rendered on top of lower priority decals. The priority can be used to ensure that important decals are always visible, while less important decals can be faded out or occluded by higher priority decals.
  void SetPriority(xiiUInt8 uiPriority); // [property]

  /// Returns the priority of this decal, used for rendering. The priority is used to determine the rendering order of decals when multiple decals overlap, with higher priority decals being rendered on top of lower priority decals. The priority can be used to ensure that important decals are always visible, while less important decals can be faded out or occluded by higher priority decals.
  xiiUInt8 GetPriority() const; // [property]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  /// Fills the mesh range data for this decal render data based on the provided mesh resource, used for rendering mesh decals. This is used to determine the correct portion of the mesh buffer to render for this decal when using meshlet-based rendering.
  void FillMeshRange(xiiDecalRenderData& ref_renderData, const xiiMeshResource& mesh) const;

private:
  xiiEnum<xiiDecalProjectionMode> m_Mode = xiiDecalProjectionMode::Projected; ///< Whether this is a projected deferred decal or a mesh decal, used to determine how the decal should be rendered.

  xiiDecalResourceHandle      m_hDecal;   ///< The decal resource containing the textures and properties for this decal, used for rendering.
  xiiDecalAtlasResourceHandle m_hAtlas;   ///< The decal atlas resource containing the atlas textures and properties for this decal, used for rendering.
  xiiHashedString             m_sAtlasId; ///< The atlas ID for this decal, used to look up the correct atlas entry in the decal atlas resource for rendering.
  xiiMeshResourceHandle       m_hMesh;    ///< The mesh resource for this decal, used for rendering mesh decals. This is only valid if the projection mode is set to Mesh, and is ignored for projected deferred decals. The mesh resource contains the geometry and materials for the mesh decal, and is used to render the decal as a mesh in the world.

  xiiVec3  m_vExtents  = xiiVec3(1.0f, 1.0f, 0.25f); ///< The extents of the decal in world space, used for rendering projected deferred decals. This is only valid if the projection mode is set to Projected, and is ignored for mesh decals. The extents define the size of the box used to project the decal onto the scene, with the center of the box being the position of the decal component in the world.
  xiiVec2  m_vUVOffset = xiiVec2::MakeZero();        ///< The UV offset for the decal, used for rendering. This is added to the UV coordinates of the decal textures, and can be used to animate the decal or to adjust the placement of the textures on the decal geometry.
  xiiVec2  m_vUVScale  = xiiVec2(1.0f);              ///< The UV scale for the decal, used for rendering. This is multiplied with the UV coordinates of the decal textures, and can be used to tile the decal textures across the decal geometry or to adjust the placement of the textures on the decal geometry.
  xiiColor m_Tint      = xiiColor::White;            ///< The tint color for this decal, used for rendering. This is multiplied with the albedo texture of the decal, and can be used to change the color of the decal without modifying the texture itself.

  xiiBitflags<xiiDecalChannelMask> m_ChannelMask; ///< The channel mask for this decal, used for rendering. This is used to specify which channels of the decal textures should be used for rendering, and can be used to optimize rendering by only using the necessary channels of the textures.

  float    m_fOpacity     = 1.0f; ///< The opacity of this decal, used for rendering. This is multiplied with the alpha channel of the albedo texture of the decal, and can be used to make the decal more transparent or more opaque without modifying the texture itself.
  float    m_fNormalBlend = 1.0f; ///< The normal blend factor for this decal, used for rendering. This is used to blend the normal map of the decal with the underlying surface normals, and can be used to adjust the strength of the normal map effect of the decal.
  float    m_fRoughness   = 0.5f; ///< The roughness of this decal, used for rendering. This is used to specify the roughness value for the material of the decal, and can be used to make the decal appear more glossy or more rough without modifying the material texture itself.
  float    m_fMetallic    = 0.0f; ///< The metallic of this decal, used for rendering. This is used to specify the metallic value for the material of the decal, and can be used to make the decal appear more metallic or more non-metallic without modifying the material texture itself.
  float    m_fEmissive    = 0.0f; ///< The emissive intensity of this decal, used for rendering. This is multiplied with the emissive texture of the decal, and can be used to make the decal appear more emissive or less emissive without modifying the texture itself.
  xiiUInt8 m_uiPriority   = 128U; ///< The priority of this decal, used for rendering. This is used to determine the rendering order of decals when multiple decals overlap, with higher priority decals being rendered on top of lower priority decals. The priority can be used to ensure that important decals are always visible, while less important decals can be faded out or occluded by higher priority decals.
};
