#include "core.h"

namespace mc {

void MazeSelect::Select(cs::Ref<MazeSelect_View> v)
{
    cs::List<int32_t> V = v->V; cs::List<int32_t> Slot = v->Slot; cs::List<int32_t> Bg = v->Bg;
    int32_t px = v->X, py = v->Y, facing = v->Facing;
    // sub0980 keeps these as Statics. m01C0 is assigned at MODULE3.BAS:784
    // from fn0AE8 - the linear index of the party leader's square.
    int32_t m01B4 = 0, m01B6 = 0, m01B8 = 0, m01BA = 0, m01BC = 0, m01C4 = 0;
    int32_t m01C0 = v->Square();
    // switch on non-constexpr constants -> if/else chain (cases are distinct)
    if (facing == mc00D2)
    {
        m01B6 = 30;
        m01B8 = -1;
    }
    else if (facing == mc00D4)
    {
        m01B6 = 1;
        m01B8 = 30;
    }
    else if (facing == mc00D6)
    {
        m01B6 = -30;
        m01B8 = 1;
    }
    else if (facing == mc00D8)
    {
        m01B6 = -1;
        m01B8 = -30;
    }
    m01BA = -m01B8;

L12048: ;
    if (px == 2)
    {
        if (facing == 1) { V[13] = 1; }
        if (facing == 3) { V[14] = 1; }
        if (facing == 4) { V[7] = 1; V[6] = 1; V[8] = 1; V[21] = 1; V[22] = 1; }
    }
    if (px == 1)
    {
        if (facing == 1) { V[4] = 1; V[11] = 1; }
        if (facing == 3) { V[5] = 1; V[12] = 1; }
        if (facing == 4) { V[1] = 1; V[2] = 1; V[3] = 1; }
    }
    if (py == 2)
    {
        if (facing == 3) { V[6] = 1; V[7] = 1; V[8] = 1; V[21] = 1; V[22] = 1; }
        if (facing == 2) { V[14] = 1; }
        if (facing == 4) { V[13] = 1; }
    }
    if (py == 1)
    {
        if (facing == 3) { V[1] = 1; V[2] = 1; V[3] = 1; }
        if (facing == 2) { V[5] = 1; V[12] = 1; }
        if (facing == 4) { V[4] = 1; V[11] = 1; }
    }
    if (px == 29)
    {
        if (facing == 1) { V[14] = 1; }
        if (facing == 2) { V[6] = 1; V[7] = 1; V[8] = 1; V[21] = 1; V[22] = 1; }
        if (facing == 3) { V[13] = 1; }
    }
    if (px == 30)
    {
        if (facing == 1) { V[5] = 1; V[12] = 1; }
        if (facing == 2) { V[1] = 1; V[2] = 1; V[3] = 1; }
        if (facing == 3) { V[4] = 1; V[11] = 1; }
    }
    if (py == 29)
    {
        if (facing == 1) { V[6] = 1; V[7] = 1; V[8] = 1; V[21] = 1; V[22] = 1; }
        if (facing == 2) { V[13] = 1; }
        if (facing == 4) { V[14] = 1; }
    }
    if (py == 30)
    {
        if (facing == 1) { V[1] = 1; V[2] = 1; V[3] = 1; }
        if (facing == 2) { V[4] = 1; V[11] = 1; }
        if (facing == 4) { V[5] = 1; V[12] = 1; }
    }
    m01BC = m01C0 + m01B8;
    if (m01BC > 900 || m01BC < 1) { goto L12782; }
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 1) { V[4] = m01C4 + 10; }
                if (facing == 2) { V[1] = m01C4 + 10; }
            }
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 1) { V[1] = m01C4 + 10; }
                if (facing == 4) { V[4] = m01C4 + 10; }
            }
        }
    }

L12782: ;
    m01BC = m01C0;
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 1) { V[5] = m01C4 + 10; }
                if (facing == 2) { V[2] = m01C4 + 10; }
                if (facing == 3) { V[4] = m01C4 + 10; }
            }
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 1) { V[2] = m01C4 + 10; }
                if (facing == 2) { V[4] = m01C4 + 10; }
                if (facing == 4) { V[5] = m01C4 + 10; }
            }
        }
    }
L1291E: ;
    m01BC = m01C0 + m01BA;
    if (m01BC > 900 || m01BC < 1) { goto L12A8E; }
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 1) { V[3] = m01C4 + 10; }
                if (facing == 2) { V[5] = m01C4 + 10; }
            }
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 2) { V[3] = m01C4 + 10; }
                if (facing == 3) { V[5] = m01C4 + 10; }
            }
        }
    }
L12A8E: ;
    m01BC = m01C0 + m01B8 + m01B8 + m01B6;
    if (m01BC < 1 || m01BC > 900) { goto L12C12; }
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 1) { V[13] = m01C4 + 10; }
                if (facing == 2) { V[21] = m01C4 + 10; }
            }
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 4) { V[13] = m01C4 + 10; }
                if (facing == 1) { V[21] = m01C4 + 10; }
            }
        }
    }
L12C12: ;
    m01BC = m01C0 + m01B8 + m01B6;
    if (m01BC < 1 || m01BC > 900) { goto L12E30; }
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 1) { V[11] = m01C4 + 10; }
                if (facing == 2) { V[6] = m01C4 + 10; }
                if (facing == 3) { V[13] = m01C4 + 10; }
                if (facing == 4) { V[1] = m01C4 + 10; }
            }
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 1) { V[6] = m01C4 + 10; }
                if (facing == 2) { V[13] = m01C4 + 10; }
                if (facing == 3) { V[1] = m01C4 + 10; }
                if (facing == 4) { V[11] = m01C4 + 10; }
            }
        }
    }
L12E30: ;
    m01BC = m01C0 + m01B6;
    if (m01BC < 1 || m01BC > 900) { goto L13048; }
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 1) { V[12] = m01C4 + 10; }
                if (facing == 2) { V[7] = m01C4 + 10; }
                if (facing == 3) { V[11] = m01C4 + 10; }
                if (facing == 4) { V[2] = m01C4 + 10; }
            }
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 1) { V[7] = m01C4 + 10; }
                if (facing == 2) { V[11] = m01C4 + 10; }
                if (facing == 3) { V[2] = m01C4 + 10; }
                if (facing == 4) { V[12] = m01C4 + 10; }
            }
        }
    }
L13048: ;
    m01BC = m01C0 + m01B6 + m01BA;
    if (m01BC < 1 || m01BC > 900) { goto L13266; }
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 1) { V[14] = m01C4 + 10; }
                if (facing == 2) { V[8] = m01C4 + 10; }
                if (facing == 3) { V[12] = m01C4 + 10; }
                if (facing == 4) { V[3] = m01C4 + 10; }
            }
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 1) { V[8] = m01C4 + 10; }
                if (facing == 2) { V[12] = m01C4 + 10; }
                if (facing == 3) { V[3] = m01C4 + 10; }
                if (facing == 4) { V[14] = m01C4 + 10; }
            }
        }
    }
L13266: ;
L1326C: ;
    m01BC = m01C0 + m01B6 + m01B6;
    if (m01BC < 1 || m01BC > 900) { goto L13392; }
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 4) { V[7] = m01C4 + 10; }
            }
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 3) { V[7] = m01C4 + 10; }
            }
        }
    }
L13392: ;
    m01BC = m01C0 + m01B6 + m01B6 + m01BA;
    if (m01BC < 1 || m01BC > 900) { goto L134C0; }
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 4) { V[8] = m01C4 + 10; }
            }
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 3) { V[8] = m01C4 + 10; }
            }
        }
    }

L134C0: ;
    m01BC = m01C0 + m01B6 + m01B6 + m01B8;
    if (m01BC < 1 || m01BC > 900) { goto L135EC; }
    for (m01C4 = 0; m01C4 <= 5; m01C4++)
    {
        if (Bit(m01C4, WallsAt(v, m01BC)) == 1)
        {
            if (m01C4 == gv1D16 || m01C4 == gc1D1A || m01C4 == gc1D1E)
            {
                if (facing == 4) { V[6] = m01C4 + 10; }
            }
            if (m01C4 == gc1D18 || m01C4 == gc1D1C || m01C4 == gc1D20)
            {
                if (facing == 3) { V[6] = m01C4 + 10; }
            }
        }
    }
L135EC: ;
L135F2: ;
L135F8: ;
    Prune(v);
    if (V[1] > 0)
    {
        if (V[2] == 0 && V[11] == 0 && V[6] == 0 && V[15] == 0 && V[7] == 0)
        {
            Bg[0] = 7;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] > 0 && V[6] == 0 && V[15] == 0 && V[7] == 0)
        {
            Bg[0] = 8;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] > 0 && V[6] == 0 && V[15] == 0 && V[7] > 0)
        {
            Bg[0] = 9;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] == 0 && V[6] == 0 && V[15] > 0 && V[7] == 0)
        {
            Bg[0] = 10;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] == 0 && V[6] == 0 && V[15] > 0 && V[7] > 0)
        {
            Bg[0] = 11;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] == 0 && V[6] == 0 && V[15] == 0 && V[7] > 0)
        {
            Bg[0] = 12;
            goto L13D8C;
        }
        if (V[2] > 0)
        {
            Bg[0] = 16;
            goto L13D8C;
        }
    }
    if (V[2] > 0)
    {
        if (V[4] > 0)
        {
            Bg[0] = 17;
            goto L13D8C;
        }
        if (V[1] > 0)
        {
            Bg[0] = 16;
            goto L13D8C;
        }
        if (V[9] > 0)
        {
            Bg[0] = 19;
            goto L13D8C;
        }
        Bg[0] = 18;
        goto L13D8C;
    }
    if (V[4] > 0)
    {
        if (V[2] == 0 && V[11] == 0 && V[6] == 0 && V[15] == 0 && V[7] == 0)
        {
            Bg[0] = 1;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] > 0 && V[6] == 0 && V[15] == 0 && V[7] == 0)
        {
            Bg[0] = 2;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] > 0 && V[6] == 0 && V[15] == 0 && V[7] != 0)
        {
            Bg[0] = 3;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] == 0 && V[6] == 0 && V[15] > 0 && V[7] == 0)
        {
            Bg[0] = 4;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] == 0 && V[6] == 0 && V[15] > 0 && V[7] != 0)
        {
            Bg[0] = 5;
            goto L13D8C;
        }
        if (V[2] == 0 && V[11] == 0 && V[6] == 0 && V[15] == 0 && V[7] != 0)
        {
            Bg[0] = 6;
            goto L13D8C;
        }
        if (V[2] > 0)
        {
            Bg[0] = 17;
            goto L13D8C;
        }
    }
    if (V[11] > 0)
    {
        if (V[9] == 0 && V[7] == 0)
        {
            Bg[0] = 20;
            goto L13D8C;
        }
        if (V[9] > 0 && V[7] == 0)
        {
            Bg[0] = 21;
            goto L13D8C;
        }
        if (V[9] > 0 && V[7] > 0)
        {
            Bg[0] = 23;
            goto L13D8C;
        }
        if (V[9] == 0 && V[7] > 0)
        {
            Bg[0] = 22;
            goto L13D8C;
        }
    }
    if (V[9] > 0)
    {
        Bg[0] = 24;
        goto L13D8C;
    }
    if (V[6] > 0)
    {
        if (V[7] > 0)
        {
            Bg[0] = 14;
            goto L13D8C;
        }
        if (V[7] == 0)
        {
            Bg[0] = 25;
            goto L13D8C;
        }
    }
    if (V[7] > 0)
    {
        Bg[0] = 13;
        goto L13D8C;
    }

L13D8C: ;
    if (V[3] > 0)
    {
        if (V[2] == 0 && V[12] == 0 && V[8] == 0 && V[16] == 0 && V[7] == 0)
        {
            Bg[1] = 7;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] > 0 && V[8] == 0 && V[16] == 0 && V[7] == 0)
        {
            Bg[1] = 8;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] > 0 && V[8] == 0 && V[16] == 0 && V[7] > 0)
        {
            Bg[1] = 9;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] == 0 && V[8] == 0 && V[16] > 0 && V[7] == 0)
        {
            Bg[1] = 10;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] == 0 && V[8] == 0 && V[16] > 0 && V[7] > 0)
        {
            Bg[1] = 11;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] == 0 && V[8] == 0 && V[16] == 0 && V[7] > 0)
        {
            Bg[1] = 12;
            goto L14A56;
        }
        if (V[2] > 0)
        {
            Bg[1] = 16;
            goto L14A56;
        }
    }
    if (V[2] > 0)
    {
        if (V[5] > 0)
        {
            Bg[1] = 17;
            goto L14A56;
        }
        if (V[3] > 0)
        {
            Bg[1] = 16;
            goto L14A56;
        }
        if (V[10] > 0)
        {
            Bg[1] = 19;
            goto L14A56;
        }
        Bg[1] = 18;
        goto L14A56;
    }
    if (V[5] > 0)
    {
        if (V[2] == 0 && V[12] == 0 && V[8] == 0 && V[16] == 0 && V[7] == 0)
        {
            Bg[1] = 1;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] > 0 && V[8] == 0 && V[16] == 0 && V[7] == 0)
        {
            Bg[1] = 2;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] > 0 && V[8] == 0 && V[16] == 0 && V[7] > 0)
        {
            Bg[1] = 3;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] == 0 && V[8] == 0 && V[16] > 0 && V[7] == 0)
        {
            Bg[1] = 4;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] == 0 && V[8] == 0 && V[16] > 0 && V[7] > 0)
        {
            Bg[1] = 5;
            goto L14A56;
        }
        if (V[2] == 0 && V[12] == 0 && V[8] == 0 && V[16] == 0 && V[7] > 0)
        {
            Bg[1] = 6;
            goto L14A56;
        }
        if (V[2] > 0)
        {
            Bg[1] = 17;
            goto L14A56;
        }
    }
    if (V[12] > 0)
    {
        if (V[10] == 0 && V[7] == 0)
        {
            Bg[1] = 20;
            goto L14A56;
        }
        if (V[10] > 0 && V[7] == 0)
        {
            Bg[1] = 21;
            goto L14A56;
        }
        if (V[10] > 0 && V[7] > 0)
        {
            Bg[1] = 23;
            goto L14A56;
        }
        if (((V[10] > 0) ? -1 : 0) == 0 && V[7] > 0)
        {
            Bg[1] = 22;
            goto L14A56;
        }
    }
    if (V[10] > 0)
    {
        Bg[1] = 24;
        goto L14A56;
    }
    if (V[8] > 0)
    {
        if (V[7] > 0)
        {
            Bg[1] = 14;
            goto L14A56;
        }
        if (V[7] == 0)
        {
            Bg[1] = 25;
            goto L14A56;
        }
    }
    if (V[7] > 0)
    {
        Bg[1] = 13;
        goto L14A56;
    }
    goto L14A56;
L14A56: ;
    for (m01B4 = 1; m01B4 <= 16; m01B4++)
    {
        if (m01B4 != 13 && m01B4 != 14)
        {
            if (V[m01B4] == gc1D1A + 10 || V[m01B4] == gc1D1C + 10)
            {
                switch (m01B4)
                {
                    case 1:
                        Slot[2] = SetBit(mc022C, Slot[2], 1);
                        break;
                    case 3:
                        Slot[2] = SetBit(mc022E, Slot[2], 1);
                        break;
                    case 2:
                        Slot[3] = SetBit(mc022C, Slot[3], 1);
                        break;
                    case 4:
                        Slot[1] = SetBit(mc022C, Slot[1], 1);
                        break;
                    case 5:
                        Slot[1] = SetBit(mc022E, Slot[1], 1);
                        break;
                    case 9:
                        Slot[4] = SetBit(mc022C, Slot[4], 1);
                        break;
                    case 10:
                        Slot[4] = SetBit(mc022E, Slot[4], 1);
                        break;
                    case 15:
                        Slot[5] = SetBit(mc022C, Slot[5], 1);
                        break;
                    case 16:
                        Slot[5] = SetBit(mc022E, Slot[5], 1);
                        break;
                    case 6:
                        Slot[6] = SetBit(mc022C, Slot[6], 1);
                        break;
                    case 8:
                        Slot[6] = SetBit(mc022E, Slot[6], 1);
                        break;
                    case 11:
                        Slot[7] = SetBit(mc022C, Slot[7], 1);
                        break;
                    case 12:
                        Slot[7] = SetBit(mc022E, Slot[7], 1);
                        break;
                    case 7:
                        Slot[8] = SetBit(mc022C, Slot[8], 1);
                        break;
                }
            }
        }
    }
}

}  // namespace mc
