// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReadSerialMotSensorLibrary.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT2_API UReadSerialMotSensorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ProcessSerialData"), Category = "Game Config")
		static FString ProcessSerialData(bool &success, float &rx, float &ry, float &rz);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ZeroSerialData"), Category = "Game Config")
		static void ZeroSerialData();
};
