// Fill out your copyright notice in the Description page of Project Settings.


#include "AStarPathGenerator.h"

// Sets default values
AAStarPathGenerator::AAStarPathGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AAStarPathGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (ValidateReferences())
	{
		// Initializing variables
		MapLocation = MapReference->GetActorLocation();
		nMapWidth = MapWidth / TileWidth;
		nMapHeight = MapHeight / TileHeight;

		// Building the 2D grid that represents the map

		for (int y = 0; y < nMapHeight; y++)
			for (int x = 0; x < nMapWidth; x++)
			{
				FMapNode node = FMapNode(x, y); 
				node.bObstacle = false;
				node.parent = nullptr;
				node.bVisited = false;

				MapNodes.Add(node); 
			}

		// Create connections - in this case nodes are on a regular grid
		for (int x = 0; x < nMapWidth; x++)
			for (int y = 0; y < nMapHeight; y++)
			{
				if (y > 0)
					MapNodes[y * nMapWidth + x].vecNeighbours.Add(&MapNodes[(y - 1) * nMapWidth + (x + 0)]);
				if (y < nMapHeight - 1)
					MapNodes[y * nMapWidth + x].vecNeighbours.Add(&MapNodes[(y + 1) * nMapWidth + (x + 0)]);
				if (x > 0)
					MapNodes[y * nMapWidth + x].vecNeighbours.Add(&MapNodes[(y + 0) * nMapWidth + (x - 1)]);
				if (x < nMapWidth - 1)
					MapNodes[y * nMapWidth + x].vecNeighbours.Add(&MapNodes[(y + 0) * nMapWidth + (x + 1)]);
			}

		// Randomizing start and end position and showing them in the map
		auto randomMapPosition = [](int width, int height)
		{
			TPair<int, int> result;
			result.Key = FMath::RandRange(0, height - 1); //Y
			result.Value = FMath::RandRange(0, width - 1); //X

			return result;
		};
		TPair<int, int> startPos = randomMapPosition(nMapWidth, nMapHeight);
		nodeStart = &MapNodes[startPos.Key * nMapWidth + startPos.Value];
		SpawnInMapTile(StartReference, nodeStart->x, nodeStart->y);
	
		TPair<int, int> endPos = randomMapPosition(nMapWidth, nMapHeight);
		nodeEnd = &MapNodes[endPos.Key * nMapWidth + endPos.Value];		
		SpawnInMapTile(EndReference, nodeEnd->x, nodeEnd->y);

		// Spawning random obstacles across the map
		for (int i = 0; i < NumberObstacles; i++)
		{
			int x = FMath::RandRange(0, nMapWidth - 1);
			int y = FMath::RandRange(0, nMapHeight - 1);

			FMapNode* obstacle = &MapNodes[y * nMapWidth + x];
			// Avoid repeating start / end tiles
			if (obstacle != nodeStart && obstacle != nodeEnd)
			{				
				obstacle->bObstacle = true;
				SpawnInMapTile(ObstacleReference, obstacle->x, obstacle->y);
			}
			else
			{
				i--;
			}
		}
		gridCreated = true;
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("There is an invalid reference passed to AStarPathGenerator class."));
	}
}

bool AAStarPathGenerator::ValidateReferences()
{
	return IsValid(MapReference) && IsValid(ObstacleReference) && IsValid(StartReference) && IsValid(EndReference) && IsValid(PathSignalingReference);
}


// Return a position in the map based on X,Y coordinates and centered on the tile
FVector AAStarPathGenerator::PositionInMap(int x, int y)
{
	float offsetZ = 50.0f;
	FVector objectLocation = FVector(MapLocation.X + x * TileWidth, MapLocation.Y + y * TileHeight, MapLocation.Z + offsetZ);
	return CenterToTile(objectLocation); //return objectLocation to return location not centered
}


// Offset the position to its tile center
FVector AAStarPathGenerator::CenterToTile(FVector Location)
{
	return FVector(Location.X + TileWidth / 2, Location.Y + TileHeight / 2, Location.Z);
}

// Spawn a duplicate of AActor in tile X,Y
void AAStarPathGenerator::SpawnInMapTile(AActor* ObjectType, int x, int y)
{
	FVector location = PositionInMap(x, y);			
	FVector originRotated = MapReference->GetActorRotation().RotateVector(location - MapLocation);
	FVector fixedLocation = originRotated + MapLocation; //In case the map is rotated, rotate the object along with it aswell

	SpawnInMap(ObjectType, fixedLocation, MapReference->GetActorRotation());
}

// Spawn a duplicate of AActor in tile X,Y
void AAStarPathGenerator::SpawnInMap(AActor* ObjectType, FVector Location, FRotator Rotation)
{
	if (IsValid(ObjectType))
	{
		FTransform transform = FTransform(Location);

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Template = ObjectType;

		AActor* objectSpawned = GetWorld()->SpawnActor(ObjectType->GetClass(), &transform, SpawnInfo);
		objectSpawned->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		objectSpawned->SetActorLocationAndRotation(Location, Rotation);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Spawnable object reference invalid."));
	}
}

// Generate solution for current grid
void AAStarPathGenerator::SolveAStar()
{

	if (!gridCreated)
	{
		UE_LOG(LogTemp, Warning, TEXT("Inner grid has not been created. Can't apply AStar algorithm."));
		return;
	}

	// Reset Navigation Graph - default all node states
	for (int x = 0; x < nMapWidth; x++)
		for (int y = 0; y < nMapHeight; y++)
		{
			MapNodes[y * nMapWidth + x].bVisited = false;
			MapNodes[y * nMapWidth + x].fGlobalGoal = INFINITY;
			MapNodes[y * nMapWidth + x].fLocalGoal = INFINITY;
			MapNodes[y * nMapWidth + x].parent = nullptr;	// No parents
		}

	auto distance = [](FMapNode* a, FMapNode* b) // Euclidean distance (Pythagoras theorem)
	{
		return sqrtf((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y));
	};

	auto heuristic = [distance](FMapNode* a, FMapNode* b)
	{
		return distance(a, b);
	};

	// Setup starting conditions
	FMapNode* nodeCurrent = nodeStart;
	nodeStart->fLocalGoal = 0.0f;
	nodeStart->fGlobalGoal = heuristic(nodeStart, nodeEnd);

	// Add start node to not tested list
	TArray<FMapNode*> listNotTestedNodes;
	listNotTestedNodes.Add(nodeStart);

	// We will also stop searching when we reach the target (not the shortest path possible)
	while (!listNotTestedNodes.IsEmpty() && nodeCurrent != nodeEnd)
	{
		// Sort Untested nodes by global goal, so lowest is first
		Algo::Sort(listNotTestedNodes, [](const FMapNode* lhs, const FMapNode* rhs) { return lhs->fGlobalGoal < rhs->fGlobalGoal; });

		// Front of listNotTestedNodes is potentially the lowest distance node. Our
		// list may also contain nodes that have been visited, so ditch these...
		while (!listNotTestedNodes.IsEmpty() && listNotTestedNodes[0]->bVisited)
			listNotTestedNodes.RemoveAt(0); 

		// ...or abort because there are no valid nodes left to test
		if (listNotTestedNodes.IsEmpty())
			break;

		nodeCurrent = listNotTestedNodes[0];  
		nodeCurrent->bVisited = true; // We only explore a node once

		UE_LOG(LogTemp, Log, TEXT("Checking Neighbours Of X(%d) Y(%d)"), nodeCurrent->x, nodeCurrent->y);

		// Check each of this node's neighbours...
		for (auto nodeNeighbour : nodeCurrent->vecNeighbours)
		{
			UE_LOG(LogTemp, Log, TEXT("Neighbour at X(%d) Y(%d). isObstacle (%d)"), nodeNeighbour->x, nodeNeighbour->y, nodeNeighbour->bObstacle);

			// Only if the neighbour is not visited and is not an obstacle, add it to NotTested List
			if (!nodeNeighbour->bVisited && (nodeNeighbour->bObstacle == false))
			{
				UE_LOG(LogTemp, Log, TEXT("Adding Neighbour to NotTestedNodes list"));
				listNotTestedNodes.Add(nodeNeighbour);
			}

			// Calculate the neighbours potential lowest parent distance
			float fPossiblyLowerGoal = nodeCurrent->fLocalGoal + distance(nodeCurrent, nodeNeighbour);
			if (fPossiblyLowerGoal < nodeNeighbour->fLocalGoal)
			{
				nodeNeighbour->parent = nodeCurrent;
				nodeNeighbour->fLocalGoal = fPossiblyLowerGoal;
				nodeNeighbour->fGlobalGoal = nodeNeighbour->fLocalGoal + heuristic(nodeNeighbour, nodeEnd);
			}
		}

	}
}

// Draw Path by starting ath the end, and following the parent node trail
void AAStarPathGenerator::ShowSolution()
{
	if (!gridCreated)
	{
		UE_LOG(LogTemp, Warning, TEXT("Inner grid has not been created. Solution can't be displayed."));
		return;
	}		

	if (nodeEnd != nullptr)
	{
		FMapNode* p = nodeEnd;
		while (p->parent != nullptr)
		{			
			if (p != nodeEnd) //Don't spawn solution in the end node
				SpawnInMapTile(PathSignalingReference, p->x, p->y);
			
			p = p->parent;						
		}
	}
}


// Called every frame
void AAStarPathGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

