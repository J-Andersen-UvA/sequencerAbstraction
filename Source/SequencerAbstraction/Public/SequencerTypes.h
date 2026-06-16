#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "SequencerTypes.generated.h"

class UMovieSceneSection;
class UMovieSceneSkeletalAnimationSection;
class UAnimSequenceBase;

USTRUCT(BlueprintType)
struct FSequenceBindingInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString DisplayName;      // Sequencer outliner label
    UPROPERTY(BlueprintReadOnly) FGuid BindingGuid;
    UPROPERTY(BlueprintReadOnly) FMovieSceneObjectBindingID ObjectBindingId;

    UPROPERTY(BlueprintReadOnly) bool bIsSpawnable = false;
    UPROPERTY(BlueprintReadOnly) bool bIsPossessable = false;

    UPROPERTY(BlueprintReadOnly) FString BoundObjectClass; // Actor/Component class (if known)
    UPROPERTY(BlueprintReadOnly) int32 TrackCount = 0;     // Tracks directly under this binding
};

USTRUCT(BlueprintType)
struct FSequenceSectionInfo
{
    GENERATED_BODY()

    // Direct handle (best for later remove/move)
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UMovieSceneSection> Section = nullptr;

    // Optional convenience fields for UI/debug
    UPROPERTY(BlueprintReadOnly)
    int32 RowIndex = 0;

    // Tick units (MovieScene tick resolution)
    UPROPERTY(BlueprintReadOnly)
    int32 StartTick = 0;

    // Exclusive end in tick units
    UPROPERTY(BlueprintReadOnly)
    int32 EndTickExclusive = 0;

    // Only set when this section is a skeletal animation section
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UAnimSequenceBase> AnimSequence = nullptr;
};


USTRUCT(BlueprintType)
struct FSequenceTrackInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString DisplayName;
    UPROPERTY(BlueprintReadOnly) FString TrackType;          // Class name (or a friendlier label if you map it)
    UPROPERTY(BlueprintReadOnly) FString ObjectBindingPath;  // e.g. "MyActor/MyComponent" if resolvable
    UPROPERTY(BlueprintReadOnly) FGuid BindingGuid;          // For object-bound tracks
    UPROPERTY(BlueprintReadOnly) FString TrackPath;          // Your own stable string key (see below)
    UPROPERTY(BlueprintReadOnly) bool bIsMasterTrack = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FSequenceSectionInfo> Sections;
};

USTRUCT(BlueprintType)
struct FSequenceOpenResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) FString Error;
};

USTRUCT(BlueprintType)
struct FActiveSkeletalAnimationInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UAnimSequenceBase> Animation = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UMovieSceneSkeletalAnimationSection> Section = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FGuid BindingGuid;

    UPROPERTY(BlueprintReadOnly)
    FString BindingName;

    UPROPERTY(BlueprintReadOnly)
    int32 SequencerDisplayFrame = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 SequencerTickFrame = 0;

    UPROPERTY(BlueprintReadOnly)
    float AnimationTimeSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 RowIndex = 0;
};

USTRUCT(BlueprintType)
struct FSourceAnimationBoneFrameData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName BoneName = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FTransform LocalTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct FSourceAnimationCurveFrameData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName CurveName = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    float Value = 0.0f;
};

USTRUCT(BlueprintType)
struct FSourceAnimationFrameData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UAnimSequenceBase> Animation = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float AnimationTimeSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 AnimationFrame = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<FSourceAnimationBoneFrameData> Bones;

    UPROPERTY(BlueprintReadOnly)
    TArray<FSourceAnimationCurveFrameData> Curves;
};
