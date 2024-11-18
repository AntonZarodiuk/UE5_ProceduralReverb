// Fill out your copyright notice in the Description page of Project Settings.


#include "PR_PartitionWorldSubsystem.h"

#include "EngineUtils.h"
#include "PR_BSPNode.h"
#include "PhysicsEngine/BodySetup.h"
#include "NNE.h"
#include "NNEModelData.h"
#include "NNERuntimeORT/Private/NNERuntimeORT.h"
#include "Settings/ProceduralReverbSettings.h"

void UPR_PartitionWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	Generate();

	// TODO: Move to Actor Component?
	TObjectPtr<UNNEModelData> ModelData = GetDefault<UProceduralReverbSettings>()->PreLoadedModelData.LoadSynchronous();
	if (!IsValid(ModelData))
	{
		return;
	}

	TWeakInterfacePtr<INNERuntimeCPU> Runtime = UE::NNE::GetRuntime<INNERuntimeCPU>(FString("NNERuntimeORTCpu"));
	if (!Runtime.IsValid())
	{
		return;
	}

	TSharedPtr<UE::NNE::IModelCPU> Model = Runtime->CreateModelCPU(ModelData);
	if (!Model.IsValid())
	{
		return;
	}

	TSharedPtr<UE::NNE::IModelInstanceCPU> ModelInstance = Model->CreateModelInstanceCPU();

	TConstArrayView<UE::NNE::FTensorDesc> InputTensorDescs = ModelInstance->GetInputTensorDescs();
	checkf(InputTensorDescs.Num() == 1, TEXT("The current example supports only models with a single input tensor"));
	UE::NNE::FSymbolicTensorShape SymbolicInputTensorShape = InputTensorDescs[0].GetShape();
	checkf(SymbolicInputTensorShape.IsConcrete(),
			TEXT("The current example supports only models without variable input tensor dimensions"));
	TArray<UE::NNE::FTensorShape> InputTensorShapes = {
		UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicInputTensorShape)
	};

	ModelInstance->SetInputTensorShapes(InputTensorShapes);

	TConstArrayView<UE::NNE::FTensorDesc> OutputTensorDescs = ModelInstance->GetOutputTensorDescs();
	checkf(OutputTensorDescs.Num() == 1, TEXT("The current example supports only models with a single output tensor"));
	UE::NNE::FSymbolicTensorShape SymbolicOutputTensorShape = OutputTensorDescs[0].GetShape();
	checkf(SymbolicOutputTensorShape.IsConcrete(),
			TEXT("The current example supports only models without variable output tensor dimensions"));
	TArray<UE::NNE::FTensorShape> OutputTensorShapes = {
		UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicOutputTensorShape)
	};

	if (RootNode)
	{
		RootNode->RunModel(ModelInstance, InputTensorShapes, OutputTensorShapes);
	}
}

void UPR_PartitionWorldSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (RootNode)
	{
		// RootNode->DrawDebug(GetWorld());
	}
}

TStatId UPR_PartitionWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPCGSubsystem, STATGROUP_Tickables);
}

void UPR_PartitionWorldSubsystem::Generate()
{
	GenerateBSPTree(GetInitialBoundingBox());

	if (RootNode.IsValid())
	{
		RootNode->CollectAcousticData(GetWorld());
	}
}

FBox UPR_PartitionWorldSubsystem::GetInitialBoundingBox() const
{
	FBox ResultBox;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TArray<UStaticMeshComponent*> Components;
		It->GetComponents<UStaticMeshComponent>(Components);

		for (const UStaticMeshComponent* Component : Components)
		{
			if (Component->Mobility != EComponentMobility::Static)
			{
				continue;
			}

			auto StaticMesh = Component->GetStaticMesh();
			if (!IsValid(StaticMesh))
			{
				continue;
			}

			FTransform ComponentToWorld = Component->GetComponentTransform();

			if (StaticMesh->GetRenderData())
			{
				FStaticMeshLODResources& LODResource = StaticMesh->GetRenderData()->LODResources[0];
				const FPositionVertexBuffer& VertexBuffer = LODResource.VertexBuffers.PositionVertexBuffer;

				int32 VertexCount = VertexBuffer.GetNumVertices();
				TArray<FVector> Vertices;
				Vertices.Reserve(VertexCount);
				for (int32 i = 0; i < VertexCount; i++)
				{
					FVector VertexPosition = ComponentToWorld.
						TransformPosition(FVector(VertexBuffer.VertexPosition(i)));
					ResultBox += VertexPosition;
				}
			}
		}
	}

	return ResultBox;
}

void UPR_PartitionWorldSubsystem::GenerateBSPTree(const FBox& InitialBox)
{
	if (!InitialBox.IsValid)
	{
		return;
	}

	RootNode = MakeShared<FPR_BSPNode>(InitialBox);
	RootNode->PartitionSpace(0);
}

void UPR_PartitionWorldSubsystem::FindNearbyNodes(const FVector& Position, float SearchRadius,
												TArray<TSharedPtr<FPR_BSPNode>>& OutNearbyNodes) const
{
	if (RootNode)
	{
		RootNode->FindNearbyNodes(Position, SearchRadius, OutNearbyNodes);
	}
}
