/*
**	Command & Conquer Generals/Zero Hour Map Categorizer
**	Copyright 2025 Vasilije Spasic <vasa.spasic@gmail.com>
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "RefPack.h"

int REF_decode(unsigned char *dest, const unsigned char *s, int *compressedsize, int src_len)
{
    const unsigned char* s_start = s;
    unsigned char *ref, *d = dest;
    unsigned char first, second, third, forth;
    unsigned int run, type;
    int ulen;

    if (!s) return 0;
    
    type = (s[0] << 8) | s[1]; s += 2;
    if (type & 0x8000) {
        if (type & 0x100) s += 4;
        ulen = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3]; s += 4;
    } else {
        if (type & 0x100) s += 3;
        ulen = (s[0] << 16) | (s[1] << 8) | s[2]; s += 3;
    }

    for (;;) {
        if (s >= s_start + src_len) break;
        first = *s++;
        if (!(first & 0x80)) {
            second = *s++; run = first & 3; while (run--) *d++ = *s++;
            ref = d - 1 - (((first & 0x60) << 3) + second);
            run = ((first & 0x1c) >> 2) + 2;
            do { *d++ = *ref++; } while (run--); continue;
        }
        if (!(first & 0x40)) {
            second = *s++; third = *s++; run = second >> 6; while (run--) *d++ = *s++;
            ref = d - 1 - (((second & 0x3f) << 8) + third);
            run = (first & 0x3f) + 3;
            do { *d++ = *ref++; } while (run--); continue;
        }
        if (!(first & 0x20)) {
            second = *s++; third = *s++; forth = *s++; run = first & 3; while (run--) *d++ = *s++;
            ref = d - 1 - ((((first & 0x10) >> 4) << 16) + (second << 8) + third);
            run = (((first & 0x0c) >> 2) << 8) + forth + 4;
            do { *d++ = *ref++; } while (run--); continue;
        }
        run = ((first & 0x1f) << 2) + 4;
        if (run <= 112) { while (run--) *d++ = *s++; continue; }
        run = first & 3; while (run--) *d++ = *s++; break;
    }
    if (compressedsize) *compressedsize = (int)(s - s_start);
    return(ulen);
}