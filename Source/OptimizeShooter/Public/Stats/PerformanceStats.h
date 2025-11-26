#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PerformanceStats.generated.h"

USTRUCT(BlueprintType)
struct FPerformanceStat
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float Min = FLT_MAX;

    UPROPERTY(BlueprintReadOnly)
    float Max = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Avg = 0.0f;

    float Total = 0.0f;
    int32 Count = 0;

    void AddSample(float Value)
    {
        Min = FMath::Min(Min, Value);
        Max = FMath::Max(Max, Value);
        Total += Value;
        Count++;
        Avg = Total / Count;
    }

    void Reset()
    {
        Min = FLT_MAX;
        Max = 0.0f;
        Avg = 0.0f;
        Total = 0.0f;
        Count = 0;
    }
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OPTIMIZESHOOTER_API UPerformanceStats : public UActorComponent
{
    GENERATED_BODY()

public:
    UPerformanceStats();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // FPS Stats
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    FPerformanceStat FPSStats;

    // Frame Time Stats (ms)
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    FPerformanceStat FrameStats;

    // Game Thread Time Stats (ms)
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    FPerformanceStat GameStats;

    // Render Thread Time Stats (ms)
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    FPerformanceStat DrawStats;

    // GPU Time Stats (ms)
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    FPerformanceStat GPUStats;

    // Toggle recording (start/stop)
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void ToggleRecording();

    // Check if currently recording
    UFUNCTION(BlueprintPure, Category = "Performance")
    bool IsRecording() const { return bIsRecording; }

    // Reset all statistics
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void ResetStats();

    // Get formatted string for display
    UFUNCTION(BlueprintCallable, Category = "Performance")
    FString GetFormattedStats() const;

protected:
    virtual void BeginPlay() override;

private:
    bool bIsRecording;
    float LastFrameTime;
    float LastGameThreadTime;
    float LastRenderThreadTime;
    float LastGPUFrameTime;
};