Node %Time
{
  string %Category { "Input" }
  string %Color { "Green" }

  OutputPin %Global
  {
    string %Type { "float" }
    string %Inline { "MaterialTimes.x" }
    string %Tooltip { "Real time. Always at the same speed, unaffected by world simulation speed." }
  }

  OutputPin %World
  {
    string %Type { "float" }
    string %Inline { "MaterialTimes.y" }
    string %Tooltip { "World simulation time. Affected by simulation speed (slow-motion) and world paused state." }
  }
}

Node %UV
{
  string %Category { "Input" }
  string %Color { "Teal" }




  OutputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %Inline { "MaterialInput.TexCoord" }
    string %Tooltip { "The UV 0 texture coordinate." }
  }
}

Node %UV2
{
  string %Category { "Input" }
  string %Color { "Teal" }
  

  

  OutputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %Inline { "MaterialInput.TexCoord1" }
    string %Tooltip { "The UV 0 texture coordinate." }
  }
}

Node %UV_Scroll
{
  string %Category { "Input" }
  string %Color { "Green" }




  InputPin %Speed
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "1, 1" }
  }

  InputPin %Scale
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "1, 1" }
  }

  InputPin %Offset
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "0, 0" }
  }

  OutputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %Inline { "(MaterialInput.TexCoord * $in1 + $in2) + frac(MaterialTimes.y * $in0)" }
    string %Tooltip { "The scrolled UV 0 texture coordinate." }
  }
}

Node %VertexPosition
{
  string %Category { "Input" }
  string %Color { "Green" }

  OutputPin %Position
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "MaterialInput.LocalPosition" }
    string %Tooltip { "The interpolated object-space vertex position." }
  }
}

Node %VertexWorldPosition
{
  string %Category { "Input" }
  string %Color { "Green" }

  OutputPin %Position
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "MaterialInput.WorldPosition" }
    string %Tooltip { "The vertex world space position." }
  }
}

Node %VertexNormal
{
  string %Category { "Input" }
  string %Color { "Green" }




  OutputPin %Normal
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "MaterialInput.WorldNormal" }
    string %Tooltip { "The world-space vertex normal." }
  }
}

Node %VertexTangent
{
  string %Category { "Input" }
  string %Color { "Green" }




  OutputPin %Tangent
  {
    string %Type { "float3" }
    string %Color { "Grape" }
    string %Inline { "MaterialInput.WorldTangent" }
    string %Tooltip { "The world-space vertex tangent." }
  }
}

Node %VertexColor
{
  string %Category { "Input" }
  string %Color { "Green" }
  

  

  OutputPin %Color
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "MaterialInput.Color" }
    string %Tooltip { "The vertex color" }
  }
}

Node %VertexColor2
{
  string %Category { "Input" }
  string %Color { "Green" }
  

  

  OutputPin %Color
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "MaterialInput.Color" }
    string %Tooltip { "The second vertex color" }
  }
}

Node %InstanceData
{
  string %Category { "Input" }
  string %Color { "Green" }
  
  OutputPin %Color
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "xiiLoadSceneSurfaceMaterial(MaterialInput.MaterialIndex, MaterialBaseIndex).BaseColorOpacity" }
    string %Tooltip { "Native material-instance base color and opacity." }
  }
  
}

Node %Camera
{
  string %Category { "Input" }
  string %Color { "Green" }

  OutputPin %Position
  {
    string %Type { "float3" }
	string %Color { "Indigo" }
    string %Inline { "MaterialCameraPosition.xyz" }
    string %Tooltip { "Global camera position." }
  }

  OutputPin %Forwards
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "MaterialCameraForward.xyz" }
    string %Tooltip { "Forward direction vector of the camera." }
  }

  OutputPin %Right
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "MaterialCameraRight.xyz" }
    string %Tooltip { "Right direction vector of the camera." }
  }

  OutputPin %Up
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "MaterialCameraUp.xyz" }
    string %Tooltip { "Up direction vector of the camera." }
  }

  OutputPin %Exposure
  {
    string %Type { "float" }
    string %Inline { "MaterialTimes.z" }
    string %Tooltip { "Current exposure value of the camera." }
  }
}

