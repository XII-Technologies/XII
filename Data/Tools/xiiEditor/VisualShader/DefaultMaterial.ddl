Node %MaterialOutput
{
  string %Category { "Output" }
  string %Color { "Grape" }
  string %NodeType { "Main" }
  string %CodePermutations { "FORWARD_DIRECTIONAL_SHADOWS
FORWARD_ATMOSPHERE
FORWARD_REFRACTION
FORWARD_SPOT_SHADOWS
RENDER_PASS
BLEND_MODE
SHADING_MODE
TWO_SIDED" }
  string %CheckPermutations { "RENDER_PASS=RENDER_PASS_FORWARD
BLEND_MODE=BLEND_MODE_OPAQUE
SHADING_MODE=SHADING_MODE_LIT
TWO_SIDED=FALSE" }
  string %CodeRenderStates { "
#include <Shaders/Materials/NativeMaterialState.h>
" }
  string %CodeVertexShader { "
#define XII_SCENE_WORLD_ATTRIBUTES
#define XII_SCENE_MATERIAL_ATTRIBUTES
#define XII_SCENE_SURFACE_FLAGS
#include <Shaders/Pipeline/GpuSceneSurfaceVertex.h>
#include <Shaders/Common/VisualShaderUtil.h>
#include <Shaders/Common/MaterialGraphUtilities.h>
#include <Shaders/Materials/NativeMaterialViewConstants.h>
#define XII_SCENE_CUSTOM_VERTEX_ENTRY
#include <Shaders/Pipeline/GpuSceneSurfaceIndexed.h>
static GBufferVertex MaterialInput;
GBufferVertex main(uint vertexIndex : SV_VertexID, uint visibleIndex : SV_InstanceID)
{
  MaterialInput = xiiLoadIndexedSceneSurfaceVertex(vertexIndex, visibleIndex);
#if INPUT_PIN_10_CONNECTED || INPUT_PIN_11_CONNECTED
  const GpuSceneInstance instance = g_SceneInstances[MaterialInput.InstanceIndex];
  const float3 offset = mul(instance.GlobalTransform, float4(ToFloat3($in10), 0)).xyz + ToFloat3($in11);
  MaterialInput.WorldPosition += offset;
  MaterialInput.Position = mul(ViewProjectionMatrix, float4(MaterialInput.WorldPosition, 1));
  MaterialInput.PreviousClipPosition += mul(PreviousViewProjectionMatrix, float4(offset, 0));
#endif
  return MaterialInput;
}
" }
  string %CodeMaterialCB { "FLOAT1(MaskThreshold);" }
  string %CodeMaterialParams { "
Permutation BLEND_MODE;
Permutation SHADING_MODE;
Permutation TWO_SIDED;
float MaskThreshold @Default($prop0);
" }
  string %CodePixelIncludes { "
#include <Shaders/Materials/NativeMaterialSurface.h>
#include <Shaders/Common/VisualShaderUtil.h>
static GBufferVertex MaterialInput;
" }
  string %CodePixelBody { "
xiiMaterialSurfaceOutput main(GBufferVertex input, bool frontFace : SV_IsFrontFace)
{
  MaterialInput = input;
  const xiiGpuSurfaceMaterial nativeMaterial = xiiLoadSceneSurfaceMaterial(input.MaterialIndex, MaterialBaseIndex);
#if TWO_SIDED != TRUE
  if (!frontFace) discard;
#endif
  xiiMaterialSurface surface;
  surface.BaseColor = float4(ToColor3($in0), saturate(ToFloat1($in5)));
  surface.AlphaCutoff = MaskThreshold;
#if BLEND_MODE == BLEND_MODE_MASKED
  clip(surface.BaseColor.a - MaskThreshold);
#endif
  surface.Normal = xiiMaterialTangentToWorld(input, ToFloat3($in1)) * (frontFace ? 1 : -1);
  surface.Metallic = saturate(ToFloat1($in2));
  surface.Reflectance = saturate(ToFloat1($in3));
  surface.Roughness = saturate(ToFloat1($in4));
  surface.Emissive = ToColor3($in6);
  surface.Occlusion = saturate(ToFloat1($in7));
  surface.Transmission = saturate(1 - ToColor4($in8).a);
  surface.Unlit = SHADING_MODE == SHADING_MODE_FULLBRIGHT;
  return xiiWriteMaterialSurface(input, surface);
}
" }
  Property %MaskThreshold
  {
    string %Type { "float" }
    string %DefaultValue { "0.25" }
  }

  Property %ApplyFog
  {
    string %Type { "bool" }
    string %DefaultValue { "true" }
  }

  // Pin 0
  InputPin %BaseColor
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1" }
  }

  // Pin 1
  InputPin %Normal
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %DefaultValue { "float3(0, 0, 1)" }
    string %DefineWhenUsingDefaultValue { "USE_NORMAL" }
    string %Tooltip { "Surface normal in tangent space." }
  }

  // Pin 2
  InputPin %Metallic
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  // Pin 3
  InputPin %Reflectance
  {
    string %Type { "float" }
    string %Color { "Yellow" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }

  // Pin 4
  InputPin %Roughness
  {
    string %Type { "float" }
    string %Color { "Orange" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }

  // Pin 5
  InputPin %Opacity
  {
    string %Type { "float" }
    string %Color { "Red" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  // Pin 6
  InputPin %Emissive
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  // Pin 7
  InputPin %Occlusion
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  // Pin 8
  InputPin %RefractionColor
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "0, 0, 0, 1" }
  }

  // Pin 9
  InputPin %SubsurfaceColor
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "0, 0, 0" }
  }

  // Pin 10
  InputPin %LocalPosOffset
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %DefaultValue { "0" }
  }

  // Pin 11
  InputPin %GlobalPosOffset
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %DefaultValue { "0" }
  }
}
