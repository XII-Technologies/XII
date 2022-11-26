#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Event generated on mapping changes.
/// \sa xiiReflectionProbeMapping::m_Events
struct xiiReflectionProbeMappingEvent
{
  enum class Type
  {
    ProbeMapped,          ///< The given probe was mapped to the atlas.
    ProbeUnmapped,        ///<  The given probe was unmapped from the atlas.
    ProbeUpdateRequested, ///< The given probe needs to be updated after which xiiReflectionProbeMapping::ProbeUpdateFinished must be called.
  };

  xiiReflectionProbeId m_Id;
  Type                 m_Type;
};

/// \brief This class creates a reflection probe atlas and controls the mapping of added probes to the available atlas indices.
class xiiReflectionProbeMapping
{
public:
  /// \brief Creates a reflection probe atlas and mapping of the given size.
  /// \param uiAtlasSize How many probes the atlas can contain.
  xiiReflectionProbeMapping(xiiUInt32 uiAtlasSize);
  ~xiiReflectionProbeMapping();

  /// \name Probe management
  ///@{

  /// \brief Adds a probe that will be considered for mapping into the atlas.
  void AddProbe(xiiReflectionProbeId probe, xiiBitflags<xiiProbeFlags> flags);

  /// \brief Marks previously added probe as dirty and potentially changes its flags.
  void UpdateProbe(xiiReflectionProbeId probe, xiiBitflags<xiiProbeFlags> flags);

  /// \brief Should be called once a requested xiiReflectionProbeMappingEvent::Type::ProbeUpdateRequested event has been completed.
  /// \param probe The probe that has finished its update.
  void ProbeUpdateFinished(xiiReflectionProbeId probe);

  /// \brief Removes a probe. If the probe was mapped, xiiReflectionProbeMappingEvent::Type::ProbeUnmapped will be fired when calling this function.
  void RemoveProbe(xiiReflectionProbeId probe);

  ///@}
  /// \name Render helpers
  ///@{

  /// \brief Returns the index at which a given probe is mapped.
  /// \param probe The probe that is being queried.
  /// \param bForExtraction If set, returns whether the index can be used for using the probe during rendering. If the probe was just mapped but not updated yet, -1 will be returned for bForExtraction = true but a valid index for bForExtraction = false so that the index can be rendered into.
  /// \return Returns the mapped index in the atlas or -1 of the probe is not mapped.
  xiiInt32 GetReflectionIndex(xiiReflectionProbeId probe, bool bForExtraction = false) const;

  /// \brief Returns the atlas texture.
  /// \return The texture handle of the cube map atlas.
  xiiGALTextureHandle GetTexture() const { return m_hReflectionSpecularTexture; }

  ///@}
  /// \name Compute atlas mapping
  ///@{

  /// \brief Should be called in the PreExtraction phase. This will reset all probe weights.
  void PreExtraction();

  /// \brief Adds weight to a probe. Should be called during extraction of the probe. The mapping will map the probes with the highest weights in the atlas over time. This can be called multiple times in a frame for a probe if it is visible in multiple views. The maximum weight is then taken.
  void AddWeight(xiiReflectionProbeId probe, float fPriority);

  /// \brief Should be called in the PostExtraction phase. This will compute the best probe mapping and potentially fire xiiReflectionProbeMappingEvent events to map / unmap or request updates of probes.
  void PostExtraction();

  ///@}

public:
  xiiEvent<const xiiReflectionProbeMappingEvent&> m_Events;

private:
  struct xiiProbeMappingFlags
  {
    typedef xiiUInt8 StorageType;

    enum Enum
    {
      SkyLight         = xiiProbeFlags::SkyLight,
      HasCustomCubeMap = xiiProbeFlags::HasCustomCubeMap,
      Sphere           = xiiProbeFlags::Sphere,
      Box              = xiiProbeFlags::Box,
      Dynamic          = xiiProbeFlags::Dynamic,
      Dirty            = XII_BIT(5),
      Usable           = XII_BIT(6),
      Default          = 0
    };

    struct Bits
    {
      StorageType SkyLight : 1;
      StorageType HasCustomCubeMap : 1;
      StorageType Sphere : 1;
      StorageType Box : 1;
      StorageType Dynamic : 1;
      StorageType Dirty : 1;
      StorageType Usable : 1;
    };
  };

  //XII_DECLARE_FLAGS_OPERATORS(xiiProbeMappingFlags);

  struct SortedProbes
  {
    XII_DECLARE_POD_TYPE();

    XII_ALWAYS_INLINE bool operator<(const SortedProbes& other) const
    {
      if (m_fPriority > other.m_fPriority) // we want to sort descending (higher priority first)
        return true;

      return m_uiIndex < other.m_uiIndex;
    }

    xiiReflectionProbeId m_uiIndex;
    float                m_fPriority = 0.0f;
  };

  struct ProbeDataInternal
  {
    xiiBitflags<xiiProbeMappingFlags> m_Flags;
    xiiInt32                          m_uiReflectionIndex = -1;
    float                             m_fPriority         = 0.0f;
    xiiReflectionProbeId              m_id;
  };

private:
  void MapProbe(xiiReflectionProbeId id, xiiInt32 iReflectionIndex);
  void UnmapProbe(xiiReflectionProbeId id);

private:
  xiiDynamicArray<ProbeDataInternal> m_RegisteredProbes;
  xiiReflectionProbeId               m_SkyLight;

  xiiUInt32                             m_uiAtlasSize = 32;
  xiiDynamicArray<xiiReflectionProbeId> m_MappedCubes;

  // GPU Data
  xiiGALTextureHandle m_hReflectionSpecularTexture;

  // Cleared every frame:
  xiiDynamicArray<SortedProbes> m_SortedProbes;     // All probes exiting in the scene, sorted by priority.
  xiiDynamicArray<SortedProbes> m_ActiveProbes;     // Probes that are currently mapped in the atlas.
  xiiDynamicArray<xiiInt32>     m_UnusedProbeSlots; // Probe slots are are currently unused in the atlas.
  xiiDynamicArray<SortedProbes> m_AddProbes;        // Probes that should be added to the atlas
};
