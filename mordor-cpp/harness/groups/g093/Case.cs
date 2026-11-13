using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int level = a.GetProperty("level").GetInt32();
        int x = a.GetProperty("x").GetInt32();
        int y = a.GetProperty("y").GetInt32();

        // ASSUMPTION: H.Dungeon[0] is level 1, so level N maps to index N-1.
        // Not verified: the Floor type outline has no level field. The in-bounds
        // rock cases check this indirectly; a wrong mapping shows as reference 0.
        Floor floor = H.Dungeon[level - 1];

        // IsRock(x, y) ignores the navigator position, so anchor at the origin
        // and keep out-of-bounds query coordinates away from the constructor.
        var nav = new DungeonNav(floor, 0, 0);
        bool rock = nav.IsRock(x, y);
        return (rock ? 1 : 0).ToString();
    }
}
