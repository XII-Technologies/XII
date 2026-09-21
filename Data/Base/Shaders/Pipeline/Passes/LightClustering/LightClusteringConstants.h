/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

#define XII_MAX_LIGHTS_PER_CLUSTER 128U
#define XII_CLUSTER_TILE_SIZE      16U
#define XII_CLUSTER_Z_SLICES       24U

/// Constants shared by ClusterGridBuild and LightListBuild passes.
///
/// Bound at slot b3 for both cluster-setup dispatch and light-list dispatch.
DECLARE_CONSTANT_BUFFER_AUTO(xiiLightClusteringConstants)
{
  UINT1(ClusterCountX);   ///< Number of clusters along screen X.
  UINT1(ClusterCountY);   ///< Number of clusters along screen Y.
  UINT1(ClusterCountZ);   ///< Number of depth slices.
  UINT1(TotalClusters);   ///< ClusterCountX * ClusterCountY * ClusterCountZ.
  FLOAT1(NearPlane);      ///< Camera near plane (metres).
  FLOAT1(FarPlane);       ///< Camera far plane (metres).
  FLOAT1(LogFarOverNear); ///< Precomputed log2(Far / Near) for Z-slice lookup.
  FLOAT1(Padding1);
  UINT1(TilePixelsX);         ///< Screen pixels per cluster tile in X.
  UINT1(TilePixelsY);         ///< Screen pixels per cluster tile in Y.
  UINT1(MaxLightsPerCluster); ///< Maximum number of lights stored per cluster.
  UINT1(ActiveLightCount);    ///< Total number of active lights to assign.
};
