#ifndef __XQ2X_YUV_H
#define __XQ2X_YUV_H

/* The default yuv lookup table generated by sysdep_palette.c is
   yuyv in memory order (thus reversed for LSB first). hq2x/lq2x (interp.h)
   needs the most significant byte of the yuv value to be free since it uses it
   to temporarily store significants bits, thus for LSB_FIRST we change lookup
   value from vyuy to vyu by shifting the lookup value left over a byte. For
   non LSB_FIRST we simply don't use the most significant byte. */
#ifdef LSB_FIRST
#define YUV_TO_XQ2X_YUV(p) ((p)>>8)
#define XQ2X_YMASK 0x00FF00
#define XQ2X_UMASK 0x0000FF
#define XQ2X_VMASK 0xFF0000
#define XQ2X_TR_Y  0x003000
#define XQ2X_TR_U  0x000007
#define XQ2X_TR_V  0x060000
#else
#define YUV_TO_XQ2X_YUV(p) (p)
#define XQ2X_YMASK Y2MASK
#define XQ2X_UMASK UMASK
#define XQ2X_VMASK VMASK
#define XQ2X_TR_Y  0x003000
#define XQ2X_TR_U  0x000006
#define XQ2X_TR_V  0x070000
#endif

#endif