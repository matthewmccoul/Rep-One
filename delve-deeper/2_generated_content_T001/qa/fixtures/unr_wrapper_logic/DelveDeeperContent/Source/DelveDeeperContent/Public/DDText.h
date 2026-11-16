// Hand-written helper. FString <-> std::string conversion and data-file loading. No game rules.
// PROPOSAL, NOT RUN - Unreal missing.
#pragma once

#include "CoreMinimal.h"

#include <string>
#include <vector>

namespace DDText
{
inline std::string ToStd(const FString& S)
{
    FTCHARToUTF8 Conv(*S, S.Len());
    return std::string(Conv.Get(), static_cast<size_t>(Conv.Length()));
}

inline FString FromStd(const std::string& S)
{
    FUTF8ToTCHAR Conv(S.c_str(), static_cast<int32>(S.size()));
    return FString(Conv.Length(), Conv.Get());
}

inline TArray<FString> FromStdList(const std::vector<std::string>& V)
{
    TArray<FString> Out;
    Out.Reserve(static_cast<int32>(V.size()));
    for (const std::string& S : V)
    {
        Out.Add(FromStd(S));
    }
    return Out;
}

// Any core Issue type (code + detail), as "CODE: detail".
template <typename TIssue>
inline FString IssueText(const TIssue& I)
{
    return FromStd(I.code + ": " + I.detail);
}

// Loads <plugin>/Content/Data/<FileName> as UTF-8 text. False with OutError when it cannot.
DELVEDEEPERCONTENT_API bool LoadDataFile(const FString& FileName, FString& OutText, FString& OutError);
}
