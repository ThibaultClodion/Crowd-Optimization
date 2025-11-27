// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PerformanceStats.generated.h"

USTRUCT()
struct FStatsData
{
    GENERATED_BODY()

    float Min = FLT_MAX;
    float Max = 0.0f;
    float Avg = 0.0f;
    float Total = 0.0f;
    int32 SampleCount = 0;

    void AddSample(float Value)
    {
        Min = FMath::Min(Min, Value);
        Max = FMath::Max(Max, Value);
        Total += Value;
        SampleCount++;
        Avg = Total / SampleCount;
    }

    void Reset()
    {
        Min = FLT_MAX;
        Max = 0.0f;
        Avg = 0.0f;
        Total = 0.0f;
        SampleCount = 0;
    }
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OPTIMIZESHOOTER_API UPerformanceStats : public UActorComponent
{
    GENERATED_BODY()

public:
    UPerformanceStats();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void ToggleRecording();

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void ResetStats();

    UFUNCTION(BlueprintCallable, Category = "Performance")
    FString GetFormattedStats() const;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void ExportToCSV();

private:
    bool bIsRecording;

    FStatsData FPSStats;
    FStatsData FrameStats;
    FStatsData GameStats;
    FStatsData DrawStats;
    FStatsData GPUStats;

    float LastFrameTime;
    float LastGameThreadTime;
    float LastRenderThreadTime;
    float LastGPUFrameTime;

    FString GetDownloadsPath() const;
    FString GenerateCSVContent() const;
};