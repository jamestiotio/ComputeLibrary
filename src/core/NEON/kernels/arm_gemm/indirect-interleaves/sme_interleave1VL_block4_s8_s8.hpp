/*
 * Copyright (c) 2022 Arm Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#if defined(__ARM_FEATURE_SVE)

template <>
void interleave_block<1, 4, VLType::SME, false>(
  int8_t * &out, const int8_t * const *in,
  size_t width, size_t height, size_t row_offset, bool first
)
{
  ARM_COMPUTE_UNUSED(first);

  __asm__ __volatile__(
      ".inst 0xd503477f  // SMSTART ZA\n"
      "cntb x20\n"
      "mov x22, %x[width]\n"
      "incb x22\n"
      "mov x19, %x[width]\n"
      "sub x9, x20, #0x1\n"
      "cntw x28\n"
      "sub x22, x22, #0x1\n"
      "ands x9, x19, x9\n"
      "udiv x22, x22, x20\n"  // n_passes = ceildiv(width, VL<T>)
      "csel x9, x9, x20, NE\n"
      "lsl x21, %x[height], #0x1\n"  // height * 2
      "lsl x20, x28, #0x1\n"
      "sub x19, x22, #0x1\n"
      "add x9, x9, #0x3\n"
      "sub x27, x28, #0x2\n"
      "whilelt p9.b, XZR, x21\n"
      "whilelt p8.b, x20, x21\n"
      "mov x26, #0x0\n"
      "mov x25, %x[in]\n"
      "lsr x19, x19, #0x1\n"  // n_loops = (n_passes - 1) / 2
      "ldr x24, [x25, #0x0]\n"
      "and x23, x22, #0x1\n"  // odd_tail = bool(n_passes & 0x1)
      "lsr x9, x9, #0x2\n"
      "ldr x22, [x25, #0x8]\n"
      "ptrue p11.s\n"
      "zip1 p10.b, p9.b, p8.b\n"
      "mov x21, %x[row_offset]\n"
      "mov x20, %x[out]\n"
      "whilelt p9.b, x26, %x[width]\n"
      "whilelt p8.b, x26, %x[width]\n"
      "add x25, x25, #0x10\n"
      "mov x12, #0x0\n"
      "cbz x27, 2f\n"
      "1:"  // K loop: Charge: Loop
      ".inst 0x25246140  // psel p0.b, p8.b/Z, p10.b[w12]\n"
      ".inst 0xe0150300  // ld1b { za0h.b[x12] }, p0/Z, [x24, x21]\n"
      ".inst 0x25646140  // psel p0.b, p8.b/Z, p10.b[w12, #4]\n"
      "ldr x24, [x25, #0x0]\n"
      ".inst 0xe01502c4  // ld1b { za0h.b[x12, #4] }, p0/Z, [x22, x21]\n"
      "add x12, x12, #0x8\n"
      "cmp x12, x27, LSL #2\n"
      "ldr x22, [x25, #0x8]\n"
      "add x25, x25, #0x10\n"
      "blt 1b\n"
      "2:"  // K loop: Charge: End
      ".inst 0x25246140  // psel p0.b, p8.b/Z, p10.b[w12]\n"
      ".inst 0xe0150300  // ld1b { za0h.b[x12] }, p0/Z, [x24, x21]\n"
      ".inst 0x25646140  // psel p0.b, p8.b/Z, p10.b[w12, #4]\n"
      "mov x25, %x[in]\n"
      ".inst 0xe01502c4  // ld1b { za0h.b[x12, #4] }, p0/Z, [x22, x21]\n"
      "ldr x24, [x25, #0x0]\n"
      "incb x21\n"
      "ldr x22, [x25, #0x8]\n"
      "add x25, x25, #0x10\n"
      "incb x26\n"
      "cbz x19, 8f\n"
      "mov x19, x19\n"
      "3:"  // K loop: Main loop
      "whilelt p8.b, x26, %x[width]\n"
      "mov x13, #0x0\n"
      "mov x12, #0x0\n"
      "cbz x27, 5f\n"
      "4:"  // K loop: Main loop: First: Loop
      ".inst 0x25356140  // psel p0.b, p8.b/Z, p10.b[w13, #2]\n"
      ".inst 0xe0152302  // ld1b { za0h.b[x13, #2] }, p0/Z, [x24, x21]\n"
      ".inst 0x25756141  // psel p1.b, p8.b/Z, p10.b[w13, #6]\n"
      "ldr x24, [x25, #0x0]\n"
      ".inst 0x25306d20  // psel p0.s, p11.s/Z, p9.s[w12]\n"
      ".inst 0xe01526c6  // ld1b { za0h.b[x13, #6] }, p1/Z, [x22, x21]\n"
      "ldr x22, [x25, #0x8]\n"
      ".inst 0xe0bf8280  // st1w { za0v.s[x12] }, p0/Z, [x20, XZR, LSL #2]\n"
      ".inst 0x25706d20  // psel p0.s, p11.s/Z, p9.s[w12, #1]\n"
      ".inst 0xe0bc8281  // st1w { za0v.s[x12, #1] }, p0/Z, [x20, x28, LSL #2]\n"
      "add x12, x12, #0x2\n"
      "cmp x12, x27\n"
      "add x25, x25, #0x10\n"
      "addvl x20, x20, #2\n"
      "add x13, x13, #0x8\n"
      "blt 4b\n"
      "5:"  // K loop: Main loop: First: Tail
      ".inst 0x25356140  // psel p0.b, p8.b/Z, p10.b[w13, #2]\n"
      ".inst 0xe0152302  // ld1b { za0h.b[x13, #2] }, p0/Z, [x24, x21]\n"
      "mov x25, %x[in]\n"
      "ldr x24, [x25, #0x0]\n"
      ".inst 0x25756141  // psel p1.b, p8.b/Z, p10.b[w13, #6]\n"
      ".inst 0x25306d20  // psel p0.s, p11.s/Z, p9.s[w12]\n"
      ".inst 0xe01526c6  // ld1b { za0h.b[x13, #6] }, p1/Z, [x22, x21]\n"
      "ldr x22, [x25, #0x8]\n"
      ".inst 0xe0bf8280  // st1w { za0v.s[x12] }, p0/Z, [x20, XZR, LSL #2]\n"
      ".inst 0x25706d20  // psel p0.s, p11.s/Z, p9.s[w12, #1]\n"
      "whilelt p9.b, x26, %x[width]\n"
      "incb x26\n"
      "add x25, x25, #0x10\n"
      ".inst 0xe0bc8281  // st1w { za0v.s[x12, #1] }, p0/Z, [x20, x28, LSL #2]\n"
      "addvl x20, x20, #2\n"
      "incb x21\n"
      "whilelt p8.b, x26, %x[width]\n"
      "mov x13, #0x0\n"
      "mov x12, #0x0\n"
      "cbz x27, 7f\n"
      "6:"  // K loop: Main loop: Second: Loop
      ".inst 0x25256140  // psel p0.b, p8.b/Z, p10.b[w13]\n"
      ".inst 0xe0152300  // ld1b { za0h.b[x13] }, p0/Z, [x24, x21]\n"
      ".inst 0x25656141  // psel p1.b, p8.b/Z, p10.b[w13, #4]\n"
      "ldr x24, [x25, #0x0]\n"
      ".inst 0x25306d20  // psel p0.s, p11.s/Z, p9.s[w12]\n"
      ".inst 0xe01526c4  // ld1b { za0h.b[x13, #4] }, p1/Z, [x22, x21]\n"
      "ldr x22, [x25, #0x8]\n"
      ".inst 0xe0bf8288  // st1w { za2v.s[x12] }, p0/Z, [x20, XZR, LSL #2]\n"
      ".inst 0x25706d20  // psel p0.s, p11.s/Z, p9.s[w12, #1]\n"
      ".inst 0xe0bc8289  // st1w { za2v.s[x12, #1] }, p0/Z, [x20, x28, LSL #2]\n"
      "add x12, x12, #0x2\n"
      "cmp x12, x27\n"
      "add x25, x25, #0x10\n"
      "addvl x20, x20, #2\n"
      "add x13, x13, #0x8\n"
      "blt 6b\n"
      "7:"  // K loop: Main loop: Second: Tail
      ".inst 0x25256140  // psel p0.b, p8.b/Z, p10.b[w13]\n"
      ".inst 0xe0152300  // ld1b { za0h.b[x13] }, p0/Z, [x24, x21]\n"
      "mov x25, %x[in]\n"
      "ldr x24, [x25, #0x0]\n"
      ".inst 0x25656141  // psel p1.b, p8.b/Z, p10.b[w13, #4]\n"
      ".inst 0x25306d20  // psel p0.s, p11.s/Z, p9.s[w12]\n"
      ".inst 0xe01526c4  // ld1b { za0h.b[x13, #4] }, p1/Z, [x22, x21]\n"
      "ldr x22, [x25, #0x8]\n"
      ".inst 0xe0bf8288  // st1w { za2v.s[x12] }, p0/Z, [x20, XZR, LSL #2]\n"
      ".inst 0x25706d20  // psel p0.s, p11.s/Z, p9.s[w12, #1]\n"
      "whilelt p9.b, x26, %x[width]\n"
      "subs x19, x19, #0x1\n"
      "add x25, x25, #0x10\n"
      ".inst 0xe0bc8289  // st1w { za2v.s[x12, #1] }, p0/Z, [x20, x28, LSL #2]\n"
      "addvl x20, x20, #2\n"
      "incb x26\n"
      "incb x21\n"
      "bgt 3b\n"
      "8:"  // K loop: Tails
      "cbnz x23, 11f\n"
      "mov x25, %x[in]\n"
      "whilelt p8.b, x26, %x[width]\n"
      "mov x13, #0x0\n"
      "mov x12, #0x0\n"
      "9:"  // K loop: Tails: Even: First
      ".inst 0x25306d20  // psel p0.s, p11.s/Z, p9.s[w12]\n"
      ".inst 0xe0bf8280  // st1w { za0v.s[x12] }, p0/Z, [x20, XZR, LSL #2]\n"
      "ldr x24, [x25, #0x0]\n"
      "add x12, x12, #0x1\n"
      ".inst 0x25356140  // psel p0.b, p8.b/Z, p10.b[w13, #2]\n"
      "cmp x12, x28\n"
      ".inst 0xe0152302  // ld1b { za0h.b[x13, #2] }, p0/Z, [x24, x21]\n"
      "add x25, x25, #0x8\n"
      "addvl x20, x20, #1\n"
      "add x13, x13, #0x4\n"
      "blt 9b\n"
      "whilelt p9.b, x26, %x[width]\n"
      "whilelt p8.b, x26, %x[width]\n"
      "mov x19, #0x0\n"
      "mov x12, #0x0\n"
      "10:"  // K loop: Tails: Even: Second
      ".inst 0x25306d20  // psel p0.s, p11.s/Z, p9.s[w12]\n"
      ".inst 0xe0bf8288  // st1w { za2v.s[x12] }, p0/Z, [x20, XZR, LSL #2]\n"
      "add x12, x12, #0x1\n"
      "cmp x12, x9\n"
      "addvl x20, x20, #1\n"
      "add x19, x19, #0x4\n"
      "blt 10b\n"
      "whilelt p9.b, x26, %x[width]\n"
      "b 13f\n"
      "11:"  // K loop: Tails: Odd
      "mov x12, #0x0\n"
      "12:"  // K loop: Tails: Odd: Loop
      ".inst 0x25306d20  // psel p0.s, p11.s/Z, p9.s[w12]\n"
      ".inst 0xe0bf8280  // st1w { za0v.s[x12] }, p0/Z, [x20, XZR, LSL #2]\n"
      "add x12, x12, #0x1\n"
      "cmp x12, x9\n"
      "addvl x20, x20, #1\n"
      "blt 12b\n"
      "13:"  // K loop: End
      "mov %x[out], x20\n"
      ".inst 0xd503467f  // SMSTOP\n"
      : [out] "+&r" (out)
      : [height] "r" (height), [in] "r" (in), [row_offset] "r" (row_offset), [width] "r" (width)
      : "cc", "memory", "p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8", "p9", "p10", "p11", "p12", "p13", "p14", "p15", "x9", "x12", "x13", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z8", "z9", "z10", "z11", "z12", "z13", "z14", "z15", "z16", "z17", "z18", "z19", "z20", "z21", "z22", "z23", "z24", "z25", "z26", "z27", "z28", "z29", "z30", "z31"
    );
}

#endif  // defined(__ARM_FEATURE_SVE)
