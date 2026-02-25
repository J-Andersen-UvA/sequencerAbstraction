#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LevelSequence.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "SequencerTypes.h"
#include "SequencerAbstractionBPLibrary.generated.h"

UCLASS()
class SEQUENCERABSTRACTION_API USequencerAbstractionBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Asset lifecycle
    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Assets")
    static ULevelSequence* CreateLevelSequenceAsset(const FString& PackagePath, const FString& AssetName, FSequenceOpenResult& Result);

    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Assets")
    static bool SaveAsset(UObject* Asset);

    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Assets")
    static ULevelSequence* LoadLevelSequenceAsset(const FString& AssetPath);

    // Sequencer state
    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Sequencer")
    static ULevelSequence* GetCurrentOpenedLevelSequence();

    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Sequencer")
    static TArray<FSequenceTrackInfo> GetAllTracksInCurrentSequence();

    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Sequencer")
    static ULevelSequence* CreateEmptyLevelSequenceAsset(const FString& PackagePath, const FString& AssetName, FSequenceOpenResult& Result);

    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|Sequencer")
    static bool OpenLevelSequenceInSequencer(ULevelSequence* Sequence);

    // Content loading
    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Content")
    static USkeletalMesh* LoadSkeletalMesh(const FString& AssetPath);

    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Content")
    static UAnimSequence* LoadAnimSequence(const FString& AssetPath);

    // Timing helpers
    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Timing")
    static bool SetSequenceFrameRateFromAnimation(ULevelSequence* Sequence, UAnimSequence* Animation, bool bAlsoSetPlaybackRangeToAnim, FSequenceOpenResult& Result);

    // Binding / adding animation
    UFUNCTION(BlueprintCallable, Category="SequencerAbstraction|Animation")
    static bool AddAnimationToSequenceOnSkeletalMeshComponent(
        ULevelSequence* Sequence,
        UObject* WorldContextObject,
        USkeletalMesh* SkeletalMesh,
        UAnimSequence* Animation,
        const FName ActorLabelOrName,
        FSequenceOpenResult& Result
    );
};