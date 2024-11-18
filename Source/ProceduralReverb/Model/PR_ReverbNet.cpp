// Fill out your copyright notice in the Description page of Project Settings.


#include "PR_ReverbNet.h"

#include "NNE.h"
#include "NNEModelData.h"
#include "NNERuntimeORT/Private/NNERuntimeORT.h"


PR_ReverbNet::PR_ReverbNet()
{
	// Create the model from a neural network model data asset
	TObjectPtr<UNNEModelData> ModelData = LoadObject<UNNEModelData>(GetTransientPackage(), TEXT("C:/Users/anton/OneDrive/Programming/KPI-FICT-M/diploma/reverb_model.onnx"));
	TWeakInterfacePtr<INNERuntimeCPU> Runtime = UE::NNE::GetRuntime<INNERuntimeCPU>(FString("NNERuntimeORTCpu"));
	TSharedPtr<UE::NNE::IModelCPU> Model = Runtime->CreateModelCPU(ModelData);
	TSharedPtr<UE::NNE::IModelInstanceCPU> ModelInstance = Model->CreateModelInstanceCPU();
 
	// Prepare the model given a certain input size
	// ModelInstance->SetInputTensorShapes(InputShapes);
 
	// Run the model passing caller owned CPU memory
	// ModelInstance->RunSync(Inputs, Outputs);
}

PR_ReverbNet::~PR_ReverbNet()
{

}
