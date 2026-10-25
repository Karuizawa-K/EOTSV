#include "EnemyProjectile.h"
#include "MC.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AEnemyProjectile::AEnemyProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    InitialLifeSpan = 5.0f;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->InitSphereRadius(6.0f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    CollisionSphere->SetGenerateOverlapEvents(true);
    CollisionSphere->SetCanEverAffectNavigation(false);
    CollisionSphere->OnComponentHit.AddDynamic(this, &AEnemyProjectile::OnProjectileHit);
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemyProjectile::OnProjectileOverlap);

    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProjectileMesh->SetGenerateOverlapEvents(false);
    ProjectileMesh->SetCanEverAffectNavigation(false);
    ProjectileMesh->SetRelativeScale3D(FVector(0.12f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        ProjectileMesh->SetStaticMesh(SphereMesh.Object);
    }

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->SetUpdatedComponent(CollisionSphere);
    ProjectileMovement->InitialSpeed = 900.0f;
    ProjectileMovement->MaxSpeed = 900.0f;
    ProjectileMovement->Velocity = FVector::ForwardVector;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bSweepCollision = true;
}

void AEnemyProjectile::BeginPlay()
{
    // Movement sweeps ignore the shooter; impact callbacks also guard against
    // initial overlaps before BeginPlay or overlaps caused by the shooter's movement.
    if (GetOwner())
    {
        CollisionSphere->IgnoreActorWhenMoving(GetOwner(), true);
    }
    if (GetInstigator())
    {
        CollisionSphere->IgnoreActorWhenMoving(GetInstigator(), true);
    }

    Super::BeginPlay();
}

void AEnemyProjectile::HandleImpact(AActor* OtherActor)
{
    if (bHasImpacted || OtherActor == this
        || (OtherActor && (OtherActor == GetOwner() || OtherActor == GetInstigator())))
    {
        return;
    }

    // Guard and disable collision before damage, which can immediately respawn the player.
    bHasImpacted = true;
    SetActorEnableCollision(false);
    ProjectileMovement->StopMovementImmediately();

    if (AMC* Player = Cast<AMC>(OtherActor))
    {
        Player->TakeCombatDamage(Damage);
    }

    Destroy();
}

void AEnemyProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
    HandleImpact(OtherActor);
}

void AEnemyProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    // Support player overlap presets without consuming shots on unrelated trigger volumes.
    if (Cast<AMC>(OtherActor))
    {
        HandleImpact(OtherActor);
    }
}
