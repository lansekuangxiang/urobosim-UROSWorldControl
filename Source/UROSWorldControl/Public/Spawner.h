// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ROSBridgeSrvServer.h"
#include "ROSBridgeHandler.h"
#include "SpawnModel.h"
#include "ROSWorldControlManager.h"
#include "Spawner.generated.h"


struct SpawnAssetParams {
	FString PathOfMesh;
	FString PathOfMaterial;
	FVector Location;
	FRotator Rotator;
	TArray<UTagMsg> Tags;
};



UCLASS()
class UROSWORLDCONTROL_API ASpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawner();	
	AROSWorldControlManager* Controller;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Spawns a StaticMeshComponent at the given Location, Rotation and Material.
	//Object pointed to by PathOfMesh has to be a StaticMeshComponent.
	//Object pointed to by PathOfMaterial has to be a UMaterial.
	bool SpawnAsset(const FString PathOfMesh, const FString PathOfMaterial, const FVector Location, const FRotator Rotation, const TArray<UTagMsg> Tags);




	class FROSSpawnMeshServer final : public FROSBridgeSrvServer
	{
	private: 
		ASpawner* Parent;
		bool GameThreadDoneFlag;
		bool ServiceSuccess;


		void SetGameThreadDoneFlag(bool Flag);
		void SetServiceSuccess(bool Success);

	public:
		FROSSpawnMeshServer(FString Namespace, FString Name, ASpawner* InParent) :
			FROSBridgeSrvServer(Namespace + TEXT("/") + Name, TEXT("unreal_msgs/spawn_model"))
		{
			Parent = InParent;
		}
		

		TSharedPtr<FROSBridgeSrv::SrvRequest> FromJson(TSharedPtr<FJsonObject> JsonObject) const override;

		TSharedPtr<FROSBridgeSrv::SrvResponse> Callback(TSharedPtr<FROSBridgeSrv::SrvRequest> Request) override;


	};

private:


	UStaticMesh * LoadMesh(const FString Path);

	UMaterialInterface * LoadMaterial(const FString Path);
};