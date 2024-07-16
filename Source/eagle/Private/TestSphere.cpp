// TestSphere.cpp

#include "TestSphere.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "WorldPartition/ContentBundle/ContentBundleLog.h"

ATestSphere::ATestSphere()
{
	PrimaryActorTick.bCanEverTick = false;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;

	// Load the static mesh and material
	// static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh/Game/Content/LevelPrototyping/Meshes/SM_ChamferCube"));
	// if (MeshAsset.Succeeded())
	// {
	//     CubeMesh = MeshAsset.Object;
	// }
	// else {
	//     UE_LOG(LogTemp, Warning, TEXT("MeshAsset failed"));
	// }
	//
	// static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Game/Content/LevelPrototyping/Materials/MI_Solid_Blue"));
	// if (MaterialAsset.Succeeded())
	// {
	//     CubeMaterial = MaterialAsset.Object;
	// }
}

void ATestSphere::BeginPlay()
{
	Super::BeginPlay();
	// GenerateVoxelSphere();
	GenerateChunk(FVector::ZeroVector); // Generate chunk at origin
	GenerateVoxelMeshFromChunk();


	ProceduralMesh->SetNotifyRigidBodyCollision(true);
	ProceduralMesh->OnComponentHit.AddDynamic(this, &ATestSphere::OnComponentHit);
}

void ATestSphere::GenerateVoxelSphere()
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	const TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	int32 TriangleOffset = 0;

	int32 VoxelCount = FMath::CeilToInt(SphereRadius * 2 / VoxelSize);

	// Create a 3D array to store which voxels are inside the sphere
	TArray<TArray<TArray<bool>>> VoxelGrid;
	VoxelGrid.SetNum(VoxelCount);
	for (auto& YZ : VoxelGrid)
	{
		YZ.SetNum(VoxelCount);
		for (auto& Z : YZ)
		{
			Z.SetNum(VoxelCount);
		}
	}

	// First pass: determine which voxels are inside the sphere
	for (int x = 0; x < VoxelCount; x++)
	{
		for (int y = 0; y < VoxelCount; y++)
		{
			for (int z = 0; z < VoxelCount; z++)
			{
				FVector Position((x - VoxelCount / 2) * VoxelSize,
				                 (y - VoxelCount / 2) * VoxelSize,
				                 (z - VoxelCount / 2) * VoxelSize);

				VoxelGrid[x][y][z] = Position.Size() <= SphereRadius;
			}
		}
	}

	// Second pass: create visible faces
	for (int x = 0; x < VoxelCount; x++)
	{
		for (int y = 0; y < VoxelCount; y++)
		{
			for (int z = 0; z < VoxelCount; z++)
			{
				if (VoxelGrid[x][y][z])
				{
					FVector Position((x - VoxelCount / 2) * VoxelSize,
					                 (y - VoxelCount / 2) * VoxelSize,
					                 (z - VoxelCount / 2) * VoxelSize);

					bool FaceVisibility[6] = {
						x == 0 || !VoxelGrid[x - 1][y][z], // Left
						x == VoxelCount - 1 || !VoxelGrid[x + 1][y][z], // Right
						y == 0 || !VoxelGrid[x][y - 1][z], // Front
						y == VoxelCount - 1 || !VoxelGrid[x][y + 1][z], // Back
						z == 0 || !VoxelGrid[x][y][z - 1], // Bottom
						z == VoxelCount - 1 || !VoxelGrid[x][y][z + 1] // Top
					};

					CreateCube(Position, VoxelSize, Vertices, Triangles, Normals, UVs, TriangleOffset, FaceVisibility);
				}
			}
		}
	}

	ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
}

void ATestSphere::CreateCube(FVector Position, float Size, TArray<FVector>& Vertices, TArray<int32>& Triangles,
                             TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& TriangleOffset,
                             const bool FaceVisibility[6])
{
	FVector v0 = Position;
	FVector v1 = Position + FVector(Size, 0, 0);
	FVector v2 = Position + FVector(Size, Size, 0);
	FVector v3 = Position + FVector(0, Size, 0);
	FVector v4 = Position + FVector(0, 0, Size);
	FVector v5 = Position + FVector(Size, 0, Size);
	FVector v6 = Position + FVector(Size, Size, Size);
	FVector v7 = Position + FVector(0, Size, Size);

	TArray<FVector> CubeVertices = {v0, v1, v2, v3, v4, v5, v6, v7};

	// Define the faces: 
	// 0: Left, 1: Right, 2: Front, 3: Back, 4: Bottom, 5: Top
	int32 Faces[6][4] = {
		{0, 3, 7, 4}, // Left
		{1, 5, 6, 2}, // Right
		{0, 4, 5, 1}, // Front
		{3, 2, 6, 7}, // Back
		{0, 1, 2, 3}, // Bottom
		{4, 7, 6, 5} // Top
	};

	for (int i = 0; i < 6; i++)
	{
		if (FaceVisibility[i])
		{
			int32 VertexOffset = Vertices.Num();
			for (int j = 0; j < 4; j++)
			{
				Vertices.Add(CubeVertices[Faces[i][j]]);

				// Add normal
				FVector Normal(0, 0, 0);
				Normal[i / 2] = i % 2 == 0 ? -1 : 1;
				Normals.Add(Normal);

				// Add UV
				UVs.Add(FVector2D(j == 1 || j == 2, j == 2 || j == 3));
			}

			// Add triangles in clockwise order
			Triangles.Add(VertexOffset);
			Triangles.Add(VertexOffset + 1);
			Triangles.Add(VertexOffset + 2);
			Triangles.Add(VertexOffset);
			Triangles.Add(VertexOffset + 2);
			Triangles.Add(VertexOffset + 3);
		}
	}

	TriangleOffset = Vertices.Num();
}

void ATestSphere::GenerateChunk(FVector ChunkPosition)
{
	VoxelData.SetNum(CHUNK_SIZE);
	for (auto& YZ : VoxelData)
	{
		YZ.SetNum(CHUNK_SIZE);
		for (auto& Z : YZ)
		{
			Z.SetNum(CHUNK_SIZE);
		}
	}

	// Example: Generate a simple sphere in the chunk
	FVector ChunkCenter(CHUNK_SIZE / 2, CHUNK_SIZE / 2, CHUNK_SIZE / 2);
	float Radius = SphereRadius;

	for (int32 x = 0; x < CHUNK_SIZE; x++)
	{
		for (int32 y = 0; y < CHUNK_SIZE; y++)
		{
			for (int32 z = 0; z < CHUNK_SIZE; z++)
			{
				FVector Position(x, y, z);
				if ((Position - ChunkCenter).Size() <= Radius)
				{
					VoxelData[x][y][z] = "Stone"; // Example voxel type
				}
				// else it remains an empty string
			}
		}
	}
}

void ATestSphere::GenerateVoxelMeshFromChunk()
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	int32 TriangleOffset = 0;

	for (int32 x = 0; x < CHUNK_SIZE; x++)
	{
		for (int32 y = 0; y < CHUNK_SIZE; y++)
		{
			for (int32 z = 0; z < CHUNK_SIZE; z++)
			{
				if (!VoxelData[x][y][z].IsEmpty())
				{
					FVector Position(x * VoxelSize, y * VoxelSize, z * VoxelSize);

					bool FaceVisibility[6] = {
						x == 0 || VoxelData[x - 1][y][z].IsEmpty(),
						x == CHUNK_SIZE - 1 || VoxelData[x + 1][y][z].IsEmpty(),
						y == 0 || VoxelData[x][y - 1][z].IsEmpty(),
						y == CHUNK_SIZE - 1 || VoxelData[x][y + 1][z].IsEmpty(),
						z == 0 || VoxelData[x][y][z - 1].IsEmpty(),
						z == CHUNK_SIZE - 1 || VoxelData[x][y][z + 1].IsEmpty()
					};
					CreateCube(Position, VoxelSize, Vertices, Triangles, Normals, UVs, TriangleOffset, FaceVisibility);
				}
			}
		}
	}

	ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(),
	                                              TArray<FProcMeshTangent>(), true);
}

void ATestSphere::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                 FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("Hit!"));
	if (OtherActor && OtherActor->IsRootComponentMovable() && OtherActor->IsA(DamagedBy->GetClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Locating!"));
		FVector GlobalHitLocation = OtherActor->GetActorLocation();
		FVector LocalHitLocation = ProceduralMesh->GetComponentTransform().InverseTransformPosition(Hit.Location);
		int32 X = FMath::FloorToInt(LocalHitLocation.X / VoxelSize);
		int32 Y = FMath::FloorToInt(LocalHitLocation.Y / VoxelSize);
		int32 Z = FMath::FloorToInt(LocalHitLocation.Z / VoxelSize);
		int32 OtherX = FMath::FloorToInt(GlobalHitLocation.X);
		int32 OtherY = FMath::FloorToInt(GlobalHitLocation.Y);
		int32 OtherZ = FMath::FloorToInt(GlobalHitLocation.Z);

		if (X >= 0 && X < CHUNK_SIZE && Y >= 0 && Y < CHUNK_SIZE && Z >= 0 && Z < CHUNK_SIZE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Removing: %d, %d, %d"), X, Y, Z);
			UE_LOG(LogTemp, Warning, TEXT("Hit At: %d, %d, %d"), OtherX, OtherY, OtherZ);
			FVector OtherActorLocation = OtherActor->GetActorTransform().GetLocation();
			RemoveVoxel(X, Y, Z);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Not Removing"));
	}
}

void ATestSphere::RemoveVoxel(int32 X, int32 Y, int32 Z)
{
	FVector WorldPosition = GetActorLocation() + FVector(X * VoxelSize * 100, Y * VoxelSize * 100, Z * VoxelSize * 100);
	if (VoxelData.Max() <= X && VoxelData.Max() <= Y && VoxelData.Max() <= Z)
	{
		if (!VoxelData[X][Y][Z].IsEmpty())
		{
			VoxelData[X][Y][Z] = FString();
			RegenerateMesh();
			UE_LOG(LogTemp, Warning, TEXT("Removed!"));
		}
		SpawnPhysicsCube(WorldPosition);
	}
}

void ATestSphere::RegenerateMesh()
{
	// Clear existing mesh data
	ProceduralMesh->ClearAllMeshSections();

	// Regenerate mesh using existing GenerateVoxelMeshFromChunk function
	GenerateVoxelMeshFromChunk();
}

void ATestSphere::SpawnPhysicsCube(FVector Position)
{
	if (CubeMesh && CubeMaterial)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AActor* CubeActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), Position, FRotator::ZeroRotator,
		                                                   SpawnParams);
		if (CubeActor)
		{
			UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(CubeActor);
			MeshComponent->SetStaticMesh(CubeMesh);
			MeshComponent->SetMaterial(0, CubeMaterial);
			MeshComponent->SetWorldScale3D(FVector(VoxelSize / 100.0f)); // Assuming the cube mesh is 100x100x100 units
			MeshComponent->SetSimulatePhysics(true);
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			CubeActor->SetRootComponent(MeshComponent);
			MeshComponent->RegisterComponent();
		}
	}
}
