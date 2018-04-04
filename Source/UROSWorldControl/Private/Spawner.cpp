// Fill out your copyright notice in the Description page of Project Settings.

#include "Spawner.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "TagStatics.h"
#include "HelperFunctions.h"
#include <string>
#include <algorithm>

// Sets default values
ASpawner::ASpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASpawner::BeginPlay()
{

	// DEBUG
	/*SpawnAsset(
		TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"),
		TEXT("Material'/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial'"),
		FVector(130, 200, 300),
		FRotator::ZeroRotator,
		TArray<UTagMsg>::TArray()
	);*/
		
}




// Called every frame
void ASpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
		
}


bool ASpawner::SpawnAsset(const FString PathToMesh, const FString PathOfMaterial, const FVector Location, const FRotator Rotation, const TArray<unreal_msgs::Tag> Tags, unreal_msgs::InstanceId* InstanceId )
{
	//Check if Path is empty
	if (PathToMesh.IsEmpty()) {
		UE_LOG(LogTemp, Warning, TEXT("Path to the spawning asset, was empty."));
		return false;
	}

	UWorld* World = GetWorld();
	//Check if World is avialable
	if (!World) {
		UE_LOG(LogTemp, Warning, TEXT("Couldn't find the World."));
		return false;
	}

	//Setup SpawnParameters 
	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = Instigator;
	SpawnParams.Owner = this;


	//Load Mesh and check if it succeded.
	UStaticMesh* Mesh = LoadMesh(PathToMesh);
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Path does not point to a static mesh"));
		return false;
	}


	//Load Material and check if it succeded
	UMaterialInterface* Material = LoadMaterial(PathOfMaterial);
	if (!Material)
	{
		UE_LOG(LogTemp, Warning, TEXT("Path does not point to a Material"));
		return false;
	}

	AStaticMeshActor* SpawnedItem;

	FString Name = InstanceId->GetModelClassName();
	if (InstanceId->GetId().IsEmpty())
	{
		//ID needs to be generated
		FString Id = GenerateId(4);
		InstanceId->SetId(Id);
	}

	FString UniqueId = UROSWorldControlHelper::GetUniqueIdOfInstanceID(InstanceId);

	if (Controller->IdToActorMap.Find(UniqueId) == nullptr)
	{
		//Actual Spawning MeshComponent
		SpawnedItem = World->SpawnActor<AStaticMeshActor>(Location, Rotation, SpawnParams);
		//Assigning the Mesh and Material to the Component
		SpawnedItem->SetMobility(EComponentMobility::Movable);
		SpawnedItem->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		SpawnedItem->GetStaticMeshComponent()->SetMaterial(0, Material);

		//Add this object to id refrence map
		Controller->IdToActorMap.Add(UniqueId, SpawnedItem);
	}
	else
	{
		//ID is already taken
		UE_LOG(LogTemp, Warning, TEXT("Semlog id: \"%s\" is not unique, therefore nothing was spawned."), *UniqueId);
		return false;
	}

	//Id tag to Actor
	FTagStatics::AddKeyValuePair(
		SpawnedItem,
		TEXT("SemLog"),
		TEXT("id"),
		UniqueId);
	
	
	//Add other Tags to Actor
	for (auto Tag : Tags)
	{
		FTagStatics::AddKeyValuePair(
			SpawnedItem,
			Tag.GetTagType(),
			Tag.GetKey(),
			Tag.GetValue());
	}
	
	
	/*
		//Check if current Tag ist SemLog;id
		if (Tag.GetTagType().Equals(TEXT("SemLog")) && Tag.GetKey().Equals(TEXT("id"))) {
			bHasId = true;
			// check if ID was already taken
			if (Controller->IdToActorMap.Find(Tag.GetValue()) == nullptr) 
			{
				//Assigning the Mesh and Material to the Component
				SpawnedItem->SetMobility(EComponentMobility::Movable);
				SpawnedItem->GetStaticMeshComponent()->SetStaticMesh(Mesh);
				SpawnedItem->GetStaticMeshComponent()->SetMaterial(0, Material);

				//Add this object to id refrence map
				Controller->IdToActorMap.Add(Tag.GetValue(), SpawnedItem);
			}
			else
			{
				//ID is already taken
				//Spawing and then destroying object avoids iterating over tags twice.
				//since this code should rarely trigger this should be faster compared to extra iterations on every spawn 
				SpawnedItem->Destroy();
				UE_LOG(LogTemp, Warning, TEXT("Semlog id: \"%s\" is not unique, therefore nothing was spawned."), *Tag.GetValue());
				return false;
			}
		}
	}
			*/		

	return true;
}

UStaticMesh * ASpawner::LoadMesh(const FString Path)
{
	UStaticMesh* Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *Path));
	return Mesh;
}

UMaterialInterface * ASpawner::LoadMaterial(const FString Path)
{
	UMaterialInterface* Material = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *Path));
	return Material;
}



TSharedPtr<FROSBridgeSrv::SrvRequest> ASpawner::FROSSpawnMeshServer::FromJson(TSharedPtr<FJsonObject> JsonObject) const
{
	TSharedPtr<FROSBridgeSpawnServiceSrv::Request> Request_ =
		MakeShareable(new FROSBridgeSpawnServiceSrv::Request());
	Request_->FromJson(JsonObject);
	return TSharedPtr<FROSBridgeSrv::SrvRequest>(Request_);
}

TSharedPtr<FROSBridgeSrv::SrvResponse> ASpawner::FROSSpawnMeshServer::Callback(TSharedPtr<FROSBridgeSrv::SrvRequest> Request) 
{
	
	TSharedPtr<FROSBridgeSpawnServiceSrv::Request> SpawnMeshRequest =
		StaticCastSharedPtr<FROSBridgeSpawnServiceSrv::Request>(Request);

	SpawnAssetParams Params;
	Params.PathOfMesh = SpawnMeshRequest->GetModelDescription().GetMeshDescription().GetPathToMesh();
	Params.PathOfMaterial = SpawnMeshRequest->GetModelDescription().GetMeshDescription().GetPathToMaterial();
	Params.Location = SpawnMeshRequest->GetModelDescription().GetPose().GetPosition().GetVector();
	Params.Rotator = FRotator(SpawnMeshRequest->GetModelDescription().GetPose().GetOrientation().GetQuat());
	Params.Tags = SpawnMeshRequest->GetModelDescription().GetTags();
	unreal_msgs::InstanceId Id = SpawnMeshRequest->GetModelDescription().GetInstanceId();
	Params.InstanceId = &Id;
	
	GameThreadDoneFlag = false;
	// Execute on game thread
	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		bool success = Parent->SpawnAsset(Params.PathOfMesh,
			Params.PathOfMaterial,
			Params.Location,
			Params.Rotator,
			Params.Tags,
			Params.InstanceId);
		SetServiceSuccess(success);
		SetGameThreadDoneFlag(true);
	}
	);

	// Wait for gamethread to be done
	while (!GameThreadDoneFlag) {
		FPlatformProcess::Sleep(0.01);
	}

	return MakeShareable<FROSBridgeSrv::SrvResponse>
		(new FROSBridgeSpawnServiceSrv::Response(ServiceSuccess, Id));
}

void ASpawner::FROSSpawnMeshServer::SetGameThreadDoneFlag(bool Flag)
{
	GameThreadDoneFlag = Flag;
}

void ASpawner::FROSSpawnMeshServer::SetServiceSuccess(bool bSuccess)
{
	ServiceSuccess = bSuccess;
}

void ASpawner::FROSSpawnSemanticMapServer::SetGameThreadDoneFlag(bool Flag)
{
	GameThreadDoneFlag = Flag;
}

void ASpawner::FROSSpawnSemanticMapServer::SetServiceSuccess(bool bSuccess)
{
	ServiceSuccess = bSuccess;
}

FString FORCEINLINE ASpawner::GenerateId(int Length) {
	auto RandChar = []() -> char
	{
		const char CharSet[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
		const size_t MaxIndex = (sizeof(CharSet) - 1);
		return CharSet[rand() % MaxIndex];
	};
	std::string RandString(Length, 0);
	std::generate_n(RandString.begin(), Length, RandChar);
	// Return as Fstring
	return FString(RandString.c_str());
}

TSharedPtr<FROSBridgeSrv::SrvRequest> ASpawner::FROSSpawnSemanticMapServer::FromJson(TSharedPtr<FJsonObject> JsonObject) const
{
	TSharedPtr<FROSBridgeSpawnMultipleModelsSrv::Request> Request_ =
		MakeShareable(new FROSBridgeSpawnMultipleModelsSrv::Request());
	Request_->FromJson(JsonObject);
	return TSharedPtr<FROSBridgeSrv::SrvRequest>(Request_);
}

TSharedPtr<FROSBridgeSrv::SrvResponse> ASpawner::FROSSpawnSemanticMapServer::Callback(TSharedPtr<FROSBridgeSrv::SrvRequest> Request)
{

	TSharedPtr<FROSBridgeSpawnMultipleModelsSrv::Request> SpawnSemanticMapRequest =
		StaticCastSharedPtr<FROSBridgeSpawnMultipleModelsSrv::Request>(Request);

	// Destroy all StaticMeshes to get clean level.
/*
	// Execute on game thread
	GameThreadDoneFlag = false;
	AsyncTask(ENamedThreads::GameThread, [=]()
	{	
		
		for (TActorIterator<AStaticMeshActor> ActorItr(Parent->GetWorld()); ActorItr; ++ActorItr)
		{
			ActorItr->Destroy();
		}
		Parent->Controller->IdToActorMap.Empty();
		SetGameThreadDoneFlag(true);
	}
	);

	// Wait for gamethread to be done
	while (!GameThreadDoneFlag) {
		FPlatformProcess::Sleep(0.01);
	}
	*/


	// Spawns everything in the SemanticMap
	TArray<unreal_msgs::ModelDescription>* Descriptions = SpawnSemanticMapRequest->GetModelDescriptions();

	TArray<bool> SuccessList;
	TArray<unreal_msgs::InstanceId> InstanceIds;

	bool bAllSucceded = true;
	for (auto Descript : *Descriptions) 
	{
		SpawnAssetParams Params;
		Params.PathOfMesh = Descript.GetMeshDescription().GetPathToMesh();
		Params.PathOfMaterial = Descript.GetMeshDescription().GetPathToMaterial();
		Params.Location = Descript.GetPose().GetPosition().GetVector();
		Params.Rotator = FRotator(Descript.GetPose().GetOrientation().GetQuat());
		Params.Tags = Descript.GetTags();
		unreal_msgs::InstanceId Id = Descript.GetInstanceId();
		Params.InstanceId = &Id;

		GameThreadDoneFlag = false;
		// Execute on game thread
		AsyncTask(ENamedThreads::GameThread, [=]()
		{
			bool success = Parent->SpawnAsset(Params.PathOfMesh,
				Params.PathOfMaterial,
				Params.Location,
				Params.Rotator,
				Params.Tags,
				Params.InstanceId);
			SetServiceSuccess(success);
			SetGameThreadDoneFlag(true);
		}
		);

		// Wait for gamethread to be done
		while (!GameThreadDoneFlag) {
			FPlatformProcess::Sleep(0.01);
		}
		InstanceIds.Add(Id);
		SuccessList.Add(ServiceSuccess);
	}

	//TODO fill this
	return MakeShareable<FROSBridgeSrv::SrvResponse>
		(new FROSBridgeSpawnMultipleModelsSrv::Response(SuccessList, InstanceIds));



}
