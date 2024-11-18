#include "PR_BSPNode.h"

#include "NNERuntimeCPU.h"
#include "Settings/ProceduralReverbSettings.h"


void FPR_BSPNode::PartitionSpace(int32 Depth)
{
	auto* Settings = GetDefault<UProceduralReverbSettings>();
	if (Depth >= Settings->MaxPartitionDepth /*|| Node->BoundingBox.GetVolume() < MinVolume*/) {
		bIsLeaf = true;
		return;
	}

	FVector Center = BoundingBox.GetCenter();
	FVector Extents = BoundingBox.GetExtent();
	int Axis = (Extents.X >= Extents.Y && Extents.X >= Extents.Z) ? 0 : 
				(Extents.Y >= Extents.Z ? 1 : 2);

	FBox LeftBox, RightBox;
	// Split based on the Axis
	switch (Axis) {
		case 0:
			LeftBox = FBox(BoundingBox.Min, FVector(Center.X, BoundingBox.Max.Y, BoundingBox.Max.Z));
			RightBox = FBox(FVector(Center.X, BoundingBox.Min.Y, BoundingBox.Min.Z), BoundingBox.Max);
			break;
		case 1:
			LeftBox = FBox(BoundingBox.Min, FVector(BoundingBox.Max.X, Center.Y, BoundingBox.Max.Z));
			RightBox = FBox(FVector(BoundingBox.Min.X, Center.Y, BoundingBox.Min.Z), BoundingBox.Max);
			break;
		case 2:
			LeftBox = FBox(BoundingBox.Min, FVector(BoundingBox.Max.X, BoundingBox.Max.Y, Center.Z));
			RightBox = FBox(FVector(BoundingBox.Min.X, BoundingBox.Min.Y, Center.Z), BoundingBox.Max);
			break;
		default:
			ensure(false);
	}

	LeftChild = MakeShared<FPR_BSPNode>(LeftBox);
	RightChild = MakeShared<FPR_BSPNode>(RightBox);

	LeftChild->PartitionSpace(Depth + 1);
	RightChild->PartitionSpace(Depth + 1);
}

void FPR_BSPNode::CollectAcousticData(const UWorld* World)
{
	if (!bIsLeaf)
	{
		LeftChild->CollectAcousticData(World);
		RightChild->CollectAcousticData(World);
		return;
	}

	auto* Settings = GetDefault<UProceduralReverbSettings>();
	AcousticData = MakeShared<FPR_AcousticData>();
	FVector Start = BoundingBox.GetCenter();
	FVector Directions[] = {
		FVector(1, 0, 0), FVector(-1, 0, 0),  // X directions
		FVector(0, 1, 0), FVector(0, -1, 0),  // Y directions
		FVector(0, 0, 1), FVector(0, 0, -1)   // Z directions
	};

	for (const FVector& Direction : Directions) {
		FVector End = Start + (Direction * Settings->RayDistance);
		FHitResult Hit;
		World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility);

		float Distance = Settings->RayDistance;
		EPhysicalSurface SurfaceType = SurfaceType_Default;
		if (Hit.bBlockingHit) {
			Distance = (Hit.ImpactPoint - Start).Size();
			UPhysicalMaterial* Material = Hit.PhysMaterial.Get();
			SurfaceType = Material ? Material->SurfaceType.GetValue() : SurfaceType_Default;
			//UE_LOG(LogTemp, Log, TEXT("Found wall: %s - %f"), *UEnum::GetValueAsString(SurfaceType), Distance);
		}

		AcousticData->Distances.Add(Distance);
		AcousticData->Materials.Add(SurfaceType);
	}
}

TSharedPtr<FPR_BSPNode> FPR_BSPNode::FindNode(const FVector& Position)
{
	if (bIsLeaf)
	{
		return nullptr;
	}

	if (LeftChild->bIsLeaf && LeftChild->BoundingBox.IsInside(Position))
	{
		return LeftChild;
	}

	if (RightChild->bIsLeaf && RightChild->BoundingBox.IsInside(Position))
	{
		return RightChild;
	}


	if (TSharedPtr<FPR_BSPNode> Result = LeftChild->FindNode(Position))
	{
		return Result;
	}

	return RightChild->FindNode(Position);
}

void FPR_BSPNode::FindNearbyNodes(
	const FVector& Position,
	const float SearchRadius,
	TArray<TSharedPtr<FPR_BSPNode>>& OutNearbyNodes)
{
	if (bIsLeaf)
	{
		return;
	}

	if (LeftChild->bIsLeaf)
	{
		float Distance = LeftChild->DistanceTo(Position);
		if (Distance <= SearchRadius)
		{
			OutNearbyNodes.Add(LeftChild);
		}
	}

	if (RightChild->bIsLeaf)
	{
		float Distance = RightChild->DistanceTo(Position);
		if (Distance <= SearchRadius)
		{
			OutNearbyNodes.Add(RightChild);
		}
	}

	LeftChild->FindNearbyNodes(Position, SearchRadius, OutNearbyNodes);
	RightChild->FindNearbyNodes(Position, SearchRadius, OutNearbyNodes);
}

float FPR_BSPNode::DistanceTo(const FVector& Point) const
{
	const FVector ClosestPoint = BoundingBox.GetClosestPointTo(Point);
	return FVector::Distance(ClosestPoint, Point);
}

void FPR_BSPNode::RunModel(
	const TSharedPtr<UE::NNE::IModelInstanceCPU>& ModelInstance,
	const TArray<UE::NNE::FTensorShape>& InputTensorShapes,
	const TArray<UE::NNE::FTensorShape>& OutputTensorShapes) const
{
	if (!AcousticData)
	{
		if (LeftChild)
		{
			LeftChild->RunModel(ModelInstance, InputTensorShapes, OutputTensorShapes);
		}
		
		if (RightChild)
		{
			RightChild->RunModel(ModelInstance, InputTensorShapes, OutputTensorShapes);
		}

		return;
	}

	TArray<float> InputData;
	TArray<float> OutputData;
	TArray<UE::NNE::FTensorBindingCPU> InputBindings;
	TArray<UE::NNE::FTensorBindingCPU> OutputBindings;

	ConvertAcousticData(InputData);

	ensure(InputData.Num() == InputTensorShapes[0].Volume());

	InputBindings.SetNumZeroed(1);
	InputBindings[0].Data = InputData.GetData();
	InputBindings[0].SizeInBytes = InputData.Num() * sizeof(float);

	OutputData.SetNumZeroed(OutputTensorShapes[0].Volume());
	OutputBindings.SetNumZeroed(1);
	OutputBindings[0].Data = OutputData.GetData();
	OutputBindings[0].SizeInBytes = OutputData.Num() * sizeof(float);

	UE::NNE::EResultStatus Result = ModelInstance->RunSync(InputBindings, OutputBindings);
	if (Result == UE::NNE::EResultStatus::Fail)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to run the model"));
		return;
	}

	SaveModelOutputData(OutputData);
}

void FPR_BSPNode::ConvertAcousticData(TArray<float>& OutData) const
{
	OutData.SetNum(3 + AcousticData->Distances.Num() + AcousticData->Materials.Num());

	float Length = AcousticData->Distances[Front] + AcousticData->Distances[Back];
	float Width = AcousticData->Distances[Left] + AcousticData->Distances[Right];
	float Height = AcousticData->Distances[Up] + AcousticData->Distances[Down];
	OutData[0] = Length;
	OutData[1] = Width;
	OutData[2] = Height;

	// Add distances to InputData
	for (int i = 0; i < AcousticData->Distances.Num(); ++i)
	{
		OutData[i + 3] = AcousticData->Distances[i];
	}

	// Add encoded materials to InputData (after distances)
	for (int i = 0; i < AcousticData->Materials.Num(); ++i)
	{
		OutData[i + 3 + AcousticData->Distances.Num()] = static_cast<float>(AcousticData->Materials[i]);
	}
}

void FPR_BSPNode::SaveModelOutputData(const TArray<float>& OutputData) const
{
	AcousticData->ReverbSettings.DecayTime = FMath::Clamp(OutputData[0], 0.0f, 5.0f);
	AcousticData->ReverbSettings.Gain = FMath::Clamp(OutputData[1], 0.0f, 1.0f);
	AcousticData->ReverbSettings.Density = FMath::Clamp(OutputData[2], 0.0f, 1.0f);
	AcousticData->ReverbSettings.WetLevel = FMath::Clamp(OutputData[3], 0.0f, 1.0f);
}

void FPR_BSPNode::DrawDebug(const UWorld* World) const
{
	if (!bIsLeaf)
	{
		LeftChild->DrawDebug(World);
		RightChild->DrawDebug(World);
		return;
	}

	DrawDebugBox(World, BoundingBox.GetCenter(), BoundingBox.GetExtent(), Color, false, -1, 0, 5);
}
