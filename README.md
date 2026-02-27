# sequencerAbstraction
A lightweight Unreal Engine Editor-only module plugin that provides a minimal abstraction layer over Sequencer.

The goal is to simplify common Sequencer workflows (asset creation, track inspection, animation binding, etc.) without needing to interact directly with low-level MovieScene APIs.

## API Overview
| Category            | Function                                                                                                                                                                                                 | Description                                                                                                                     |
| ------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| **Assets**          | `CreateLevelSequenceAsset(const FString& PackagePath, const FString& AssetName, FSequenceOpenResult& Result)`                                                                                            | Creates a new `ULevelSequence` asset at the specified content path.                                                             |
| **Assets**          | `LoadLevelSequenceAsset(const FString& AssetPath)`                                                                                                                                                       | Loads an existing `ULevelSequence` from a content path.                                                                         |
| **Assets**          | `SaveAsset(UObject* Asset)`                                                                                                                                                                              | Saves a loaded asset to disk.                                                                                                   |
| **Sequencer**       | `GetCurrentOpenedLevelSequence()`                                                                                                                                                                        | Returns the currently opened sequence in Sequencer. Returns `nullptr` if none is open.                                          |
| **Sequencer**       | `OpenLevelSequenceInSequencer(ULevelSequence* Sequence)`                                                                                                                                                 | Opens the provided sequence in the Sequencer editor UI.                                                                         |
| **Sequencer**       | `GetAllTracksInCurrentSequence()`                                                                                                                                                                        | Returns all master and object-bound tracks in the currently opened sequence as `FSequenceTrackInfo` (including their sections). |
| **Sequencer**       | `AddSkeletalMeshToOpenSequenceFromPath(UObject* WorldContextObject, const FString& SkeletalMeshAssetPath, const FName SpawnedActorLabel, FSequenceOpenResult& Result)`                                   | Spawns a skeletal mesh actor from an asset path and binds it to the currently opened sequence. Returns its binding `FGuid`.     |
| **Content**         | `LoadSkeletalMesh(const FString& AssetPath)`                                                                                                                                                             | Loads a `USkeletalMesh` from a content path.                                                                                    |
| **Content**         | `LoadAnimSequence(const FString& AssetPath)`                                                                                                                                                             | Loads a `UAnimSequence` from a content path.                                                                                    |
| **Content**         | `LoadControlRigClass(const FString& AssetPath)`                                                                                                                                                          | Loads a `UControlRig` generated class (`TSubclassOf<UControlRig>`) from a Control Rig asset path.                               |
| **Tracks**          | `RemoveTrackInSequenceByTrackPath(ULevelSequence* Sequence, const FString& TrackPath, FSequenceOpenResult& Result)`                                                                                      | Removes a specific track using its serialized track path key.                                                                   |
| **Tracks**          | `RemoveTracksInSequenceByBindingGuid(ULevelSequence* Sequence, const FGuid& BindingGuid, const FString& OptionalTrackClassName, FSequenceOpenResult& Result)`                                            | Removes all tracks for a binding, optionally filtered by track class name.                                                      |
| **Sections**        | `RemoveAnimSection(ULevelSequence* Sequence, UMovieSceneSkeletalAnimationSection* Section, FSequenceOpenResult& Result)`                                                                                 | Removes a specific skeletal animation section from its track.                                                                   |
| **Sections**        | `RemoveAnimSectionsByAnimSequence(ULevelSequence* Sequence, UAnimSequenceBase* Animation, int32 MaxToRemove, FSequenceOpenResult& Result, const FGuid& BindingGuid)`                                     | Removes animation sections that reference the specified animation asset, optionally limited by binding and count.               |
| **Sections**        | `MoveAnimationSectionStartTo(ULevelSequence* Sequence, UMovieSceneSection* Section, int32 NewStartFrame, FSequenceOpenResult& Result)`                                                                   | Moves (translates) a section so its start frame matches the specified frame while preserving duration.                          |
| **Sections**        | `MoveAnimationSectionEndTo(ULevelSequence* Sequence, UMovieSceneSection* Section, int32 NewEndFrameInclusive, FSequenceOpenResult& Result)`                                                              | Moves (translates) a section so its end frame matches the specified frame while preserving duration.                            |
| **Timing**          | `SetSequenceFrameRateFromAnimation(ULevelSequence* Sequence, UAnimSequence* Animation, bool bAlsoSetPlaybackRangeToAnim, FSequenceOpenResult& Result)`                                                   | Sets the sequence display rate from the animation’s sampling rate. Optionally updates playback range to match animation length. |
| **Timing**          | `SetSequencePlaybackRange(ULevelSequence* Sequence, int32 StartFrame, int32 EndFrame)`                                                                                                                   | Sets the playback range of the sequence in display frames.                                                                      |
| **Timing**          | `GetSequencePlaybackRange(ULevelSequence* Sequence, int32& OutStartFrame, int32& OutEndFrame)`                                                                                                           | Retrieves the current playback range in display frames.                                                                         |
| **Timing (Legacy)** | `MoveAnimationSectionStartTo_Legacy(UMovieSceneSkeletalAnimationSection* Section, int32 NewStartFrame)`                                                                                                  | Legacy start-move function operating directly on a skeletal animation section (no sequence context).                            |
| **Timing (Legacy)** | `MoveAnimationSectionEndTo_Legacy(UMovieSceneSkeletalAnimationSection* Section, int32 NewEndFrame)`                                                                                                      | Legacy end-move function operating directly on a skeletal animation section (no sequence context).                              |
| **Animation**       | `AddAnimSectionToBinding(ULevelSequence* Sequence, const FGuid& BindingGuid, UAnimSequence* Animation, int32 StartFrame, int32 RowIndex, bool bAllowOverlapSameRow, FSequenceOpenResult& Result)`        | Adds a skeletal animation section to the binding’s animation track. Supports automatic row selection and overlap handling.      |
| **ControlRig**      | `AddRigToBinding(ULevelSequence* Sequence, UObject* WorldContextObject, const FGuid& BindingGuid, TSubclassOf<UControlRig> ControlRigClass, bool bLayered, FSequenceOpenResult& Result)`                 | Adds a Control Rig track to an existing binding. Supports layered rigs.                                                         |
| **Bake**            | `BakeBindingToAnimSequence(ULevelSequence* Sequence, UObject* WorldContextObject, const FGuid& BindingGuid, const FString& TargetPackagePath, const FString& NewAssetName, FSequenceOpenResult& Result)` | Bakes the specified binding to a new `UAnimSequence` asset at the target package path.                                          |



## Data Structures
FSequenceTrackInfo

Used by GetAllTracksInCurrentSequence().
```cpp
struct FSequenceTrackInfo
{
    FString DisplayName;
    FString TrackType;
    FString ObjectBindingPath;
    FGuid   BindingGuid;
    FString TrackPath;
    bool    bIsMasterTrack;
};
```

### Example workflow in C++
```cpp
ULevelSequence* Seq = USequencerAbstractionBPLibrary::GetCurrentOpenedLevelSequence();

if (!Seq)
{
    FSequenceOpenResult Result;
    Seq = USequencerAbstractionBPLibrary::CreateLevelSequenceAsset(
        "/Game/Cinematics",
        "NewSequence",
        Result
    );

    USequencerAbstractionBPLibrary::OpenLevelSequenceInSequencer(Seq);
}
```
