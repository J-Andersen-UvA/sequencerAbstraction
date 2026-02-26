#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LevelSequence.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "SequencerTypes.h"

#include "SequencerAbstractionBPLibrary.generated.h"

class UControlRig;

UCLASS()
class SEQUENCERABSTRACTION_API USequencerAbstractionBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Asset lifecycle
    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Assets")
    static ULevelSequence* CreateLevelSequenceAsset(const FString& PackagePath, const FString& AssetName, FSequenceOpenResult& Result);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Assets")
    static bool SaveAsset(UObject* Asset);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Assets")
    static ULevelSequence* LoadLevelSequenceAsset(const FString& AssetPath);

    // Sequencer state
    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Sequencer")
    static ULevelSequence* GetCurrentOpenedLevelSequence();

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Sequencer")
    static TArray<FSequenceTrackInfo> GetAllTracksInCurrentSequence();

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Sequencer")
    static bool OpenLevelSequenceInSequencer(ULevelSequence* Sequence);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Sequencer")
    static FGuid AddSkeletalMeshToOpenSequenceFromPath(
        UObject* WorldContextObject,
        const FString& SkeletalMeshAssetPath,
        const FName SpawnedActorLabel,
        FSequenceOpenResult& Result);

    // Content loading
    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Content")
    static USkeletalMesh* LoadSkeletalMesh(const FString& AssetPath);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Content")
    static UAnimSequence* LoadAnimSequence(const FString& AssetPath);

    // Track helpers
    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Tracks")
    static bool RemoveTrackInSequenceByTrackPath(
        ULevelSequence* Sequence,
        const FString& TrackPath,
        FSequenceOpenResult& Result);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Tracks")
    static int32 RemoveTracksInSequenceByBindingGuid(
        ULevelSequence* Sequence,
        const FGuid& BindingGuid,
        const FString& OptionalTrackClassName,
        FSequenceOpenResult& Result);

    // Sections
    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Sections")
    static bool RemoveAnimSection(
        ULevelSequence* Sequence,
        UMovieSceneSkeletalAnimationSection* Section,
        FSequenceOpenResult& Result);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Sections")
    static int32 RemoveAnimSectionsByAnimSequence(
        ULevelSequence* Sequence,
        UAnimSequenceBase* Animation,
        UPARAM(meta = (ClampMin = "0")) int32 MaxToRemove,
        FSequenceOpenResult& Result,
        const FGuid& BindingGuid);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Sections")
    static bool MoveAnimationSectionStartTo(
        ULevelSequence* Sequence,
        UMovieSceneSkeletalAnimationSection* Section,
        int32 NewStartFrame,
        FSequenceOpenResult& Result);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Sections")
    static bool MoveAnimationSectionEndTo(
        ULevelSequence* Sequence,
        UMovieSceneSkeletalAnimationSection* Section,
        int32 NewEndFrameInclusive,
        FSequenceOpenResult& Result);

    // Timing helpers
    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Timing")
    static bool SetSequenceFrameRateFromAnimation(
        ULevelSequence* Sequence,
        UAnimSequence* Animation,
        bool bAlsoSetPlaybackRangeToAnim,
        FSequenceOpenResult& Result);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Timing")
    static bool SetSequencePlaybackRange(
        ULevelSequence* Sequence,
        int32 StartFrame,
        int32 EndFrame);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Timing")
    static bool GetSequencePlaybackRange(
        ULevelSequence* Sequence,
        int32& OutStartFrame,
        int32& OutEndFrame);

    // Rename the older duplicates so UHT doesn't conflict
    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Timing", meta = (DisplayName = "Move Animation Section Start To (Legacy)"))
    static bool MoveAnimationSectionStartTo_Legacy(
        UMovieSceneSkeletalAnimationSection* Section,
        int32 NewStartFrame);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Timing", meta = (DisplayName = "Move Animation Section End To (Legacy)"))
    static bool MoveAnimationSectionEndTo_Legacy(
        UMovieSceneSkeletalAnimationSection* Section,
        int32 NewEndFrame);

    // Binding / adding animation
    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Animation",
        meta = (CPP_Default_RowIndex = "-1", CPP_Default_bAllowOverlapSameRow = "false"))
    static UMovieSceneSkeletalAnimationSection* AddAnimSectionToBinding(
        ULevelSequence* Sequence,
        const FGuid& BindingGuid,
        UAnimSequence* Animation,
        int32 StartFrame,
        int32 RowIndex,
        bool bAllowOverlapSameRow,
        FSequenceOpenResult& Result);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|ControlRig")
    static bool AddRigToBinding(
        ULevelSequence* Sequence,
        UObject* WorldContextObject,
        const FGuid& BindingGuid,
        TSubclassOf<UControlRig> ControlRigClass,
        bool bLayered,
        FSequenceOpenResult& Result);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Bake")
    static bool BakeBindingToAnimSequence(
        ULevelSequence* Sequence,
        UObject* WorldContextObject,
        const FGuid& BindingGuid,
        const FString& TargetPackagePath,
        const FString& NewAssetName,
        FSequenceOpenResult& Result);
};
