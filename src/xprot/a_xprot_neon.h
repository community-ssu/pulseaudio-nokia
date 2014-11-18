#ifndef A_XPROT_NEON_H
#define A_XPROT_NEON_H

#define mul32_16_s16x4(a, b, c) \
  vreinterpret_s32_s16( \
        vshrn_n_s32(vqaddq_s32(vqdmull_s16(vreinterpret_s16_s32(a), b), c), 16))

#define mul32_16_s32x2(a, b, c) \
          vshrn_n_s32(vqaddq_s32(vqdmull_s16(a, b), c), 16)

#define vext8_s16_2(a, b) \
  vreinterpret_s16_s8(vext_s8(vreinterpret_s8_s16(a),vreinterpret_s8_s32(b),2))

#define vext8_s16_2_s32x2(a, b) \
  vreinterpret_s32_s16(vext8_s16_2(vreinterpret_s16_s32(a), \
                                   vreinterpret_s32_s16(b)))

#define a_xprot_lfsn_vtrn(w1, t, T, rav, k, w2) \
  vtrn_s32(vand_s32(vqadd_s32( \
                  vqdmulh_lane_s32(w1, vset_lane_s32(t, T, 0), 0), rav), k), w2)

#define LOW(a) vget_low_s32(a)

#define HIGH(a) vget_high_s32(a)

#define SET_LOW(a, b) \
  (a) = vcombine_s32(b, HIGH(a))

#define SET_HIGH(a, b) \
  (a) = vcombine_s32(LOW(a), (b))

/* Needed as gcc issues extra stores when usign the intrinsic */
#define _vtrnq_s32(a, b) \
__asm__ __volatile__ ("vtrn.s32 %q0, %q1\n" \
                      : "=&w"(a), "=&w"(b) \
                      : "0" (a), "1" (b) \
                      )

#endif // A_XPROT_NEON_H
