// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "AStarPathGenerator.generated.h"

 
USTRUCT() struct FMapNode
{
	GENERATED_BODY()

	bool bObstacle = false;			// Is the node an obstruction?
	bool bVisited = false;			// Have we searched this node before?
	float fGlobalGoal;				// Distance to goal so far
	float fLocalGoal;				// Distance to goal if we took the alternative route
	int x;							// Nodes position in 2D space
	int y;

	TArray<FMapNode*> vecNeighbours;	// Connections to neighbours
	FMapNode* parent;					// Node connecting to this node that offers shortest parent

	FMapNode()
	{
	}

	FMapNode(int x,int y)
	{
		// Always initialize your USTRUCT variables!
		// exception is if you know the variable type has its own default 
		this->x = x;
		this->y = y;
	}

};

UCLASS()
class ASTARPLUGIN_API AAStarPathGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAStarPathGenerator();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		float MapWidth = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		float MapHeight = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		float TileWidth = 100;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		float TileHeight = 100;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		int NumberObstacles = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		AActor* MapReference;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		AActor* ObstacleReference;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		AActor* PathSignalingReference;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		AActor* StartReference;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General Parameters")
		AActor* EndReference;

	UFUNCTION(BlueprintCallable, Category="AStarPathGenerator")
	void SolveAStar();

	UFUNCTION(BlueprintCallable, Category = "AStarPathGenerator")
	void ShowSolution();

	bool ValidateReferences();
	void SpawnInMapTile(AActor*, int x, int y);
	void SpawnInMap(AActor*, FVector, FRotator);
	FVector PositionInMap(int x, int y);
	FVector CenterToTile(FVector);
	

private:
	bool gridCreated = false;
	FVector MapLocation;
	TArray<FMapNode> MapNodes;
	int nMapWidth;
	int nMapHeight;
	FMapNode* nodeStart = nullptr;
	FMapNode* nodeEnd = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
