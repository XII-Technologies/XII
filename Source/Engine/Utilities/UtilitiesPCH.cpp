#include <Utilities/UtilitiesPCH.h>

XII_STATICLINK_LIBRARY(Utilities)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(Utilities_DGML_Implementation_DGMLCreator);
  XII_STATICLINK_REFERENCE(Utilities_DataStructures_Implementation_DynamicOctree);
  XII_STATICLINK_REFERENCE(Utilities_DataStructures_Implementation_DynamicQuadtree);
  XII_STATICLINK_REFERENCE(Utilities_DataStructures_Implementation_ObjectSelection);
  XII_STATICLINK_REFERENCE(Utilities_FileFormats_Implementation_OBJLoader);
  XII_STATICLINK_REFERENCE(Utilities_GridAlgorithms_Implementation_Rasterization);
  XII_STATICLINK_REFERENCE(Utilities_PathFinding_Implementation_GridNavmesh);
}
