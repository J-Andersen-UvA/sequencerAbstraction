#include "SequencerAbstractionBPLibrary.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Subsystems/AssetEditorSubsystem.h"

#include "ISequencer.h"
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "MVVM/SectionModelStorageExtension.h"
#include "MVVM/Selection/Selection.h"
#include "MVVM/ViewModels/ChannelModel.h"
#include "MVVM/ViewModels/SectionModel.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "LevelSequence.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Logging/LogMacros.h"

#include "MovieScene.h"
#include "MovieSceneSequencePlayer.h"
#include "MovieSceneTimeUnit.h"
#include "MovieSceneTrack.h"
#include "MovieSceneSection.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Sections/MovieSceneSkeletalAnimationSection.h"
#include "MovieSceneBindingProxy.h"
#include "ControlRigSequencerEditorLibrary.h"
#include "MovieSceneObjectBindingID.h" // UE::MovieScene::FRelativeObjectBindingID, FMovieSceneObjectBindingID
#include "MovieScenePossessable.h"
#include "MovieSceneSpawnable.h"
#include "Channels/MovieSceneBoolChannel.h"
#include "LevelSequenceEditorSubsystem.h"
#include "ExtensionLibraries/MovieSceneSectionExtensions.h"
#include "MovieSceneScriptingChannel.h"
#include "Channels/MovieSceneChannel.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "MediaSource.h"
#include "MovieSceneMediaSection.h"
#include "MovieSceneMediaTrack.h"

#include "LevelEditor.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Modules/ModuleManager.h"
#include "IAssetViewport.h"

#include "Exporters/AnimSeqExportOption.h"
#include "Animation/AnimationSettings.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimCurveTypes.h"
#include "Factories/AnimSequenceFactory.h"

#include "Animation/SkeletalMeshActor.h"
#include "ControlRig.h"
#include "ControlRigObjectBinding.h"
#include "Rigs/FKControlRig.h"
#include "Rigs/RigHierarchy.h"
#include "Sequencer/MovieSceneControlRigParameterTrack.h"
#include "Sequencer/MovieSceneControlRigParameterSection.h"
#include "SequencerTools.h"
#include "Units/Execution/RigUnit_InverseExecution.h"
#include "Modules/ModuleManager.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UnrealType.h"

static TSharedPtr<ISequencer> GetOpenSequencerForSequence(ULevelSequence* Sequence)
{
#if WITH_EDITOR
    if (!Sequence || !GEditor)
    {
        return nullptr;
    }

    UAssetEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
    if (!EditorSubsystem)
    {
        return nullptr;
    }

    if (IAssetEditorInstance* Inst = EditorSubsystem->FindEditorForAsset(Sequence, false))
    {
        if (ILevelSequenceEditorToolkit* Toolkit = static_cast<ILevelSequenceEditorToolkit*>(Inst))
        {
            return Toolkit->GetSequencer();
        }
    }
#endif

    return nullptr;
}


bool UBakeAbstraction::BakeBindingToControlRig(
    const FGuid& BindingGuid,
    TSubclassOf<UControlRig> ControlRigClass,
    bool bReduceKeys,
    float Tolerance,
    bool bResetControls
)
{
#if WITH_EDITOR
	UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return false;

	ULevelSequence* LevelSequence = USequencerAbstractionBPLibrary::GetCurrentOpenedLevelSequence();
	if (!LevelSequence) return false;

    TSubclassOf<UControlRig> FKRigClass = ControlRigClass;
	if (!FKRigClass) return false;

	UAnimSeqExportOption* Params = NewObject<UAnimSeqExportOption>(GetTransientPackage());
	Params->ResetToDefault();
    Params->bBakeTimecode = true;

    FMovieSceneBindingProxy BindingProxy(BindingGuid, LevelSequence);

	UControlRigSequencerEditorLibrary::BakeToControlRig(
		World,
		LevelSequence,
		FKRigClass,
		Params,
		bReduceKeys,
		Tolerance,
		BindingProxy,
        bResetControls
	);

    TSharedPtr<ISequencer> SequencerPtr = GetOpenSequencerForSequence(LevelSequence);
    if (!SequencerPtr.IsValid()) return false;

	SequencerPtr->ForceEvaluate();

    return true;
#else
	UE_LOG(LogTemp, Warning, TEXT("Only usable in the editor."));
    return false;
#endif
}