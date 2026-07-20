#pragma once

#include <map>
#include <vector>
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpiderLegIKComponent.generated.h"

USTRUCT(BlueprintType)
struct FSpiderLegGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spider|IK")
	TArray<FName> FootBoneNames;
};

USTRUCT(BlueprintType)
struct FFootIKInfo
{
	GENERATED_BODY()
	
	int GoupId;
	//FVector FootToMiddleOffset;
	FVector FootMiddleOffset;
	FVector FootMiddleLocation;
	
	FVector FootDefeaultOffset;
	FVector FootBonePlannedLocation;
	
	FVector FootBoneStartLocation;
	FVector FootBoneEndLocation;
	
	float MoveTime;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName FootBoneName;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector FootBoneCurrentLocation;
};

enum FootState : uint8
{
	WaitMove = 0,
	Moving,
	Adjusting
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPIDERMOVE_API USpiderLegIKComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpiderLegIKComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	void CreateFootIKInfo();
	
private:
	void UpdatePlannedLocations(float DeltaTime);
	FVector GetWorldLocation(FVector LocalLocation);
	void DebugFootIKInfo();
	void TryStartMoveFeet();
	void UpdateFeetMovement(float DeltaTime, float Speed = 1.f);
	void FindLandingLocation(const FVector& FootBonePlannedLocation, FVector& FootBoneResult, const FVector& MiddleBoneLocation);
	
	bool TrySweepHitPoint(const FVector& StartPoint, const FVector& EndPoint, FVector& PointResult);
	bool IsLegIntersectingObstacle(const FVector& LandingPoint, const FVector& MiddJointPoint);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spider|IK")
	TArray<FSpiderLegGroup> LegGroups;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spider|IK")
	float LegMoveTime = 0.15f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spider|IK")
	float StepThreshold = 40.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spider|IK")
	float LegMaxLength = 220.f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spider|IK")
	TArray<FFootIKInfo> FootIKInfoArray;
	
private:
	FootState State;
	ACharacter* SpiderCharacter = nullptr;
	int CurrentMoveGroupId;
	FVector LastVelocity;
	FCollisionObjectQueryParams ObjectQueryParams;
	FCollisionQueryParams QueryParams;
};
