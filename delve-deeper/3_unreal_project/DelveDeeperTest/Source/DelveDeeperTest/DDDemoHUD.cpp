#include "DDDemoHUD.h"

#include "DDDemoCharacter.h"
#include "DDDemoDirector.h"
#include "DDDemoInteractable.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"

namespace
{
TArray<FString> Wrap(const FString& Text, int32 Width)
{
    TArray<FString> Out;
    TArray<FString> Words;
    Text.ParseIntoArray(Words, TEXT(" "));
    FString Line;
    for (const FString& W : Words)
    {
        if (!Line.IsEmpty() && Line.Len() + 1 + W.Len() > Width)
        {
            Out.Add(Line);
            Line.Reset();
        }
        Line += Line.IsEmpty() ? W : TEXT(" ") + W;
    }
    if (!Line.IsEmpty())
    {
        Out.Add(Line);
    }
    return Out;
}
}  // namespace

void ADDDemoHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas)
    {
        return;
    }
    UFont* Font = GEngine->GetMediumFont();
    const float Scale = FMath::Max(1.f, Canvas->ClipY / 900.f);
    const float LineH = 22.f * Scale;

    ADDDemoDirector* Director = nullptr;
    for (TActorIterator<ADDDemoDirector> It(GetWorld()); It; ++It)
    {
        Director = *It;
        break;
    }

    // Status, top left.
    float Y = 16.f;
    TArray<FString> Status;
    if (Director)
    {
        Status = Director->GetStatusLines();
    }
    Status.Add(TEXT("WASD move   Mouse look   Space jump   E interact   Q alternate"));
    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), 8.f, 8.f, 760.f * Scale, (Status.Num() * LineH) + 16.f);
    for (const FString& S : Status)
    {
        DrawText(S, FLinearColor(0.85f, 0.9f, 1.f), 16.f, Y, Font, Scale);
        Y += LineH;
    }

    // Crosshair and prompt, centre.
    const float CX = Canvas->ClipX * 0.5f;
    const float CY = Canvas->ClipY * 0.5f;
    DrawRect(FLinearColor::White, CX - 2.f, CY - 2.f, 4.f, 4.f);
    if (const ADDDemoCharacter* Player = Cast<ADDDemoCharacter>(GetOwningPawn()))
    {
        if (const ADDDemoInteractable* Focus = Player->GetFocus())
        {
            const FString Prompt = Focus->GetPrompt();
            float W = 0.f, H = 0.f;
            GetTextSize(Prompt, W, H, Font, Scale * 1.2f);
            DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), CX - W * 0.5f - 12.f, CY + 60.f, W + 24.f, H + 12.f);
            DrawText(Prompt, FLinearColor(1.f, 0.85f, 0.3f), CX - W * 0.5f, CY + 66.f, Font, Scale * 1.2f);
        }
    }

    // Win banner, stays up once the demo is over (DD-DESC-01).
    if (Director && Director->IsDemoOver())
    {
        const FString Banner = TEXT("YOU WIN - You went down the descent. The demo is over.");
        float W = 0.f, H = 0.f;
        GetTextSize(Banner, W, H, Font, Scale * 2.f);
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.75f), CX - W * 0.5f - 20.f, CY - 140.f, W + 40.f, H + 24.f);
        DrawText(Banner, FLinearColor(1.f, 0.85f, 0.3f), CX - W * 0.5f, CY - 128.f, Font, Scale * 2.f);
    }

    // Recent lines, bottom left (newest at the bottom).
    if (Director)
    {
        TArray<TPair<FString, FLinearColor>> Rows;
        for (const FDDDemoMessage& M : Director->GetMessages())
        {
            const FString Full = M.Speaker.IsEmpty() ? M.Text : M.Speaker + TEXT(": ") + M.Text;
            const FLinearColor Color = M.Speaker == TEXT("Figure") ? FLinearColor(0.55f, 1.f, 0.55f)
                                     : M.Speaker == TEXT("Log")    ? FLinearColor(0.75f, 0.75f, 0.75f)
                                     : M.Speaker == TEXT("Error")  ? FLinearColor(1.f, 0.3f, 0.3f)
                                                                   : FLinearColor::White;
            for (const FString& L : Wrap(Full, 110))
            {
                Rows.Add({L, Color});
            }
        }
        const int32 MaxRows = 12;
        const int32 First = FMath::Max(0, Rows.Num() - MaxRows);
        const int32 Count = Rows.Num() - First;
        float BY = Canvas->ClipY - 16.f - Count * LineH;
        if (Count > 0)
        {
            DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), 8.f, BY - 8.f, 1000.f * Scale, Count * LineH + 16.f);
        }
        for (int32 i = First; i < Rows.Num(); ++i)
        {
            DrawText(Rows[i].Key, Rows[i].Value, 16.f, BY, Font, Scale);
            BY += LineH;
        }
    }
}
