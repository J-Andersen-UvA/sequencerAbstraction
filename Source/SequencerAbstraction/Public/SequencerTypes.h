#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "SequencerTypes.generated.h"

class UMovieSceneSection;
class UAnimSequenceBase;

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