#include "Stats/PerformanceStats.h"
#include "Engine/Engine.h"

UPerformanceStats::UPerformanceStats()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

    bIsRecording = false;
    LastFrameTime = 0.0f;
    LastGameThreadTime = 0.0f;
    LastRenderThreadTime = 0.0f;
    LastGPUFrameTime = 0.0f;
}

void UPerformanceStats::BeginPlay()
{
    Super::BeginPlay();
    ResetStats();
}

void UPerformanceStats::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Only record if recording is active
    if (!bIsRecording || !GEngine)
        return;

    // Get frame time in milliseconds
    float FrameTimeMs = DeltaTime * 1000.0f;
    FrameStats.AddSample(FrameTimeMs);

    // Calculate FPS
    float CurrentFPS = (DeltaTime > 0.0f) ? (1.0f / DeltaTime) : 0.0f;
    FPSStats.AddSample(CurrentFPS);

    FStatUnitData* UnitData = GetWorld()->GetGameViewport()->GetStatUnitData();
    // Game thread time (from FPlatformTime)
    float GameThreadTime = UnitData->GameThreadTime;
    if (GameThreadTime > 0.0f)
    {
        GameStats.AddSample(GameThreadTime);
    }

    // Render thread time
    float RenderThreadTime = UnitData->RenderThreadTime;
    if (RenderThreadTime > 0.0f)
    {
        DrawStats.AddSample(RenderThreadTime);
    }

	// GPU time
    float GPUTime = UnitData->GPUFrameTime[0];
    if (GPUTime > 0.0f)
    {
        GPUStats.AddSample(GPUTime);
    }
}

void UPerformanceStats::ToggleRecording()
{
    bIsRecording = !bIsRecording;

    if (bIsRecording)
    {
        // Starting recording - reset stats
        ResetStats();

		// Enable stat unit display (mandatory for GPU/Render/Game times)
        GEngine->Exec(GetWorld(), TEXT("stat unit"));
        GEngine->Exec(GetWorld(), TEXT("r.RHICmdBypass 0"));

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
                TEXT("Performance Recording STARTED"));
        }
    }
    else
    {
        // Stopping recording - display results
        if (GEngine)
        {
			// Disable stat unit display
            GEngine->Exec(GetWorld(), TEXT("stat unit"));
            GEngine->Exec(GetWorld(), TEXT("r.RHICmdBypass 1"));

            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow,
                TEXT("========================="));

            FString StatsText = GetFormattedStats();
            TArray<FString> Lines;
            StatsText.ParseIntoArray(Lines, TEXT("\n"));

            // Display each line with a small delay offset
            for (int32 i = 0; i < Lines.Num(); i++)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, Lines[i]);
            }

            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow,
                TEXT("========================="));
        }
    }
}

void UPerformanceStats::ResetStats()
{
    FPSStats.Reset();
    FrameStats.Reset();
    GameStats.Reset();
    DrawStats.Reset();
    GPUStats.Reset();
}

FString UPerformanceStats::GetFormattedStats() const
{
    FString Result;

    Result += FString::Printf(TEXT("GPU   - Min: %7.2f | Avg: %7.2f | Max: %7.2f ms\n"),
        GPUStats.Min == FLT_MAX ? 0.0f : GPUStats.Min,
        GPUStats.Avg,
        GPUStats.Max);

    Result += FString::Printf(TEXT("Draw  - Min: %7.2f | Avg: %7.2f | Max: %7.2f ms\n"),
        DrawStats.Min == FLT_MAX ? 0.0f : DrawStats.Min,
        DrawStats.Avg,
        DrawStats.Max);

    Result += FString::Printf(TEXT("Game  - Min: %7.2f | Avg: %7.2f | Max: %7.2f ms\n"),
        GameStats.Min == FLT_MAX ? 0.0f : GameStats.Min,
        GameStats.Avg,
        GameStats.Max);

    Result += FString::Printf(TEXT("Frame - Min: %7.2f | Avg: %7.2f | Max: %7.2f ms\n"),
        FrameStats.Min == FLT_MAX ? 0.0f : FrameStats.Min,
        FrameStats.Avg,
        FrameStats.Max);

    Result += FString::Printf(TEXT("FPS   - Min: %7.1f | Avg: %7.1f | Max: %7.1f"),
        FPSStats.Min == FLT_MAX ? 0.0f : FPSStats.Min,
        FPSStats.Avg,
        FPSStats.Max);

    return Result;
}