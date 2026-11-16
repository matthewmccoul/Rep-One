// Hand-written thin wrapper. All behaviour comes from dd::map (the core).
// PROPOSAL, NOT RUN - Unreal missing.
#include "DDMapLibrary.h"

#include "DDText.h"

#include "dd/map.h"

namespace
{
bool ReadMap(const FString& MapFile, dd::map::FloorMap& Map, FString& Error)
{
    FString Text;
    if (!DDText::LoadDataFile(MapFile, Text, Error))
    {
        return false;
    }
    dd::map::Issue Issue;
    if (!dd::map::fromText(DDText::ToStd(Text), Map, Issue))
    {
        Error = DDText::IssueText(Issue);
        return false;
    }
    return true;
}
}  // namespace

bool UDDMapLibrary::ReadDataFile(const FString& FileName, FString& Text, FString& Error)
{
    return DDText::LoadDataFile(FileName, Text, Error);
}

bool UDDMapLibrary::LoadFloor(const FString& MapFile, TArray<FDDMapNode>& Nodes, TArray<FDDMapEdge>& Edges, FString& Error)
{
    Nodes.Reset();
    Edges.Reset();
    dd::map::FloorMap Map;
    if (!ReadMap(MapFile, Map, Error))
    {
        return false;
    }
    for (const dd::map::Node& N : Map.nodes)
    {
        FDDMapNode Out;
        Out.Id = DDText::FromStd(N.id);
        Out.Type = DDText::FromStd(N.type);
        Out.Name = DDText::FromStd(N.name);
        Out.Area = DDText::FromStd(N.area);
        Out.InsideRoom = DDText::FromStd(N.insideRoom);
        Out.X = N.x;
        Out.Y = N.y;
        Out.bHidden = N.hidden;
        for (const dd::map::Encounter& E : N.encounters)
        {
            Out.EncounterSlots.Add(DDText::FromStd(E.slot));
        }
        for (const dd::map::Trap& T : N.traps)
        {
            Out.TrapSlots.Add(DDText::FromStd(T.slot));
        }
        Out.LootSlots = DDText::FromStdList(N.lootSlots);
        Out.Controls = DDText::FromStdList(N.controls);
        Nodes.Add(Out);
    }
    for (const dd::map::Edge& E : Map.edges)
    {
        FDDMapEdge Out;
        Out.Id = DDText::FromStd(E.id);
        Out.From = DDText::FromStd(E.from);
        Out.To = DDText::FromStd(E.to);
        Out.Door = DDText::FromStd(E.door);
        Edges.Add(Out);
    }
    return true;
}

bool UDDMapLibrary::ValidateFloor(const FString& MapFile, TArray<FString>& Problems)
{
    Problems.Reset();
    dd::map::FloorMap Map;
    FString Error;
    if (!ReadMap(MapFile, Map, Error))
    {
        Problems.Add(Error);
        return false;
    }
    for (const dd::map::Issue& Found : dd::map::validate(Map))
    {
        Problems.Add(DDText::IssueText(Found));
    }
    return Problems.Num() == 0;
}

bool UDDMapLibrary::FindReachable(const FString& MapFile, const FString& Start, const TArray<FString>& BlockedEdges,
                                  const TArray<FString>& BlockedNodes, bool bPlainDoorsOnly, TArray<FString>& Reachable,
                                  FString& Error)
{
    Reachable.Reset();
    dd::map::FloorMap Map;
    if (!ReadMap(MapFile, Map, Error))
    {
        return false;
    }
    dd::map::ReachOptions Options;
    for (const FString& Id : BlockedEdges)
    {
        Options.blockedEdges.insert(DDText::ToStd(Id));
    }
    for (const FString& Id : BlockedNodes)
    {
        Options.blockedNodes.insert(DDText::ToStd(Id));
    }
    Options.plainOnly = bPlainDoorsOnly;
    for (const std::string& Id : Map.reachable(DDText::ToStd(Start), Options))
    {
        Reachable.Add(DDText::FromStd(Id));
    }
    return true;
}
