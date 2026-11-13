NO_REFERENCE: None of the shown Mordor.Core methods implements a 32-bit signed absolute value. `RgbaImage.Decode` and `X87.RoundToLong` don't take absolute values as their rule. `Character.Cost` calls `Math.Abs(current)` as one step of a cost formula, so it doesn't test absI on its own. The BCL `Math.Abs` is not a Mordor.Core method. No files were written.

REFERENCE: none