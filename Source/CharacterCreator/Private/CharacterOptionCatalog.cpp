#include "CharacterOptionCatalog.h"

#include "Engine/SkeletalMesh.h"

void UCharacterOptionCatalog::GetOptionsForPart(
    ECreatorBodyPart BodyPart, TArray<FCreatorMeshOption>& OutOptions) const
{
    OutOptions.Reset();
    for (const FCreatorMeshOption& Option : Options)
    {
        if (Option.BodyPart == BodyPart && !Option.Mesh.IsNull())
        {
            OutOptions.Add(Option);
        }
    }
}

bool UCharacterOptionCatalog::ApplyOption(
    UCharacterCustomizationComponent* Target, const FCreatorMeshOption& Option) const
{
    if (!IsValid(Target) || Option.Mesh.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterCreator: Cannot apply option: invalid component or mesh reference."));
        return false;
    }

    // Loading is only performed when an option is selected, not during the UI's list generation.
    USkeletalMesh* Mesh = Option.Mesh.LoadSynchronous();
    if (!IsValid(Mesh))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterCreator: Failed to load mesh for option %s."),
            *Option.OptionId.ToString());
        return false;
    }

    return Target->SetCharacterPart(Option.BodyPart, Mesh);
}

void UCharacterOptionCatalog::GetThumbnailsForGroup(
    ECreatorThumbnailGroup Group, TArray<FCreatorThumbnailEntry>& OutThumbnails) const
{
    OutThumbnails.Reset();
    for (const FCreatorThumbnailEntry& Entry : ThumbnailLibrary)
    {
        if (Entry.Group == Group && !Entry.Texture.IsNull())
        {
            OutThumbnails.Add(Entry);
        }
    }
}
