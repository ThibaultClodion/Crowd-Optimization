#include "Stats/PerformanceStats.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"

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

void UPerformanceStats::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsRecording || !GEngine)
	{
		return;
	}

	// Frame time in milliseconds
	float FrameTimeMs = DeltaTime * 1000.0f;
	FrameStats.AddSample(FrameTimeMs);

	// Calculate FPS
	float CurrentFPS = (DeltaTime > 0.0f) ? (1.0f / DeltaTime) : 0.0f;
	FPSStats.AddSample(CurrentFPS);

	FStatUnitData* UnitData = GetWorld()->GetGameViewport()->GetStatUnitData();

	// Game thread time
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
		ResetStats();

		// Enable stat unit display (required for GPU/Render/Game times)
		GEngine->Exec(GetWorld(), TEXT("stat unit"));
		GEngine->Exec(GetWorld(), TEXT("r.RHICmdBypass 0"));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Performance Recording STARTED"));
		}
	}
	else
	{
		if (GEngine)
		{
			// Disable stat unit display
			GEngine->Exec(GetWorld(), TEXT("stat unit"));
			GEngine->Exec(GetWorld(), TEXT("r.RHICmdBypass 1"));

			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("========================="));

			FString StatsText = GetFormattedStats();
			TArray<FString> Lines;
			StatsText.ParseIntoArray(Lines, TEXT("\n"));

			for (int32 i = 0; i < Lines.Num(); i++)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, Lines[i]);
			}

			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("========================="));

			ExportToCSV();
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

FString UPerformanceStats::GetDownloadsPath() const
{
	FString DownloadsPath;

#if PLATFORM_WINDOWS
	// Use USERPROFILE environment variable to construct Downloads path
	const int MAX_PATH = 260;
	TCHAR UserProfilePath[MAX_PATH];
	FPlatformMisc::GetEnvironmentVariable(TEXT("USERPROFILE"), UserProfilePath, MAX_PATH);
	DownloadsPath = FString(UserProfilePath) / TEXT("Downloads");
#else
	// For other platforms, use project saved directory
	DownloadsPath = FPaths::ProjectSavedDir();
#endif

	return DownloadsPath;
}

FString UPerformanceStats::GenerateCSVContent() const
{
	FString CSVContent;

	// FPS data (separate section)
	CSVContent += TEXT("Metric,Min (FPS),Average (FPS),Max (FPS),Sample Count\n");
	CSVContent += FString::Printf(TEXT("FPS,%.1f,%.1f,%.1f,%d\n"),
		FPSStats.Min == FLT_MAX ? 0.0f : FPSStats.Min,
		FPSStats.Avg,
		FPSStats.Max,
		FPSStats.SampleCount);

	// Empty line separator
	CSVContent += TEXT("\n");

	// CSV header
	CSVContent += TEXT("Metric,Min (ms),Average (ms),Max (ms),Sample Count\n");

	// Frame data
	CSVContent += FString::Printf(TEXT("Frame,%.2f,%.2f,%.2f,%d\n"),
		FrameStats.Min == FLT_MAX ? 0.0f : FrameStats.Min,
		FrameStats.Avg,
		FrameStats.Max,
		FrameStats.SampleCount);

	// Game Thread data
	CSVContent += FString::Printf(TEXT("Game,%.2f,%.2f,%.2f,%d\n"),
		GameStats.Min == FLT_MAX ? 0.0f : GameStats.Min,
		GameStats.Avg,
		GameStats.Max,
		GameStats.SampleCount);

	// Draw (Render Thread) data
	CSVContent += FString::Printf(TEXT("Draw,%.2f,%.2f,%.2f,%d\n"),
		DrawStats.Min == FLT_MAX ? 0.0f : DrawStats.Min,
		DrawStats.Avg,
		DrawStats.Max,
		DrawStats.SampleCount);

	// GPU data
	CSVContent += FString::Printf(TEXT("GPU,%.2f,%.2f,%.2f,%d\n"),
		GPUStats.Min == FLT_MAX ? 0.0f : GPUStats.Min,
		GPUStats.Avg,
		GPUStats.Max,
		GPUStats.SampleCount);

	return CSVContent;
}

void UPerformanceStats::ExportToCSV()
{
	FString CSVContent = GenerateCSVContent();

	// Generate filename with timestamp
	FDateTime Now = FDateTime::Now();
	FString FileName = FString::Printf(TEXT("PerformanceStats_%s.csv"),
		*Now.ToString(TEXT("%Y%m%d_%H%M%S")));

	FString DownloadsPath = GetDownloadsPath();
	FString FullPath = DownloadsPath / FileName;

	// Save file
	if (FFileHelper::SaveStringToFile(CSVContent, *FullPath))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
				FString::Printf(TEXT("CSV exported successfully: %s"), *FullPath));
		}
		UE_LOG(LogTemp, Log, TEXT("Performance stats exported to: %s"), *FullPath);
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("CSV export failed!"));
		}
		UE_LOG(LogTemp, Error, TEXT("Failed to export performance stats to: %s"), *FullPath);
	}
}