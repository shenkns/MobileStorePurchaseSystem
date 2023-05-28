// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "Blueprint/UserWidget.h"

#include "PurchaseWidget.generated.h"

UCLASS()
class MOBILESTOREPURCHASESYSTEM_API UPurchaseWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void Show();

	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void Hide();
};