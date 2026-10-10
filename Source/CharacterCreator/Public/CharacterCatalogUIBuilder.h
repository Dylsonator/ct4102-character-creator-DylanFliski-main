#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CharacterCatalogUIBuilder.generated.h"

class AActor;
class UCharacterOptionCatalog;
class UScrollBox;

/**
 * One Blueprint node replaces the four old CharacterTypes Data Table generation loops.
 * The existing UMG designer, colour picker and layout remain in place.
 */
UCLASS()
class CHARACTERCREATOR_API UCharacterCatalogUIBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Populate four existing scroll boxes from DA_CharacterOptions.
     *
     * - The character must have the Phase 1 CharacterCustomizationComponent.
     * - Leave CharacterActor unset to auto-find a character with that component.
     * - With ClearExisting enabled, old prebuilt/legacy buttons are removed.
     * - Invalid/missing mesh references are skipped (not shown as empty buttons).
     * - This DOES NOT change or replace your colour/skin/extra controls.
     *
     * Place this node AFTER the legacy UI's Data Table button-creation loops, or
     * delete/disable those four loops once the new node has been tested.
     */
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Catalog UI",
        meta = (WorldContext = "WorldContextObject",
                DisplayName = "Populate Character Part Buttons"))
    static int32 PopulateCharacterPartButtons(
        const UObject* WorldContextObject,
        UCharacterOptionCatalog* Catalog,
        AActor* CharacterActor,
        UScrollBox* HeadScrollBox,
        UScrollBox* BodyScrollBox,
        UScrollBox* LegsScrollBox,
        UScrollBox* FeetScrollBox,
        bool bClearExisting = true);
};
