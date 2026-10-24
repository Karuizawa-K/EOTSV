
#include "BantayStatue.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

ABantayStatue::ABantayStatue()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    StatueMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StatueMesh"));
    StatueMesh->SetupAttachment(RootComponent);

    ActivationRange = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationRange"));
    ActivationRange->SetupAttachment(RootComponent);
    ActivationRange->SetSphereRadius(250.0f);
    ActivationRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ActivationRange->SetCollisionResponseToAllChannels(ECR_Ignore);
    ActivationRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    InteractionDistance = 250.0f;
    RequiredFacingDot = 0.5f;
}

void ABantayStatue::BeginPlay()
{
    Super::BeginPlay();
}

void ABantayStatue::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ABantayStatue::ActivateCheckpoint()
{
    UE_LOG(LogTemp, Warning, TEXT("Bantay checkpoint activated: %s"), *GetName());
}
