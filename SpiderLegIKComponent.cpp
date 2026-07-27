#include "SpiderLegIKComponent.h"

#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

USpiderLegIKComponent::USpiderLegIKComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USpiderLegIKComponent::BeginPlay()
{
	Super::BeginPlay();
	
	SpiderCharacter = Cast<ACharacter>(GetOwner()); 
	CreateFootIKInfo();
	
	LastVelocity = FVector::ZeroVector;
	CurrentMoveGroupId = 0;
	State = WaitMove;
	
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	QueryParams.AddIgnoredActor(GetOwner());
}

void USpiderLegIKComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	FVector CurrentVelocity = SpiderCharacter->GetVelocity();
	
	if (CurrentVelocity.IsZero() == true && LastVelocity.IsZero() == false)
	{
		State = Adjusting;
		
		for (auto& FootIKInfo : FootIKInfoArray)
		{
			FootIKInfo.FootBoneStartLocation = FootIKInfo.FootBoneCurrentLocation;
			FootIKInfo.FootBoneEndLocation = FootIKInfo.FootBoneCurrentLocation;
			FindLandingLocation(FootIKInfo.FootBonePlannedLocation, FootIKInfo.FootBoneEndLocation, FootIKInfo.FootMiddleLocation);
			FootIKInfo.MoveTime = 0;
		}
	}
	
	LastVelocity = CurrentVelocity;
	
	UpdatePlannedLocations(DeltaTime);
	
	switch(State)
	{
	case WaitMove:
		TryStartMoveFeet();
		break;
		
	case Moving:
		UpdateFeetMovement(DeltaTime);
		break;
		
	case Adjusting:
		UpdateFeetMovement(DeltaTime, 0.7);
		break;
		
	default:
		break;
	}
	
	//DebugFootIKInfo();
}

void USpiderLegIKComponent::CreateFootIKInfo()
{
	USkeletalMesh* SkeletalMesh = SpiderCharacter->GetMesh()->GetSkeletalMeshAsset();
	FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	
	for (int i = 0; i < LegGroups.Num(); i++)
	{
		for (int j = 0; j < LegGroups[i].FootBoneNames.Num(); j++)
		{
			int BoneIndex = RefSkeleton.FindBoneIndex(LegGroups[i].FootBoneNames[j]);
			FTransform BoneTransform = FAnimationRuntime::GetComponentSpaceTransform(RefSkeleton,RefSkeleton.GetRefBonePose(),BoneIndex);
			FVector FootOffset = BoneTransform.GetLocation();
			
			FString ParentBoneName = LegGroups[i].FootBoneNames[j].ToString();
			ParentBoneName.RemoveFromEnd(TEXT("004"));
			ParentBoneName += TEXT("002");
			//UE_LOG(LogTemp, Warning, TEXT("Location: %s"), *ParentBoneName);
			BoneIndex = RefSkeleton.FindBoneIndex(FName(ParentBoneName));
			BoneTransform = FAnimationRuntime::GetComponentSpaceTransform(RefSkeleton,RefSkeleton.GetRefBonePose(), BoneIndex);
			FVector FootMiddleJointOffset = BoneTransform.GetLocation();
			
			FFootIKInfo footIKInfo;
			
			footIKInfo.FootBoneName = LegGroups[i].FootBoneNames[j];
			footIKInfo.FootDefeaultOffset = FootOffset;
			footIKInfo.FootBonePlannedLocation = GetWorldLocation(FootOffset);
			footIKInfo.FootBoneCurrentLocation = GetWorldLocation(FootOffset);
			footIKInfo.FootBoneStartLocation = GetWorldLocation(FootOffset);
			footIKInfo.FootBoneEndLocation = GetWorldLocation(FootOffset);
			
			//footIKInfo.FootToMiddleOffset = FootMiddleJointOffset - FootOffset;
			footIKInfo.FootMiddleOffset = FootMiddleJointOffset;
			footIKInfo.FootMiddleLocation = GetWorldLocation(FootMiddleJointOffset);
			footIKInfo.MoveTime = 0;
			
			footIKInfo.GoupId = i;
			FootIKInfoArray.Add(footIKInfo);
		}
	}
}

void USpiderLegIKComponent::TryStartMoveFeet()
{
	bool bShouldMoveGroup = false;
	
	for (auto& FootIKInfo : FootIKInfoArray)
	{
		if (FootIKInfo.GoupId == CurrentMoveGroupId)
		{
			float Offset = (FootIKInfo.FootBoneCurrentLocation - FootIKInfo.FootBonePlannedLocation).Length();
			if (Offset >= StepThreshold)
			{
				bShouldMoveGroup = true;
				break;
			}	
		}
	}
	
	if (bShouldMoveGroup == true)
	{
		for (auto& FootIKInfo : FootIKInfoArray)
		{
			if (FootIKInfo.GoupId == CurrentMoveGroupId)
			{
				FootIKInfo.FootBoneStartLocation = FootIKInfo.FootBoneCurrentLocation;
				FootIKInfo.FootBoneEndLocation = FootIKInfo.FootBoneCurrentLocation;
				FindLandingLocation(FootIKInfo.FootBonePlannedLocation, FootIKInfo.FootBoneEndLocation, FootIKInfo.FootMiddleLocation);
				FootIKInfo.MoveTime = 0;
			}
		}
		State = Moving;
	}
}

void USpiderLegIKComponent::UpdateFeetMovement(float DeltaTime, float Speed)
{
	for (auto& FootIKInfo : FootIKInfoArray)
	{
		if (FootIKInfo.GoupId != CurrentMoveGroupId)
			continue;

		FootIKInfo.MoveTime += DeltaTime;
		float Alpha = FMath::Clamp(FootIKInfo.MoveTime / (LegMoveTime / Speed), 0.0f, 1.0f);
		
		FVector TargetLocation = FMath::Lerp(FootIKInfo.FootBoneStartLocation, FootIKInfo.FootBoneEndLocation, Alpha);
		FootIKInfo.FootBoneCurrentLocation = TargetLocation;
	} 
	
	for (auto& FootIKInfo : FootIKInfoArray)
	{
		if (FootIKInfo.GoupId != CurrentMoveGroupId)
			continue;
		
		if (FootIKInfo.FootBoneCurrentLocation.Equals(FootIKInfo.FootBoneEndLocation,0.01f) == false)
			return;
		
		//Remove tiny offset
		FootIKInfo.FootBoneCurrentLocation = FootIKInfo.FootBoneEndLocation;
		FootIKInfo.MoveTime = 0;
	}
	
	CurrentMoveGroupId = (CurrentMoveGroupId + 1) % LegGroups.Num();
	State = WaitMove;
}

void USpiderLegIKComponent::FindLandingLocation(const FVector& FootBonePlannedLocation, FVector& FootBoneResult, const FVector& FootMiddleLocation)
{
	for (int i = 0; i < 9; i++)
	{
		float Alpha = 1.0f - i * 0.1f;
		FVector StartPoint = FMath::Lerp(SpiderCharacter->GetActorLocation(), FootBonePlannedLocation, Alpha);
		StartPoint.Z = FootBonePlannedLocation.Z;
		
		FVector SweepStart = StartPoint + SpiderCharacter->GetActorUpVector() * 200.0f;
		FVector SweepEnd = StartPoint - SpiderCharacter->GetActorUpVector() * 200.0f;
		//DrawDebugLine(GetWorld(), SweepStart ,SweepEnd,  FColor::Red ,false,0.02f,0,2.0f);
		
		if (TrySweepHitPoint(SweepStart, SweepEnd, FootBoneResult) == true &&
			IsLegIntersectingObstacle(FootBoneResult, FootMiddleLocation) == false && 
			FVector::Distance(SpiderCharacter->GetActorLocation(), FootBoneResult) <= LegMaxLength)
		{
			//DrawDebugSphere(GetWorld(), FootBoneResult,7.0f,8, FColor::Black,false,3.f);
			return;
		}
	}
	
	FVector SweepEnd = SpiderCharacter->GetActorLocation();
	SweepEnd.Z = FootBonePlannedLocation.Z;
	if (TrySweepHitPoint(FootBonePlannedLocation, SweepEnd, FootBoneResult) == true &&
		IsLegIntersectingObstacle(FootBoneResult, FootMiddleLocation) == false)
		return;
	
	if (TrySweepHitPoint(FootBonePlannedLocation, SpiderCharacter->GetActorLocation(), FootBoneResult) == true && 
		IsLegIntersectingObstacle(FootBoneResult, FootMiddleLocation) == false)
		return;
	
	if (TrySweepHitPoint(SpiderCharacter->GetActorLocation(), FootBonePlannedLocation, FootBoneResult) == true && 
		IsLegIntersectingObstacle(FootBoneResult, FootMiddleLocation) == false)
		return;
}

bool USpiderLegIKComponent::TrySweepHitPoint(const FVector& StartPoint, const FVector& EndPoint, FVector& PointResult)
{
	FHitResult HitResult;
	GetWorld()->SweepSingleByObjectType(HitResult, StartPoint, EndPoint,FQuat::Identity,ObjectQueryParams, FCollisionShape::MakeSphere(3.0f), QueryParams);
	if (HitResult.bBlockingHit > 0)
	{
		PointResult = HitResult.ImpactPoint;
		return true;
	}
	return false;
}

//Test if any part of leg intersecting obstacles.
bool USpiderLegIKComponent::IsLegIntersectingObstacle(const FVector& LandingPoint, const FVector& FootMiddleLocation)
{
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByObjectType(HitResult, SpiderCharacter->GetActorLocation(), FootMiddleLocation, ObjectQueryParams, QueryParams);
	if (HitResult.bBlockingHit > 0)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Location: %s"), *MiddJointPoint.ToString());
		DrawDebugLine(GetWorld(), SpiderCharacter->GetActorLocation() ,FootMiddleLocation,  FColor::Black,false,0.02f,0,2.0f);
		return true;
	}
	
	FVector EndPoint = FMath::Lerp(FootMiddleLocation, LandingPoint, 0.98f);
	GetWorld()->LineTraceSingleByObjectType(HitResult, FootMiddleLocation, EndPoint, ObjectQueryParams, QueryParams);
	if (HitResult.bBlockingHit > 0)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Location: %s"), *MiddJointPoint.ToString());
		DrawDebugLine(GetWorld(), FootMiddleLocation, EndPoint, FColor::Cyan,false,0.02f,0,2.0f);
		return true;
	}
	
	return false;
}

void USpiderLegIKComponent::UpdatePlannedLocations(float DeltaTime)
{
	USkeletalMeshComponent* SkeletalMeshComp = Cast<ACharacter>(GetOwner())->GetMesh();
	for (auto& FootIKInfo : FootIKInfoArray)
	{
		FootIKInfo.FootBonePlannedLocation = SpiderCharacter->GetMesh()->GetComponentTransform().TransformPosition(FootIKInfo.FootDefeaultOffset);
		FootIKInfo.FootBonePlannedLocation += SpiderCharacter->GetCharacterMovement()->Velocity * DeltaTime;
		
		FootIKInfo.FootMiddleLocation = SpiderCharacter->GetMesh()->GetComponentTransform().TransformPosition(FootIKInfo.FootMiddleOffset);
		FootIKInfo.FootMiddleLocation += SpiderCharacter->GetCharacterMovement()->Velocity * DeltaTime;
	}
}

FVector USpiderLegIKComponent::GetWorldLocation(FVector LocalLocation)
{
	USkeletalMeshComponent* SkeletalMeshComp = SpiderCharacter->GetMesh();
	return SkeletalMeshComp->GetComponentTransform().TransformPosition(LocalLocation);
}

void USpiderLegIKComponent::DebugFootIKInfo()
{
	for (auto FootIKInfo : FootIKInfoArray)
	{
		DrawDebugSphere(GetWorld(), FootIKInfo.FootBonePlannedLocation,3.0f,8,FColor::Red,false,0.01f);
		DrawDebugSphere(GetWorld(), FootIKInfo.FootBoneCurrentLocation,3.0f,8,FColor::Blue,false,0.01f);
		DrawDebugSphere(GetWorld(), FootIKInfo.FootMiddleLocation,3.0f,8,FColor::Cyan,false,0.01f);
	}
}

