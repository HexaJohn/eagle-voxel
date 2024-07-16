// TestSphere.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "TestSphere.generated.h"

UCLASS()
class ATestSphere : public AActor
{
	GENERATED_BODY()

public:
	ATestSphere();
	static constexpr int32 CHUNK_SIZE = 16;
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                    FVector NormalImpulse, const FHitResult& Hit);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(EditAnywhere, Category = "Voxel Sphere")
	float SphereRadius = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Voxel Sphere")
	float VoxelSize = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Voxel Sphere")
	AActor* DamagedBy;

	UPROPERTY(EditAnywhere, Category = "Voxel Sphere")
	UStaticMesh* CubeMesh;

	UPROPERTY(EditAnywhere, Category = "Voxel Sphere")
	UMaterialInterface* CubeMaterial;

	TArray<TArray<TArray<FString>>> VoxelData;

	void GenerateVoxelSphere();
	void CreateCube(FVector Position, float Size, TArray<FVector>& Vertices, TArray<int32>& Triangles,
	                TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& TriangleOffset,
	                const bool FaceVisibility[6]);
	void GenerateChunk(FVector ChunkPosition);
	void GenerateVoxelMeshFromChunk();
	void RemoveVoxel(int32 X, int32 Y, int32 Z);
	void RegenerateMesh();
	void SpawnPhysicsCube(FVector Position);
};
