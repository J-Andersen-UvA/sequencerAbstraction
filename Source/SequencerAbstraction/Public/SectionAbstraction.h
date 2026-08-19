#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "LevelSequence.h"

#include "SequencerTypes.h"
#include "SectionAbstraction.generated.h"

USTRUCT(BlueprintType)
struct FSectionLabelEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SequenceAbstraction|SecctionAbstraction")
	TObjectPtr<UMovieSceneSection> Section = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SequenceAbstraction|SectionAbstraction")
	FString Label;
};

UCLASS()
class SEQUENCERABSTRACTION_API USectionAbstraction : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /* Pairs sections with corresponding labels into a list of structs */
	UFUNCTION(BlueprintCallable, Category = "SequenceAbstraction|SectionAbstraction")
	static FSectionLabelEntry CreateSectionLabelEntry(
		UMovieSceneSection* Section,
		const FString& Label
	);

    /* Sets the animation asset for a skeletal animation section */
	UFUNCTION(BlueprintCallable, Category = "SequenceAbstraction|SectionAbstraction")
	static void SetAnimationAsset(
		UMovieSceneSkeletalAnimationSection* Section,
		UAnimSequenceBase* Animation
	);
    
    /* Splits an animation section at a specified frame */
    UFUNCTION(BlueprintCallable, Category = "SequencerAbstraction|SectionAbstraction")
    static void SplitAnimationSection(
        UMovieSceneSection* Section,
        int32 SplitFrame,
        FFrameRate FrameRate,
        UMovieSceneSkeletalAnimationSection*& OutRightSection,
        UMovieSceneSkeletalAnimationSection*& OutLeftSection,
        bool bDeleteKeys = false
    );

    /* Functions moved from SequencerAbstractionBPLibrary class */

	static bool RemoveAnimationSection(
		ULevelSequence* Sequence,
		UMovieSceneSkeletalAnimationSection* Section,
		FSequenceOpenResult& Result
	);


private:

    

};