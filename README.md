# sequencerAbstraction
A lightweight Unreal Engine Editor-only module plugin that provides a minimal abstraction layer over Sequencer.

The goal is to simplify common Sequencer workflows (asset creation, track inspection, animation binding, etc.) without needing to interact directly with low-level MovieScene APIs.

## API Overview
| Category      | Function                                                                                                                                                                                                                 | Description                                                                                     |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------- |
| **Assets**    | `CreateLevelSequenceAsset(const FString& PackagePath, const FString& AssetName, FSequenceOpenResult& Result)`                                                                                                            | Creates a new `ULevelSequence` asset at the specified path.                                     |
| **Assets**    | `SaveAsset(UObject* Asset)`                                                                                                                                                                                              | Saves a loaded asset to disk.                                                                   |
| **Assets**    | `LoadLevelSequenceAsset(const FString& AssetPath)`                                                                                                                                                                       | Loads an existing `ULevelSequence` from a content path.                                         |
| **Sequencer** | `GetCurrentOpenedLevelSequence()`                                                                                                                                                                                        | Returns the currently opened sequence in Sequencer. Returns `nullptr` if none is open.          |
| **Sequencer** | `OpenLevelSequenceInSequencer(ULevelSequence* Sequence)`                                                                                                                                                                 | Opens a sequence in the Sequencer UI.                                                           |
| **Sequencer** | `GetAllTracksInCurrentSequence()`                                                                                                                                                                                        | Returns all tracks in the currently opened sequence (master and bound tracks).                  |
| **Content**   | `LoadSkeletalMesh(const FString& AssetPath)`                                                                                                                                                                             | Loads a `USkeletalMesh` from content path.                                                      |
| **Content**   | `LoadAnimSequence(const FString& AssetPath)`                                                                                                                                                                             | Loads a `UAnimSequence` from content path.                                                      |
| **Timing**    | `SetSequenceFrameRateFromAnimation(ULevelSequence* Sequence, UAnimSequence* Animation, bool bAlsoSetPlaybackRangeToAnim, FSequenceOpenResult& Result)`                                                                   | Sets sequence display rate based on animation sampling rate. Optionally adjusts playback range. |
| **Animation** | `AddAnimationToSequenceOnSkeletalMeshComponent(ULevelSequence* Sequence, UObject* WorldContextObject, USkeletalMesh* SkeletalMesh, UAnimSequence* Animation, const FName ActorLabelOrName, FSequenceOpenResult& Result)` | Binds actor, ensures skeletal mesh, creates animation track, and inserts animation section.     |

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
