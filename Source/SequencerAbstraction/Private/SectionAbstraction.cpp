#include "AnimationBlender.h"

#include "Sections/MovieSceneSkeletalAnimationSection.h"

void USectionAbstraction::SetAnimationAsset(
	UMovieSceneSkeletalAnimationSection* Section,
	UAnimSequenceBase* Animation)
{
	if (!Section || !Animation)
	{
		return;
	}

	Section->Params.Animation = Animation;
	Section->Modify(true);

	return;
}

FSectionLabelEntry USectionAbstraction::CreateSectionLabelEntry(
	UMovieSceneSection* Section,
	const FString& Label)
{
	FSectionLabelEntry Entry;

	Entry.Section = Section;
	Entry.Label = Label;

	return Entry;
}