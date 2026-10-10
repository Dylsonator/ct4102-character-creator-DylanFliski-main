#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterCustomizationComponent.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;

/** C++ equivalent of the original Blueprint BodyParts enum. */
UENUM(BlueprintType)
enum class ECreatorBodyPart : uint8
{
    Head UMETA(DisplayName = "Head"),
    Body UMETA(DisplayName = "Body / Torso"),
    Legs UMETA(DisplayName = "Legs"),
    Feet UMETA(DisplayName = "Feet")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnCreatorPartChanged,
    ECreatorBodyPart, BodyPart,
    USkeletalMesh*, NewMesh
);

/**
 * Minimal, non-destructive C++ migration of the existing modular-character mesh swaps.
 *
 * Keep the existing Character Blueprint and its mesh components. Add this component
 * to the Character Blueprint, and forward the existing SwapHead / SwapTorso /
 * SwapLegs / SwapFeet Blueprint events to the functions on this component.
 *
 * The legacy CharacterTypes/ColourTable DataTables and BP structs are untouched.
 */
UCLASS(ClassGroup = (CharacterCreator), meta = (BlueprintSpawnableComponent))
class UCharacterCustomizationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCharacterCustomizationComponent();

    /** Automatically bind components named Head, Body, Legs and Feet on BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Setup")
    bool bAutoFindMeshComponents = true;

    /** These names can be edited if your existing Blueprint components use different names. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Setup")
    FName HeadComponentName = TEXT("Head");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Setup")
    FName BodyComponentName = TEXT("Body");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Setup")
    FName LegsComponentName = TEXT("Legs");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Setup")
    FName FeetComponentName = TEXT("Feet");

    /** Optional: only enable when all the body parts use compatible skeletons. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    bool bSyncPartsToBodyOnBeginPlay = false;

    UPROPERTY(BlueprintAssignable, Category = "Character Creator|Events")
    FOnCreatorPartChanged OnPartChanged;

    /** Use this if automatic component discovery doesn't match the Blueprint component names. */
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Setup")
    void RegisterPartComponent(ECreatorBodyPart BodyPart, USkeletalMeshComponent* MeshComponent);

    /** Find mesh components on the owning Character actor by component name. */
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Setup")
    void AutoFindMeshComponents();

    /** A single reusable replacement for all four original Set Skeletal Mesh Blueprint nodes. */
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Customisation")
    bool SetCharacterPart(ECreatorBodyPart BodyPart, USkeletalMesh* NewMesh);

    /** Convenience functions to forward the original Blueprint custom events directly. */
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Customisation")
    bool SetHeadMesh(USkeletalMesh* NewMesh);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Customisation")
    bool SetBodyMesh(USkeletalMesh* NewMesh);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Customisation")
    bool SetLegsMesh(USkeletalMesh* NewMesh);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Customisation")
    bool SetFeetMesh(USkeletalMesh* NewMesh);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Setup")
    USkeletalMeshComponent* GetPartComponent(ECreatorBodyPart BodyPart) const;

    /** Make Head, Legs and Feet follow Body's pose. Call only when their skeletons are compatible. */
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    bool SyncModularAnimationToBody();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> HeadMeshComponent;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> BodyMeshComponent;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> LegsMeshComponent;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> FeetMeshComponent;

    USkeletalMeshComponent* FindMeshComponentNamed(FName ComponentName) const;
};
