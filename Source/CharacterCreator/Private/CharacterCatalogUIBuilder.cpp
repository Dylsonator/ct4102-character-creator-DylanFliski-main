#include "CharacterCatalogUIBuilder.h"

#include "CharacterCatalogOptionButton.h"
#include "CharacterCustomizationComponent.h"
#include "CharacterOptionCatalog.h"
#include "Components/ScrollBox.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

namespace
{
    static UScrollBox* ScrollBoxForPart(
        ECreatorBodyPart Part, UScrollBox* Head, UScrollBox* Body,
        UScrollBox* Legs, UScrollBox* Feet)
    {
        switch (Part)
        {
            case ECreatorBodyPart::Head: return Head;
            case ECreatorBodyPart::Body: return Body;
            case ECreatorBodyPart::Legs: return Legs;
            case ECreatorBodyPart::Feet: return Feet;
            default: return nullptr;
        }
    }
}

int32 UCharacterCatalogUIBuilder::PopulateCharacterPartButtons(
    const UObject* WorldContextObject,
    UCharacterOptionCatalog* Catalog,
    AActor* CharacterActor,
    UScrollBox* HeadScrollBox,
    UScrollBox* BodyScrollBox,
    UScrollBox* LegsScrollBox,
    UScrollBox* FeetScrollBox,
    bool bClearExisting)
{
    if (!IsValid(Catalog))
    {
        UE_LOG(LogTemp, Error,
            TEXT("CharacterCreator Phase3: Assign DA_CharacterOptions to the Catalog input."));
        return 0;
    }

    UWorld* World = IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
    if (!IsValid(World))
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterCreator Phase3: No valid world for creating UI buttons."));
        return 0;
    }

    UCharacterCustomizationComponent* Target = IsValid(CharacterActor)
        ? CharacterActor->FindComponentByClass<UCharacterCustomizationComponent>()
        : nullptr;

    if (!IsValid(Target))
    {
        // Matches the existing creator's one-preview-character workflow.
        // Explicitly pass CharacterActor if a level ever has several characters.
        for (TActorIterator<AActor> Actor(World); Actor; ++Actor)
        {
            if (UCharacterCustomizationComponent* Found =
                Actor->FindComponentByClass<UCharacterCustomizationComponent>())
            {
                Target = Found;
                break;
            }
        }
    }

    if (!IsValid(Target))
    {
        UE_LOG(LogTemp, Error,
            TEXT("CharacterCreator Phase3: No CharacterCustomizationComponent found. " )
            TEXT("Call this after the preview Character is spawned."));
        return 0; // Don't clear existing buttons if no target exists.
    }

    if (!IsValid(HeadScrollBox) && !IsValid(BodyScrollBox) &&
        !IsValid(LegsScrollBox) && !IsValid(FeetScrollBox))
    {
        UE_LOG(LogTemp, Error,
            TEXT("CharacterCreator Phase3: Connect at least one ScrollBox widget pin."));
        return 0;
    }

    if (bClearExisting)
    {
        if (IsValid(HeadScrollBox)) HeadScrollBox->ClearChildren();
        if (IsValid(BodyScrollBox)) BodyScrollBox->ClearChildren();
        if (IsValid(LegsScrollBox)) LegsScrollBox->ClearChildren();
        if (IsValid(FeetScrollBox)) FeetScrollBox->ClearChildren();
    }

    int32 AddedCount = 0;
    int32 SkippedCount = 0;
    TSet<FName> UsedOptionIds;

    for (const FCreatorMeshOption& Option : Catalog->Options)
    {
        UScrollBox* Destination = ScrollBoxForPart(Option.BodyPart,
            HeadScrollBox, BodyScrollBox, LegsScrollBox, FeetScrollBox);
        if (!IsValid(Destination)) continue;

        if (UsedOptionIds.Contains(Option.OptionId) || Option.Mesh.IsNull())
        {
            ++SkippedCount;
            continue;
        }

        // Detect deleted/broken references before creating a button.
        // For a small character creator catalogue this synchronous check is fine;
        // call once when building the UI, not every frame.
        if (!IsValid(Option.Mesh.LoadSynchronous()))
        {
            ++SkippedCount;
            UE_LOG(LogTemp, Warning,
                TEXT("CharacterCreator Phase3: Skipping missing mesh for %s."),
                *Option.OptionId.ToString());
            continue;
        }

        UCharacterCatalogOptionButton* Tile =
            CreateWidget<UCharacterCatalogOptionButton>(
                World, UCharacterCatalogOptionButton::StaticClass());
        if (!IsValid(Tile))
        {
            ++SkippedCount;
            continue;
        }

        Tile->Configure(Option, Catalog, Target);
        Destination->AddChild(Tile);
        UsedOptionIds.Add(Option.OptionId);
        ++AddedCount;
    }

    UE_LOG(LogTemp, Display,
        TEXT("CharacterCreator Phase3: Added %d working mesh buttons; skipped %d invalid/duplicate options."),
        AddedCount, SkippedCount);
    return AddedCount;
}
