#include "CharacterCustomizationComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"

UCharacterCustomizationComponent::UCharacterCustomizationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCharacterCustomizationComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoFindMeshComponents)
    {
        AutoFindMeshComponents();
    }

    if (bSyncPartsToBodyOnBeginPlay)
    {
        SyncModularAnimationToBody();
    }
}

USkeletalMeshComponent* UCharacterCustomizationComponent::FindMeshComponentNamed(FName ComponentName) const
{
    const AActor* Owner = GetOwner();
    if (!Owner || ComponentName.IsNone())
    {
        return nullptr;
    }

    TArray<USkeletalMeshComponent*> MeshComponents;
    Owner->GetComponents<USkeletalMeshComponent>(MeshComponents);

    for (USkeletalMeshComponent* Component : MeshComponents)
    {
        if (IsValid(Component) && Component->GetFName() == ComponentName)
        {
            return Component;
        }
    }

    return nullptr;
}

void UCharacterCustomizationComponent::AutoFindMeshComponents()
{
    // Explicit registrations take priority over name-based discovery.
    if (!IsValid(HeadMeshComponent))
    {
        HeadMeshComponent = FindMeshComponentNamed(HeadComponentName);
    }

    if (!IsValid(BodyMeshComponent))
    {
        BodyMeshComponent = FindMeshComponentNamed(BodyComponentName);
    }

    if (!IsValid(LegsMeshComponent))
    {
        LegsMeshComponent = FindMeshComponentNamed(LegsComponentName);
    }

    if (!IsValid(FeetMeshComponent))
    {
        FeetMeshComponent = FindMeshComponentNamed(FeetComponentName);
    }
}

void UCharacterCustomizationComponent::RegisterPartComponent(
    ECreatorBodyPart BodyPart,
    USkeletalMeshComponent* MeshComponent)
{
    if (!IsValid(MeshComponent) || MeshComponent->GetOwner() != GetOwner())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("CharacterCreator: Attempted to register an invalid or foreign SkeletalMeshComponent."));
        return;
    }

    switch (BodyPart)
    {
        case ECreatorBodyPart::Head:
            HeadMeshComponent = MeshComponent;
            break;
        case ECreatorBodyPart::Body:
            BodyMeshComponent = MeshComponent;
            break;
        case ECreatorBodyPart::Legs:
            LegsMeshComponent = MeshComponent;
            break;
        case ECreatorBodyPart::Feet:
            FeetMeshComponent = MeshComponent;
            break;
        default:
            break;
    }
}

USkeletalMeshComponent* UCharacterCustomizationComponent::GetPartComponent(
    ECreatorBodyPart BodyPart) const
{
    switch (BodyPart)
    {
        case ECreatorBodyPart::Head:
            return HeadMeshComponent;
        case ECreatorBodyPart::Body:
            return BodyMeshComponent;
        case ECreatorBodyPart::Legs:
            return LegsMeshComponent;
        case ECreatorBodyPart::Feet:
            return FeetMeshComponent;
        default:
            return nullptr;
    }
}

bool UCharacterCustomizationComponent::SetCharacterPart(
    ECreatorBodyPart BodyPart,
    USkeletalMesh* NewMesh)
{
    // Do not accidentally clear an existing character part when a row is missing a mesh.
    if (!IsValid(NewMesh))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterCreator: No valid mesh was supplied for part %d."),
            static_cast<int32>(BodyPart));
        return false;
    }

    USkeletalMeshComponent* TargetComponent = GetPartComponent(BodyPart);
    if (!IsValid(TargetComponent) && bAutoFindMeshComponents)
    {
        AutoFindMeshComponents();
        TargetComponent = GetPartComponent(BodyPart);
    }

    if (!IsValid(TargetComponent))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("CharacterCreator: Cannot swap part %d. Assign Head/Body/Legs/Feet components or call RegisterPartComponent."),
            static_cast<int32>(BodyPart));
        return false;
    }

    TargetComponent->SetSkeletalMeshAsset(NewMesh);
    OnPartChanged.Broadcast(BodyPart, NewMesh);
    return true;
}

bool UCharacterCustomizationComponent::SetHeadMesh(USkeletalMesh* NewMesh)
{
    return SetCharacterPart(ECreatorBodyPart::Head, NewMesh);
}

bool UCharacterCustomizationComponent::SetBodyMesh(USkeletalMesh* NewMesh)
{
    return SetCharacterPart(ECreatorBodyPart::Body, NewMesh);
}

bool UCharacterCustomizationComponent::SetLegsMesh(USkeletalMesh* NewMesh)
{
    return SetCharacterPart(ECreatorBodyPart::Legs, NewMesh);
}

bool UCharacterCustomizationComponent::SetFeetMesh(USkeletalMesh* NewMesh)
{
    return SetCharacterPart(ECreatorBodyPart::Feet, NewMesh);
}

bool UCharacterCustomizationComponent::SyncModularAnimationToBody()
{
    if (!IsValid(BodyMeshComponent) && bAutoFindMeshComponents)
    {
        AutoFindMeshComponents();
    }

    if (!IsValid(BodyMeshComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterCreator: No Body component found to drive modular animations."));
        return false;
    }

    // The Body component can drive one animation while other compatible parts
    // copy its final bone transforms. Verify skeleton compatibility in the editor.
    if (IsValid(HeadMeshComponent) && HeadMeshComponent != BodyMeshComponent)
    {
        HeadMeshComponent->SetLeaderPoseComponent(BodyMeshComponent);
    }
    if (IsValid(LegsMeshComponent) && LegsMeshComponent != BodyMeshComponent)
    {
        LegsMeshComponent->SetLeaderPoseComponent(BodyMeshComponent);
    }
    if (IsValid(FeetMeshComponent) && FeetMeshComponent != BodyMeshComponent)
    {
        FeetMeshComponent->SetLeaderPoseComponent(BodyMeshComponent);
    }

    return true;
}
