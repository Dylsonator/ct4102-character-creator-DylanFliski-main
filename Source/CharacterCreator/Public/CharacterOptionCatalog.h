#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterCustomizationComponent.h"
#include "CharacterOptionCatalog.generated.h"

class USkeletalMesh;
class UTexture2D;

/** A single option displayed by the new character creator. */
USTRUCT(BlueprintType)
struct FCreatorMeshOption
{
    GENERATED_BODY()

    /** Stable key derived from the asset's Unreal object path (not from its display name). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Creator")
    FName OptionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator")
    ECreatorBodyPart BodyPart = ECreatorBodyPart::Head;

    /** Soft reference keeps asset loading deferred until the player selects an option. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator")
    TSoftObjectPtr<USkeletalMesh> Mesh;

    /** Optional matching texture used by the character-creator UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator")
    TSoftObjectPtr<UTexture2D> Thumbnail;
};

/** Which existing Pictures folder a thumbnail came from. */
UENUM(BlueprintType)
enum class ECreatorThumbnailGroup : uint8
{
    Character UMETA(DisplayName = "Characters"),
    Colour UMETA(DisplayName = "Colours"),
    Skin UMETA(DisplayName = "Skins")
};

/** A thumbnail kept for the future colour/skin picker, as well as mesh icons. */
USTRUCT(BlueprintType)
struct FCreatorThumbnailEntry
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Creator")
    FName OptionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator")
    ECreatorThumbnailGroup Group = ECreatorThumbnailGroup::Character;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator")
    TSoftObjectPtr<UTexture2D> Texture;
};

/**
 * Generated, editable catalog of character part options.
 *
 * Populate this asset using ACharacterOptionCatalogBuilder's editor button.
 * The old Blueprint CharacterStruct, CharacterTypes Data Table and UMG widgets
 * are deliberately untouched until the new UI is ready.
 */
UCLASS(BlueprintType)
class CHARACTERCREATOR_API UCharacterOptionCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Options")
    TArray<FCreatorMeshOption> Options;

    /** All images from Pictures/Characters, Pictures/Colours and Pictures/Skins.
     * Colour/Skin entries are thumbnail references only; this does not invent
     * skin colour values or replace your ColourTable Data Table.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Thumbnails")
    TArray<FCreatorThumbnailEntry> ThumbnailLibrary;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Catalog")
    void GetThumbnailsForGroup(ECreatorThumbnailGroup Group, TArray<FCreatorThumbnailEntry>& OutThumbnails) const;

    /** Ready for the next phase's auto-generated UMG buttons. */
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Catalog")
    void GetOptionsForPart(ECreatorBodyPart BodyPart, TArray<FCreatorMeshOption>& OutOptions) const;

    /** Apply one of the catalog entries directly to the existing Phase 1 C++ component. */
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Catalog")
    bool ApplyOption(UCharacterCustomizationComponent* Target, const FCreatorMeshOption& Option) const;
};
