/*===========================================================================
	ASTAT -- ALU/MAC status register
===========================================================================*/

/* flag definitions */
#define SSFLAG			0x80
#define MVFLAG			0x40
#define QFLAG			0x20
#define SFLAG			0x10
#define CFLAG			0x08
#define VFLAG			0x04
#define NFLAG			0x02
#define ZFLAG			0x01

/* extracts flags */
#define GET_SS			(adsp2100.astat & SSFLAG)
#define GET_MV			(adsp2100.astat & MVFLAG)
#define GET_Q			(adsp2100.astat &  QFLAG)
#define GET_S			(adsp2100.astat &  SFLAG)
#define GET_C			(adsp2100.astat &  CFLAG)
#define GET_V			(adsp2100.astat &  VFLAG)
#define GET_N			(adsp2100.astat &  NFLAG)
#define GET_Z			(adsp2100.astat &  ZFLAG)

/* clears flags */
#define CLR_SS			(adsp2100.astat &= ~SSFLAG)
#define CLR_MV			(adsp2100.astat &= ~MVFLAG)
#define CLR_Q			(adsp2100.astat &=  ~QFLAG)
#define CLR_S			(adsp2100.astat &=  ~SFLAG)
#define CLR_C			(adsp2100.astat &=  ~CFLAG)
#define CLR_V			(adsp2100.astat &=  ~VFLAG)
#define CLR_N			(adsp2100.astat &=  ~NFLAG)
#define CLR_Z			(adsp2100.astat &=  ~ZFLAG)

/* sets flags */
#define SET_SS			(adsp2100.astat |= SSFLAG)
#define SET_MV			(adsp2100.astat |= MVFLAG)
#define SET_Q			(adsp2100.astat |=  QFLAG)
#define SET_S			(adsp2100.astat |=  SFLAG)
#define SET_C			(adsp2100.astat |=  CFLAG)
#define SET_V			(adsp2100.astat |=  VFLAG)
#define SET_Z			(adsp2100.astat |=  ZFLAG)
#define SET_N			(adsp2100.astat |=  NFLAG)

/* flag clearing; must be done before setting */
#define CLR_FLAGS		(adsp2100.astat &= adsp2100.astat_clear)

/* compute flags */
#define CALC_Z(r)		(adsp2100.astat |= ((r & 0xffff) == 0))
#define CALC_N(r)		(adsp2100.astat |= (r >> 14) & 0x02)
#define CALC_V(s,d,r)	(adsp2100.astat |= ((s ^ d ^ r ^ (r >> 1)) >> 13) & 0x04)
#define CALC_C(r)		(adsp2100.astat |= (r >> 13) & 0x08)
#define CALC_C_SUB(r)	(adsp2100.astat |= (~r >> 13) & 0x08)
#define CALC_NZ(r) 		CLR_FLAGS; CALC_N(r); CALC_Z(r)
#define CALC_NZV(s,d,r) CLR_FLAGS; CALC_N(r); CALC_Z(r); CALC_V(s,d,r)
#define CALC_NZVC(s,d,r) CLR_FLAGS; CALC_N(r); CALC_Z(r); CALC_V(s,d,r); CALC_C(r)
#define CALC_NZVC_SUB(s,d,r) CLR_FLAGS; CALC_N(r); CALC_Z(r); CALC_V(s,d,r); CALC_C_SUB(r)



/*===========================================================================
	MSTAT -- ALU/MAC control register
===========================================================================*/

/* flag definitions */
#define MSTAT_BANK		0x01			/* register bank select */
#define MSTAT_REVERSE	0x02			/* bit-reverse addressing enable (DAG1) */
#define MSTAT_STICKYV	0x04			/* sticky ALU overflow enable */
#define MSTAT_SATURATE	0x08			/* AR saturation mode enable */

#if SUPPORT_2101_EXTENSIONS
#define MSTAT_INTEGER	0x10			/* MAC result placement; 0=fractional, 1=integer */
#define MSTAT_TIMER		0x20			/* timer enable */
#define MSTAT_GOMODE	0x40			/* go mode enable */
#endif

/* call this whenever mstat changes */
INLINE void mstat_changed(void)
{
#if SWAPCORE
	/* did the MSTAT_BANK flag actually change? */
	if ( (adsp2100.mstat_old ^ adsp2100.mstat) & MSTAT_BANK )
	{
		ADSPCORE tmp = adsp2100.r[0];
		adsp2100.r[0] = adsp2100.r[1];
		adsp2100.r[1] = tmp;
		adsp2100.mstat_old = adsp2100.mstat;
	}
#else
    core = &adsp2100.r[adsp2100.mstat & MSTAT_BANK];
#endif
	if (adsp2100.mstat & MSTAT_STICKYV)
		adsp2100.astat_clear = ~(CFLAG | NFLAG | ZFLAG);
	else 
		adsp2100.astat_clear = ~(CFLAG | VFLAG | NFLAG | ZFLAG);
}


/*===========================================================================
	SSTAT -- stack status register
===========================================================================*/

/* flag definitions */
#define PC_EMPTY		0x01			/* PC stack empty */
#define PC_OVER			0x02			/* PC stack overflow */
#define COUNT_EMPTY		0x04			/* count stack empty */
#define COUNT_OVER		0x08			/* count stack overflow */
#define STATUS_EMPTY	0x10			/* status stack empty */
#define STATUS_OVER		0x20			/* status stack overflow */
#define LOOP_EMPTY		0x40			/* loop stack empty */
#define LOOP_OVER		0x80			/* loop stack overflow */



/*===========================================================================
	PC stack handlers
===========================================================================*/

INLINE UINT16 pc_stack_top(void)
{
	if (adsp2100.pc_sp > 0)
		return adsp2100.pc_stack[adsp2100.pc_sp - 1];
	else
		return adsp2100.pc_stack[0];
}

INLINE void set_pc_stack_top(UINT16 top)
{
	if (adsp2100.pc_sp > 0)
		adsp2100.pc_stack[adsp2100.pc_sp - 1] = top;
	else
		adsp2100.pc_stack[0] = top;
}

INLINE void pc_stack_push(void)
{
	if (adsp2100.pc_sp < PC_STACK_DEPTH)
	{
		adsp2100.pc_stack[adsp2100.pc_sp] = adsp2100.pc;
		adsp2100.pc_sp++;
		adsp2100.sstat &= ~PC_EMPTY;
	}
	else
		adsp2100.sstat |= PC_OVER;
}

INLINE void pc_stack_push_val(UINT16 val)
{
	if (adsp2100.pc_sp < PC_STACK_DEPTH)
	{
		adsp2100.pc_stack[adsp2100.pc_sp] = val;
		adsp2100.pc_sp++;
		adsp2100.sstat &= ~PC_EMPTY;
	}
	else
		adsp2100.sstat |= PC_OVER;
}

INLINE void pc_stack_pop(void)
{
	if (adsp2100.pc_sp > 0)
	{
		adsp2100.pc_sp--;
		if (adsp2100.pc_sp == 0)
			adsp2100.sstat |= PC_EMPTY;
	}
	adsp2100.pc = adsp2100.pc_stack[adsp2100.pc_sp];
}

INLINE UINT16 pc_stack_pop_val(void)
{
	if (adsp2100.pc_sp > 0)
	{
		adsp2100.pc_sp--;
		if (adsp2100.pc_sp == 0)
			adsp2100.sstat |= PC_EMPTY;
	}
	return adsp2100.pc_stack[adsp2100.pc_sp];
}


/*===========================================================================
	CNTR stack handlers
===========================================================================*/

INLINE UINT16 cntr_stack_top(void)
{
	if (adsp2100.cntr_sp > 0)
		return adsp2100.cntr_stack[adsp2100.cntr_sp - 1];
	else
		return adsp2100.cntr_stack[0];
}

INLINE void cntr_stack_push(void)
{
	if (adsp2100.cntr_sp < CNTR_STACK_DEPTH)
	{
		adsp2100.cntr_stack[adsp2100.cntr_sp] = adsp2100.cntr;
		adsp2100.cntr_sp++;
		adsp2100.sstat &= ~COUNT_EMPTY;
	}
	else
		adsp2100.sstat |= COUNT_OVER;
}

INLINE void cntr_stack_pop(void)
{
	if (adsp2100.cntr_sp > 0)
	{
		adsp2100.cntr_sp--;
		if (adsp2100.cntr_sp == 0)
			adsp2100.sstat |= COUNT_EMPTY;
	}
	adsp2100.cntr = adsp2100.cntr_stack[adsp2100.cntr_sp];
}


/*===========================================================================
	LOOP stack handlers
===========================================================================*/

INLINE UINT32 loop_stack_top(void)
{
	if (adsp2100.loop_sp > 0)
		return adsp2100.loop_stack[adsp2100.loop_sp - 1];
	else
		return adsp2100.loop_stack[0];
}

INLINE void loop_stack_push(UINT32 value)
{
	if (adsp2100.loop_sp < LOOP_STACK_DEPTH)
	{
		adsp2100.loop_stack[adsp2100.loop_sp] = value;
		adsp2100.loop_sp++;
		adsp2100.loop = value >> 4;
		adsp2100.loop_condition = value & 15;
		adsp2100.sstat &= ~LOOP_EMPTY;
	}
	else
		adsp2100.sstat |= LOOP_OVER;
}

INLINE void loop_stack_pop(void)
{
	if (adsp2100.loop_sp > 0)
	{
		adsp2100.loop_sp--;
		if (adsp2100.loop_sp == 0)
		{
			adsp2100.loop = 0xffff;
			adsp2100.loop_condition = 0;
			adsp2100.sstat |= LOOP_EMPTY;
		}
		else
		{
			adsp2100.loop = adsp2100.loop_stack[adsp2100.loop_sp -1] >> 4;
			adsp2100.loop_condition = adsp2100.loop_stack[adsp2100.loop_sp - 1] & 15;
		}
	}
}


/*===========================================================================
	STAT stack handlers
===========================================================================*/

INLINE void stat_stack_push(void)
{
	if (adsp2100.stat_sp < STAT_STACK_DEPTH)
	{
		adsp2100.stat_stack[adsp2100.stat_sp][0] = adsp2100.mstat;
		adsp2100.stat_stack[adsp2100.stat_sp][1] = adsp2100.imask;
		adsp2100.stat_stack[adsp2100.stat_sp][2] = adsp2100.astat;
		adsp2100.stat_sp++;
		adsp2100.sstat &= ~STATUS_EMPTY;
	}
	else
		adsp2100.sstat |= STATUS_OVER;
}

INLINE void stat_stack_pop(void)
{
	if (adsp2100.stat_sp > 0)
	{
		adsp2100.stat_sp--;
		if (adsp2100.stat_sp == 0)
			adsp2100.sstat |= STATUS_EMPTY;
	}
	adsp2100.mstat = adsp2100.stat_stack[adsp2100.stat_sp][0]; mstat_changed();
	adsp2100.imask = adsp2100.stat_stack[adsp2100.stat_sp][1]; check_irqs();
	adsp2100.astat = adsp2100.stat_stack[adsp2100.stat_sp][2];
}



/*===========================================================================
	condition code checking
===========================================================================*/

INLINE int CONDITION(int c)
{
	if (c != 14)
		return condition_table[((c) << 8) | adsp2100.astat];
	else if (--adsp2100.cntr != 0) 
		return 0;
	else
	{
		cntr_stack_pop();
		return 1;
	}
}



/*===========================================================================
	register writing
===========================================================================*/

static void wr_inval(INT32 val) { logerror( "ADSP %04x: Writing to an invalid register!", adsp2100.ppc ); }
static void wr_ax0(INT32 val)	{ CORE(ax0.s) = val; }
static void wr_ax1(INT32 val)	{ CORE(ax1.s) = val; }
static void wr_mx0(INT32 val)	{ CORE(mx0.s) = val; }
static void wr_mx1(INT32 val)	{ CORE(mx1.s) = val; }
static void wr_ay0(INT32 val)	{ CORE(ay0.s) = val; }
static void wr_ay1(INT32 val)	{ CORE(ay1.s) = val; }
static void wr_my0(INT32 val)	{ CORE(my0.s) = val; }
static void wr_my1(INT32 val)	{ CORE(my1.s) = val; }
static void wr_si(INT32 val)	{ CORE(si.s) = val; }
static void wr_se(INT32 val)	{ CORE(se.s) = (INT8)val; }
static void wr_ar(INT32 val)	{ CORE(ar.s) = val; }
static void wr_mr0(INT32 val)	{ CORE(mr.mrx.mr0.s) = val; }
static void wr_mr1(INT32 val)	{ CORE(mr.mrx.mr1.s) = val; CORE(mr.mrx.mr2.s) = (INT16)val >> 15; }
static void wr_mr2(INT32 val)	{ CORE(mr.mrx.mr2.s) = (INT8)val; }
static void wr_sr0(INT32 val)	{ CORE(sr.srx.sr0.s) = val; }
static void wr_sr1(INT32 val)	{ CORE(sr.srx.sr1.s) = val; }
static void wr_i0(INT32 val)    { adsp2100.i[0] = val & 0x3fff; adsp2100.base[0] = val & adsp2100.lmask[0]; }
static void wr_i1(INT32 val)    { adsp2100.i[1] = val & 0x3fff; adsp2100.base[1] = val & adsp2100.lmask[1]; }
static void wr_i2(INT32 val)    { adsp2100.i[2] = val & 0x3fff; adsp2100.base[2] = val & adsp2100.lmask[2]; }
static void wr_i3(INT32 val)    { adsp2100.i[3] = val & 0x3fff; adsp2100.base[3] = val & adsp2100.lmask[3]; }
static void wr_i4(INT32 val)    { adsp2100.i[4] = val & 0x3fff; adsp2100.base[4] = val & adsp2100.lmask[4]; }
static void wr_i5(INT32 val)    { adsp2100.i[5] = val & 0x3fff; adsp2100.base[5] = val & adsp2100.lmask[5]; }
static void wr_i6(INT32 val)    { adsp2100.i[6] = val & 0x3fff; adsp2100.base[6] = val & adsp2100.lmask[6]; }
static void wr_i7(INT32 val)    { adsp2100.i[7] = val & 0x3fff; adsp2100.base[7] = val & adsp2100.lmask[7]; }
static void wr_m0(INT32 val)    { adsp2100.m[0] = (INT16)(val << 2) >> 2; }
static void wr_m1(INT32 val)    { adsp2100.m[1] = (INT16)(val << 2) >> 2; }
static void wr_m2(INT32 val)    { adsp2100.m[2] = (INT16)(val << 2) >> 2; }
static void wr_m3(INT32 val)    { adsp2100.m[3] = (INT16)(val << 2) >> 2; }
static void wr_m4(INT32 val)    { adsp2100.m[4] = (INT16)(val << 2) >> 2; }
static void wr_m5(INT32 val)    { adsp2100.m[5] = (INT16)(val << 2) >> 2; }
static void wr_m6(INT32 val)    { adsp2100.m[6] = (INT16)(val << 2) >> 2; }
static void wr_m7(INT32 val)    { adsp2100.m[7] = (INT16)(val << 2) >> 2; }
static void wr_l0(INT32 val)    { adsp2100.l[0] = val & 0x3fff; adsp2100.lmask[0] = mask_table[val & 0x3fff]; adsp2100.base[0] = adsp2100.i[0] & adsp2100.lmask[0]; }
static void wr_l1(INT32 val)    { adsp2100.l[1] = val & 0x3fff; adsp2100.lmask[1] = mask_table[val & 0x3fff]; adsp2100.base[1] = adsp2100.i[1] & adsp2100.lmask[1]; }
static void wr_l2(INT32 val)    { adsp2100.l[2] = val & 0x3fff; adsp2100.lmask[2] = mask_table[val & 0x3fff]; adsp2100.base[2] = adsp2100.i[2] & adsp2100.lmask[2]; }
static void wr_l3(INT32 val)    { adsp2100.l[3] = val & 0x3fff; adsp2100.lmask[3] = mask_table[val & 0x3fff]; adsp2100.base[3] = adsp2100.i[3] & adsp2100.lmask[3]; }
static void wr_l4(INT32 val)    { adsp2100.l[4] = val & 0x3fff; adsp2100.lmask[4] = mask_table[val & 0x3fff]; adsp2100.base[4] = adsp2100.i[4] & adsp2100.lmask[4]; }
static void wr_l5(INT32 val)    { adsp2100.l[5] = val & 0x3fff; adsp2100.lmask[5] = mask_table[val & 0x3fff]; adsp2100.base[5] = adsp2100.i[5] & adsp2100.lmask[5]; }
static void wr_l6(INT32 val)    { adsp2100.l[6] = val & 0x3fff; adsp2100.lmask[6] = mask_table[val & 0x3fff]; adsp2100.base[6] = adsp2100.i[6] & adsp2100.lmask[6]; }
static void wr_l7(INT32 val)    { adsp2100.l[7] = val & 0x3fff; adsp2100.lmask[7] = mask_table[val & 0x3fff]; adsp2100.base[7] = adsp2100.i[7] & adsp2100.lmask[7]; }
static void wr_astat(INT32 val) { adsp2100.astat = val & 0x00ff; }
#if SUPPORT_2101_EXTENSIONS
static void wr_mstat(INT32 val) { adsp2100.mstat = val & 0x007f; mstat_changed(); }
#else
static void wr_mstat(INT32 val) { adsp2100.mstat = val & 0x000f; mstat_changed(); }
#endif
static void wr_sstat(INT32 val) { adsp2100.sstat = val & 0x00ff; }
#if SUPPORT_2101_EXTENSIONS
static void wr_imask(INT32 val) { adsp2100.imask = val & 0x003f; check_irqs(); }
#else
static void wr_imask(INT32 val) { adsp2100.imask = val & 0x000f; check_irqs(); }
#endif
static void wr_icntl(INT32 val) { adsp2100.icntl = val & 0x001f; check_irqs(); }
static void wr_cntr(INT32 val)  { cntr_stack_push(); adsp2100.cntr = val & 0x3fff; }
static void wr_sb(INT32 val)	{ CORE(sb.s) = (INT16)(val << 11) >> 11; }
static void wr_px(INT32 val)    { adsp2100.px = val; }
#if SUPPORT_2101_EXTENSIONS
static void wr_ifc(INT32 val)	{ adsp2100.ifc = val; }
static void wr_tx0(INT32 val)	{ if ( adsp2105_tx_callback ) (*adsp2105_tx_callback)( 0, val ); }
static void wr_tx1(INT32 val)	{ if ( adsp2105_tx_callback ) (*adsp2105_tx_callback)( 1, val ); }
static void wr_owrctr(INT32 val) { adsp2100.cntr = val & 0x3fff; }
#endif
static void wr_topstack(INT32 val) { pc_stack_push_val(val & 0x3fff); }

#define WRITE_REG(grp,reg,val) ((*wr_reg[grp][reg])(val))

static void (*wr_reg[4][16])(INT32) =
{
	{
		wr_ax0, wr_ax1, wr_mx0, wr_mx1, wr_ay0, wr_ay1, wr_my0, wr_my1, 
		wr_si, wr_se, wr_ar, wr_mr0, wr_mr1, wr_mr2, wr_sr0, wr_sr1
	},
	{
		wr_i0, wr_i1, wr_i2, wr_i3, wr_m0, wr_m1, wr_m2, wr_m3,
		wr_l0, wr_l1, wr_l2, wr_l3, wr_inval, wr_inval, wr_inval, wr_inval
	},
	{
		wr_i4, wr_i5, wr_i6, wr_i7, wr_m4, wr_m5, wr_m6, wr_m7,
		wr_l4, wr_l5, wr_l6, wr_l7, wr_inval, wr_inval, wr_inval, wr_inval
	},
	{
		wr_astat, wr_mstat, wr_inval, wr_imask, wr_icntl, wr_cntr, wr_sb, wr_px,
#if SUPPORT_2101_EXTENSIONS
		wr_inval, wr_tx0, wr_inval, wr_tx1, wr_ifc, wr_owrctr, wr_inval, wr_topstack
#else
		wr_inval, wr_inval, wr_inval, wr_inval, wr_inval, wr_inval, wr_inval, wr_topstack
#endif
	}
};
		


/*===========================================================================
	register reading
===========================================================================*/

static INT32 rd_inval(void) { logerror( "ADSP %04x: Reading from an invalid register!", adsp2100.ppc ); return 0; }
static INT32 rd_ax0(void)	{ return CORE(ax0.s); }
static INT32 rd_ax1(void)	{ return CORE(ax1.s); }
static INT32 rd_mx0(void)	{ return CORE(mx0.s); }
static INT32 rd_mx1(void)	{ return CORE(mx1.s); }
static INT32 rd_ay0(void)	{ return CORE(ay0.s); }
static INT32 rd_ay1(void)	{ return CORE(ay1.s); }
static INT32 rd_my0(void)	{ return CORE(my0.s); }
static INT32 rd_my1(void)	{ return CORE(my1.s); }
static INT32 rd_si(void)	{ return CORE(si.s); }
static INT32 rd_se(void)	{ return CORE(se.s); }
static INT32 rd_ar(void)	{ return CORE(ar.s); }
static INT32 rd_mr0(void)	{ return CORE(mr.mrx.mr0.s); }
static INT32 rd_mr1(void)	{ return CORE(mr.mrx.mr1.s); }
static INT32 rd_mr2(void)	{ return CORE(mr.mrx.mr2.s); }
static INT32 rd_sr0(void)	{ return CORE(sr.srx.sr0.s); }
static INT32 rd_sr1(void)	{ return CORE(sr.srx.sr1.s); }
static INT32 rd_i0(void)    { return adsp2100.i[0]; }
static INT32 rd_i1(void)    { return adsp2100.i[1]; }
static INT32 rd_i2(void)    { return adsp2100.i[2]; }
static INT32 rd_i3(void)    { return adsp2100.i[3]; }
static INT32 rd_i4(void)    { return adsp2100.i[4]; }
static INT32 rd_i5(void)    { return adsp2100.i[5]; }
static INT32 rd_i6(void)    { return adsp2100.i[6]; }
static INT32 rd_i7(void)    { return adsp2100.i[7]; }
static INT32 rd_m0(void)    { return adsp2100.m[0]; }
static INT32 rd_m1(void)    { return adsp2100.m[1]; }
static INT32 rd_m2(void)    { return adsp2100.m[2]; }
static INT32 rd_m3(void)    { return adsp2100.m[3]; }
static INT32 rd_m4(void)    { return adsp2100.m[4]; }
static INT32 rd_m5(void)    { return adsp2100.m[5]; }
static INT32 rd_m6(void)    { return adsp2100.m[6]; }
static INT32 rd_m7(void)    { return adsp2100.m[7]; }
static INT32 rd_l0(void)    { return adsp2100.l[0]; }
static INT32 rd_l1(void)    { return adsp2100.l[1]; }
static INT32 rd_l2(void)    { return adsp2100.l[2]; }
static INT32 rd_l3(void)    { return adsp2100.l[3]; }
static INT32 rd_l4(void)    { return adsp2100.l[4]; }
static INT32 rd_l5(void)    { return adsp2100.l[5]; }
static INT32 rd_l6(void)    { return adsp2100.l[6]; }
static INT32 rd_l7(void)    { return adsp2100.l[7]; }
static INT32 rd_astat(void) { return adsp2100.astat; }
static INT32 rd_mstat(void) { return adsp2100.mstat; }
static INT32 rd_sstat(void) { return adsp2100.sstat; }
static INT32 rd_imask(void) { return adsp2100.imask; }
static INT32 rd_icntl(void) { return adsp2100.icntl; }
static INT32 rd_cntr(void)  { return adsp2100.cntr; }
static INT32 rd_sb(void)	{ return CORE(sb.s); }
static INT32 rd_px(void)    { return adsp2100.px; }
#if SUPPORT_2101_EXTENSIONS
static INT32 rd_rx0(void)	{ if ( adsp2105_rx_callback ) return (*adsp2105_rx_callback)( 0 ); else return 0; }
static INT32 rd_rx1(void)	{ if ( adsp2105_rx_callback ) return (*adsp2105_rx_callback)( 1 ); else return 0; }
#endif
static INT32 rd_stacktop(void)	{ return pc_stack_pop_val(); }

#if REGPOINTERS

#define ICORE(_reg)    (INT16*)&adsp2100._reg
static INT16 *pt_reg[4][16] =
{
	{
		&CORE(ax0.s),		&CORE(ax1.s),		&CORE(mx0.s),		&CORE(mx1.s),
		&CORE(ay0.s),		&CORE(ay1.s),		&CORE(my0.s),		&CORE(my1.s),
		&CORE(si.s),		&CORE(se.s),		&CORE(ar.s),		&CORE(mr.mrx.mr0.s),
		&CORE(mr.mrx.mr1.s),&CORE(mr.mrx.mr2.s),&CORE(sr.srx.sr0.s),&CORE(sr.srx.sr1.s)
	},{
		ICORE(i[0]),		ICORE(i[1]),		ICORE(i[2]),		ICORE(i[3]),
		&adsp2100.m[0], 	&adsp2100.m[1], 	&adsp2100.m[2], 	&adsp2100.m[3],
		ICORE(l[0]),		ICORE(l[1]),		ICORE(l[2]),		ICORE(l[3]),
		&CORE(zero.s),		&CORE(zero.s),		&CORE(zero.s),		&CORE(zero.s)
	},{
		ICORE(i[4]),		ICORE(i[5]),		ICORE(i[6]),		ICORE(i[7]),
		&adsp2100.m[4], 	&adsp2100.m[5], 	&adsp2100.m[6], 	&adsp2100.m[7],
		ICORE(l[4]),		ICORE(l[5]),		ICORE(l[6]),		ICORE(l[7]),
		&CORE(zero.s),		&CORE(zero.s),		&CORE(zero.s),		&CORE(zero.s)
	},{
		ICORE(astat),		ICORE(mstat),		ICORE(sstat),		ICORE(imask),
		ICORE(icntl),		ICORE(cntr),		&CORE(sb.s),		ICORE(px),
		&CORE(zero.s),		&CORE(zero.s),		&CORE(zero.s),		&CORE(zero.s),
		&CORE(zero.s),		&CORE(zero.s),		&CORE(zero.s),		(INT16 *)0xdeadbeef
	}
};

/* for a test you could replace SUPPORT_2101_EXTENSIONS with 0,
   as long as rd_rx0 and rd_rx1 aren't really used */
#if SUPPORT_2101_EXTENSIONS 

#define READ_REG(grp,reg) (rd_reg[grp][reg] ? (*rd_reg[grp][reg])() : *pt_reg[grp][reg])

static INT32 (*rd_reg[4][16])(void) =
{
	{
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL
	},
	{
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL
	},
	{
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL
	},
	{
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		rd_rx0, NULL,	rd_rx1, NULL,	NULL,	NULL,	NULL,	rd_stacktop
	}
};

#else	/* !SUPPORT_2101_EXTENSIONS */

#define READ_REG(grp,reg) ( (((grp)<<4)|(reg)) < 0x3f ? *pt_reg[grp][reg] : pc_stack_pop_val() )

#endif

#else	/* !REGPOINTERS */

#define READ_REG(grp,reg) ((*rd_reg[grp][reg])())

static INT32 (*rd_reg[4][16])(void) =
{
	{
		rd_ax0, rd_ax1, rd_mx0, rd_mx1, rd_ay0, rd_ay1, rd_my0, rd_my1, 
		rd_si, rd_se, rd_ar, rd_mr0, rd_mr1, rd_mr2, rd_sr0, rd_sr1
	},
	{
		rd_i0, rd_i1, rd_i2, rd_i3, rd_m0, rd_m1, rd_m2, rd_m3,
		rd_l0, rd_l1, rd_l2, rd_l3, rd_inval, rd_inval, rd_inval, rd_inval
	},
	{
		rd_i4, rd_i5, rd_i6, rd_i7, rd_m4, rd_m5, rd_m6, rd_m7,
		rd_l4, rd_l5, rd_l6, rd_l7, rd_inval, rd_inval, rd_inval, rd_inval
	},
	{
		rd_astat, rd_mstat, rd_sstat, rd_imask, rd_icntl, rd_cntr, rd_sb, rd_px,
#if SUPPORT_2101_EXTENSIONS
		rd_rx0, rd_inval, rd_rx1, rd_inval, rd_inval, rd_inval, rd_inval, rd_stacktop
#else
		rd_inval, rd_inval, rd_inval, rd_inval, rd_inval, rd_inval, rd_inval, rd_stacktop
#endif
	}
};
#endif


/*===========================================================================
	Modulus addressing logic
===========================================================================*/

INLINE void modify_address(UINT32 ireg, UINT32 mreg)
{
	UINT32 base = adsp2100.base[ireg];
	UINT32 i = adsp2100.i[ireg];
	UINT32 l = adsp2100.l[ireg];
	
	i += adsp2100.m[mreg];
	if (i < base) i += l;
	else if (i >= base + l) i -= l;
	adsp2100.i[ireg] = i;
}



/*===========================================================================
	Data memory accessors
===========================================================================*/

INLINE void data_write_dag1(UINT32 op, INT32 val)
{
	UINT32 ireg = (op >> 2) & 3;
	UINT32 mreg = op & 3;
	UINT32 base = adsp2100.base[ireg];
	UINT32 i = adsp2100.i[ireg];
	UINT32 l = adsp2100.l[ireg];
	
	if ( adsp2100.mstat & MSTAT_REVERSE )
	{
		UINT32 ir = reverse_table[ i & 0x3fff ];
		WWORD_DATA(ir, val);
	}
	else
		WWORD_DATA(i, val);
	
	i += adsp2100.m[mreg];
	if (i < base) i += l;
	else if (i >= base + l) i -= l;
	adsp2100.i[ireg] = i;
}
		

INLINE UINT16 data_read_dag1(UINT32 op)
{
	UINT32 ireg = (op >> 2) & 3;
	UINT32 mreg = op & 3;
	UINT32 base = adsp2100.base[ireg];
	UINT32 i = adsp2100.i[ireg];
	UINT32 l = adsp2100.l[ireg];
	UINT16 res;

	if ( adsp2100.mstat & MSTAT_REVERSE )
	{
		UINT32 ir = reverse_table[ i & 0x3fff ];
		res = RWORD_DATA(ir);
	}
	else
		res = RWORD_DATA(i);
	
	i += adsp2100.m[mreg];
	if (i < base) i += l;
	else if (i >= base + l) i -= l;
	adsp2100.i[ireg] = i;
	
	return res;
}

INLINE void data_write_dag2(UINT32 op, INT32 val)
{
	UINT32 ireg = 4 + ((op >> 2) & 3);
	UINT32 mreg = 4 + (op & 3);
	UINT32 base = adsp2100.base[ireg];
	UINT32 i = adsp2100.i[ireg];
	UINT32 l = adsp2100.l[ireg];
	
	WWORD_DATA(i, val);
	
	i += adsp2100.m[mreg];
	if (i < base) i += l;
	else if (i >= base + l) i -= l;
	adsp2100.i[ireg] = i;
}
		

INLINE UINT16 data_read_dag2(UINT32 op)
{
	UINT32 ireg = 4 + ((op >> 2) & 3);
	UINT32 mreg = 4 + (op & 3);
	UINT32 base = adsp2100.base[ireg];
	UINT32 i = adsp2100.i[ireg];
	UINT32 l = adsp2100.l[ireg];

	UINT16 res = RWORD_DATA(i);
	
	i += adsp2100.m[mreg];
	if (i < base) i += l;
	else if (i >= base + l) i -= l;
	adsp2100.i[ireg] = i;
	
	return res;
}
		
/*===========================================================================
	Program memory accessors
===========================================================================*/

INLINE void pgm_write_dag2(UINT32 op, INT32 val)
{
	UINT32 ireg = 4 + ((op >> 2) & 3);
	UINT32 mreg = 4 + (op & 3);
	UINT32 base = adsp2100.base[ireg];
	UINT32 i = adsp2100.i[ireg];
	UINT32 l = adsp2100.l[ireg];
	
	WWORD_PGM(i, ((val&0xffff) << 8) | adsp2100.px);

	i += adsp2100.m[mreg];
	if (i < base) i += l;
	else if (i >= base + l) i -= l;
	adsp2100.i[ireg] = i;
}
		

INLINE UINT16 pgm_read_dag2(UINT32 op)
{
	UINT32 ireg = 4 + ((op >> 2) & 3);
	UINT32 mreg = 4 + (op & 3);
	UINT32 base = adsp2100.base[ireg];
	UINT32 i = adsp2100.i[ireg];
	UINT32 l = adsp2100.l[ireg];
	UINT32 res;

	res = RWORD_PGM(i);
	adsp2100.px = res;
	res >>= 8;
	
	i += adsp2100.m[mreg];
	if (i < base) i += l;
	else if (i >= base + l) i -= l;
	adsp2100.i[ireg] = i;
	
	return res;
}
		


/*===========================================================================
	ALU register reading
===========================================================================*/

#if SWAPCORE
#define ALU_GETXREG_UNSIGNED(x) (*(UINT16 *)((UINT8 *)&adsp2100.r[0] + alu_xregs[x]))
#define ALU_GETXREG_SIGNED(x)	(*( INT16 *)((UINT8 *)&adsp2100.r[0] + alu_xregs[x]))
#define ALU_GETYREG_UNSIGNED(y) (*(UINT16 *)((UINT8 *)&adsp2100.r[0] + alu_yregs[y]))
#define ALU_GETYREG_SIGNED(y)	(*( INT16 *)((UINT8 *)&adsp2100.r[0] + alu_yregs[y]))
#else
#define ALU_GETXREG_UNSIGNED(x) (*(UINT16 *)((UINT8 *)core + alu_xregs[x]))
#define ALU_GETXREG_SIGNED(x)   (*( INT16 *)((UINT8 *)core + alu_xregs[x]))
#define ALU_GETYREG_UNSIGNED(y) (*(UINT16 *)((UINT8 *)core + alu_yregs[y]))
#define ALU_GETYREG_SIGNED(y)   (*( INT16 *)((UINT8 *)core + alu_yregs[y]))
#endif

static const UINT32 alu_xregs[8] =
{
	offsetof(ADSPCORE, ax0),
	offsetof(ADSPCORE, ax1),
	offsetof(ADSPCORE, ar),
	offsetof(ADSPCORE, mr.mrx.mr0),
	offsetof(ADSPCORE, mr.mrx.mr1),
	offsetof(ADSPCORE, mr.mrx.mr2),
	offsetof(ADSPCORE, sr.srx.sr0),
	offsetof(ADSPCORE, sr.srx.sr1)
};

static const UINT32 alu_yregs[4] =
{
	offsetof(ADSPCORE, ay0),
	offsetof(ADSPCORE, ay1),
	offsetof(ADSPCORE, af),
	offsetof(ADSPCORE, zero)
};



/*===========================================================================
	MAC register reading
===========================================================================*/

#if SWAPCORE
#define MAC_GETXREG_UNSIGNED(x) (*(UINT16 *)((UINT8 *)&adsp2100.r[0] + mac_xregs[x]))
#define MAC_GETXREG_SIGNED(x)	(*( INT16 *)((UINT8 *)&adsp2100.r[0] + mac_xregs[x]))
#define MAC_GETYREG_UNSIGNED(y) (*(UINT16 *)((UINT8 *)&adsp2100.r[0] + mac_yregs[y]))
#define MAC_GETYREG_SIGNED(y)	(*( INT16 *)((UINT8 *)&adsp2100.r[0] + mac_yregs[y]))
#else
#define MAC_GETXREG_UNSIGNED(x) (*(UINT16 *)((UINT8 *)core + mac_xregs[x]))
#define MAC_GETXREG_SIGNED(x)   (*( INT16 *)((UINT8 *)core + mac_xregs[x]))
#define MAC_GETYREG_UNSIGNED(y) (*(UINT16 *)((UINT8 *)core + mac_yregs[y]))
#define MAC_GETYREG_SIGNED(y)   (*( INT16 *)((UINT8 *)core + mac_yregs[y]))
#endif

static const UINT32 mac_xregs[8] =
{
	offsetof(ADSPCORE, mx0),
	offsetof(ADSPCORE, mx1),
	offsetof(ADSPCORE, ar),
	offsetof(ADSPCORE, mr.mrx.mr0),
	offsetof(ADSPCORE, mr.mrx.mr1),
	offsetof(ADSPCORE, mr.mrx.mr2),
	offsetof(ADSPCORE, sr.srx.sr0),
	offsetof(ADSPCORE, sr.srx.sr1)
};

static const UINT32 mac_yregs[4] =
{
	offsetof(ADSPCORE, my0),
	offsetof(ADSPCORE, my1),
	offsetof(ADSPCORE, mf),
	offsetof(ADSPCORE, zero)
};



/*===========================================================================
	SHIFT register reading
===========================================================================*/

#if SWAPCORE
#define SHIFT_GETXREG_UNSIGNED(x) (*(UINT16 *)((UINT8 *)&adsp2100.r[0] + shift_xregs[x]))
#define SHIFT_GETXREG_SIGNED(x)   (*( INT16 *)((UINT8 *)&adsp2100.r[0] + shift_xregs[x]))
#else
#define SHIFT_GETXREG_UNSIGNED(x) (*(UINT16 *)((UINT8 *)core + shift_xregs[x]))
#define SHIFT_GETXREG_SIGNED(x)   (*( INT16 *)((UINT8 *)core + shift_xregs[x]))
#endif

static const UINT32 shift_xregs[8] =
{
	offsetof(ADSPCORE, si),
	offsetof(ADSPCORE, si),
	offsetof(ADSPCORE, ar),
	offsetof(ADSPCORE, mr.mrx.mr0),
	offsetof(ADSPCORE, mr.mrx.mr1),
	offsetof(ADSPCORE, mr.mrx.mr2),
	offsetof(ADSPCORE, sr.srx.sr0),
	offsetof(ADSPCORE, sr.srx.sr1)
};



/*===========================================================================
	ALU operations (result in AR)
===========================================================================*/

void alu_op_ar(int op)
{
	INT32 xop = (op >> 8) & 7;
	INT32 yop = (op >> 11) & 3;
	INT32 res;
	
	switch ((op >> 13) & 15)
	{
		case 0x00:
			/* Y				Clear when y = 0 */
			res = ALU_GETYREG_UNSIGNED(yop);
			CALC_NZ(res);
			break;
		case 0x01:
			/* Y + 1			PASS 1 when y = 0 */
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = yop + 1;
			CALC_NZ(res);
			if (yop == 0x7fff) SET_V;
			else if (yop == 0xffff) SET_C;
			break;
		case 0x02:
			/* X + Y + C */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			yop += GET_C >> 3;
			res = xop + yop;
			CALC_NZVC(xop, yop, res);
			break;
		case 0x03:
			/* X + Y			X when y = 0 */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop + yop;
			CALC_NZVC(xop, yop, res);
			break;
		case 0x04:
			/* NOT Y */
			res = ALU_GETYREG_UNSIGNED(yop) ^ 0xffff;
			CALC_NZ(res);
			break;
		case 0x05:
			/* -Y */
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = -yop;
			CALC_NZ(res);
			if (yop == 0x8000) SET_V;
			if (yop == 0x0000) SET_C;
			break;
		case 0x06:
			/* X - Y + C - 1	X + C - 1 when y = 0 */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			yop -= (GET_C >> 3) - 1;
			res = xop - yop;
			CALC_NZVC_SUB(xop, yop, res);
			break;
		case 0x07:
			/* X - Y */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop - yop;
			CALC_NZVC_SUB(xop, yop, res);
			break;
		case 0x08:
			/* Y - 1			PASS -1 when y = 0 */
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = yop - 1;
			CALC_NZ(res);
			if (yop == 0x8000) SET_V;
			else if (yop == 0x0000) SET_C;
			break;
		case 0x09:
			/* Y - X			-X when y = 0 */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = yop - xop;
			CALC_NZVC_SUB(yop, xop, res);
			break;
		case 0x0a:
			/* Y - X + C - 1	-X + C - 1 when y = 0 */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			xop -= (GET_C >> 3) - 1;
			res = yop - xop;
			CALC_NZVC_SUB(yop, xop, res);
			break;
		case 0x0b:
			/* NOT X */
			res = ALU_GETXREG_UNSIGNED(xop) ^ 0xffff;
			CALC_NZ(res);
			break;
		case 0x0c:
			/* X AND Y */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop & yop;
			CALC_NZ(res);
			break;
		case 0x0d:
			/* X OR Y */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop | yop;
			CALC_NZ(res);
			break;
		case 0x0e:
			/* X XOR Y */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop ^ yop;
			CALC_NZ(res);
			break;
		case 0x0f:
			/* ABS X */
			xop = ALU_GETXREG_UNSIGNED(xop);
			res = (xop & 0x8000) ? -xop : xop;
			if (xop == 0) SET_Z;
			if (xop == 0x8000) SET_N, SET_V;
			CLR_S;
			if (xop & 0x8000) SET_S;
			break;
		default:
			res = 0;	/* just to keep the compiler happy */
			break;
	}

	/* saturate */
	if ((adsp2100.mstat & MSTAT_SATURATE) && GET_V) res = GET_C ? -32768 : 32767;

	/* set the final value */	
	CORE(ar.u) = res;
}



/*===========================================================================
	ALU operations (result in AF)
===========================================================================*/

void alu_op_af(int op)
{
	INT32 xop = (op >> 8) & 7;
	INT32 yop = (op >> 11) & 3;
	INT32 res;
	
	switch ((op >> 13) & 15)
	{
		case 0x00:
			/* Y				Clear when y = 0 */
			res = ALU_GETYREG_UNSIGNED(yop);
			CALC_NZ(res);
			break;
		case 0x01:
			/* Y + 1			PASS 1 when y = 0 */
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = yop + 1;
			CALC_NZ(res);
			if (yop == 0x7fff) SET_V;
			else if (yop == 0xffff) SET_C;
			break;
		case 0x02:
			/* X + Y + C */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			yop += GET_C >> 3;
			res = xop + yop;
			CALC_NZVC(xop, yop, res);
			break;
		case 0x03:
			/* X + Y			X when y = 0 */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop + yop;
			CALC_NZVC(xop, yop, res);
			break;
		case 0x04:
			/* NOT Y */
			res = ALU_GETYREG_UNSIGNED(yop) ^ 0xffff;
			CALC_NZ(res);
			break;
		case 0x05:
			/* -Y */
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = -yop;
			CALC_NZ(res);
			if (yop == 0x8000) SET_V;
			if (yop == 0x0000) SET_C;
			break;
		case 0x06:
			/* X - Y + C - 1	X + C - 1 when y = 0 */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			yop -= (GET_C >> 3) - 1;
			res = xop - yop;
			CALC_NZVC_SUB(xop, yop, res);
			break;
		case 0x07:
			/* X - Y */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop - yop;
			CALC_NZVC_SUB(xop, yop, res);
			break;
		case 0x08:
			/* Y - 1			PASS -1 when y = 0 */
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = yop - 1;
			CALC_NZ(res);
			if (yop == 0x8000) SET_V;
			else if (yop == 0x0000) SET_C;
			break;
		case 0x09:
			/* Y - X			-X when y = 0 */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = yop - xop;
			CALC_NZVC_SUB(yop, xop, res);
			break;
		case 0x0a:
			/* Y - X + C - 1	-X + C - 1 when y = 0 */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			xop -= (GET_C >> 3) - 1;
			res = yop - xop;
			CALC_NZVC_SUB(yop, xop, res);
			break;
		case 0x0b:
			/* NOT X */
			res = ALU_GETXREG_UNSIGNED(xop) ^ 0xffff;
			CALC_NZ(res);
			break;
		case 0x0c:
			/* X AND Y */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop & yop;
			CALC_NZ(res);
			break;
		case 0x0d:
			/* X OR Y */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop | yop;
			CALC_NZ(res);
			break;
		case 0x0e:
			/* X XOR Y */
			xop = ALU_GETXREG_UNSIGNED(xop);
			yop = ALU_GETYREG_UNSIGNED(yop);
			res = xop ^ yop;
			CALC_NZ(res);
			break;
		case 0x0f:
			/* ABS X */
			xop = ALU_GETXREG_UNSIGNED(xop);
			res = (xop & 0x8000) ? -xop : xop;
			if (xop == 0) SET_Z;
			if (xop == 0x8000) SET_N, SET_V;
			CLR_S;
			if (xop & 0x8000) SET_S;
			break;
		default:
			res = 0;	/* just to keep the compiler happy */
			break;
	}

	/* set the final value */	
	CORE(af.u) = res;
}



/*===========================================================================
	MAC operations (result in MR)
===========================================================================*/

void mac_op_mr(int op)
{
#if SUPPORT_2101_EXTENSIONS
	INT8 shift = ((adsp2100.mstat & MSTAT_INTEGER) >> 4) ^ 1;
#else
	INT8 shift = 1;
#endif
	INT32 xop = (op >> 8) & 7;
	INT32 yop = (op >> 11) & 3;
	INT32 temp;
	INT64 res;
	
	switch ((op >> 13) & 15)
	{
		case 0x00:
			/* no-op */
			return;
		case 0x01:
			/* X * Y (RND) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = (INT64)temp;
#if 0
			if ((res & 0xffff) == 0x8000) res &= ~((UINT64)0x10000);
			else res += (res & 0x8000) << 1;
#else
			temp &= 0xffff;
			res += 0x8000;
			if ( temp == 0x8000 )
				res &= ~((UINT64)0x10000);
#endif
			break;
		case 0x02:
			/* MR + X * Y (RND) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
#if 0
			if ((res & 0xffff) == 0x8000) res &= ~((UINT64)0x10000);
			else res += (res & 0x8000) << 1;
#else
			temp &= 0xffff;
			res += 0x8000;
			if ( temp == 0x8000 )
				res &= ~((UINT64)0x10000);
#endif
			break;
		case 0x03:
			/* MR - X * Y (RND) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
#if 0
			if ((res & 0xffff) == 0x8000) res &= ~((UINT64)0x10000);
			else res += (res & 0x8000) << 1;
#else
			temp &= 0xffff;
			res += 0x8000;
			if ( temp == 0x8000 )
				res &= ~((UINT64)0x10000);
#endif
			break;
		case 0x04:
			/* X * Y (SS)		Clear when y = 0 */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = (INT64)temp;
			break;
		case 0x05:
			/* X * Y (SU) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = (xop * yop) << shift;
			res = (INT64)temp;
			break;
		case 0x06:
			/* X * Y (US) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = (INT64)temp;
			break;
		case 0x07:
			/* X * Y (UU) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = ((UINT16)xop * (UINT16)yop) << shift;
			res = (INT64)temp;
			break;
		case 0x08:
			/* MR + X * Y (SS) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
			break;
		case 0x09:
			/* MR + X * Y (SU) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
			break;
		case 0x0a:
			/* MR + X * Y (US) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
			break;
		case 0x0b:
			/* MR + X * Y (UU) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = ((UINT16)xop * (UINT16)yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
			break;
		case 0x0c:
			/* MR - X * Y (SS) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
			break;
		case 0x0d:
			/* MR - X * Y (SU) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
			break;
		case 0x0e:
			/* MR - X * Y (US) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
			break;
		case 0x0f:
			/* MR - X * Y (UU) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = ((UINT16)xop * (UINT16)yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
			break;
		default:
			res = 0;	/* just to keep the compiler happy */
			break;
	}

	/* set the final value */
	temp = (res >> 31) & 0x1ff;
	CLR_MV;
	if (temp != 0x000 && temp != 0x1ff) SET_MV;
	CORE(mr.mr) = res;
	CORE(mr.mrx.mrzero.u) = 0;
}



/*===========================================================================
	MAC operations (result in MF)
===========================================================================*/

void mac_op_mf(int op)
{
#if SUPPORT_2101_EXTENSIONS
	INT8 shift = ((adsp2100.mstat & MSTAT_INTEGER) >> 4) ^ 1;
#else
	INT8 shift = 1;
#endif
	INT32 xop = (op >> 8) & 7;
	INT32 yop = (op >> 11) & 3;
	INT32 temp;
	INT64 res;
	
	switch ((op >> 13) & 15)
	{
		case 0x00:
			/* no-op */
			return;
		case 0x01:
			/* X * Y (RND) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = (INT64)temp;
#if 0
			if ((res & 0xffff) == 0x8000) res &= ~((UINT64)0x10000);
			else res += (res & 0x8000) << 1;
#else
			temp &= 0xffff;
			res += 0x8000;
			if ( temp == 0x8000 )
				res &= ~((UINT64)0x10000);
#endif
			break;
		case 0x02:
			/* MR + X * Y (RND) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
#if 0
			if ((res & 0xffff) == 0x8000) res &= ~((UINT64)0x10000);
			else res += (res & 0x8000) << 1;
#else
			temp &= 0xffff;
			res += 0x8000;
			if ( temp == 0x8000 )
				res &= ~((UINT64)0x10000);
#endif
			break;
		case 0x03:
			/* MR - X * Y (RND) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
#if 0
			if ((res & 0xffff) == 0x8000) res &= ~((UINT64)0x10000);
			else res += (res & 0x8000) << 1;
#else
			temp &= 0xffff;
			res += 0x8000;
			if ( temp == 0x8000 )
				res &= ~((UINT64)0x10000);
#endif
			break;
		case 0x04:
			/* X * Y (SS)		Clear when y = 0 */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = (INT64)temp;
			break;
		case 0x05:
			/* X * Y (SU) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = (xop * yop) << shift;
			res = (INT64)temp;
			break;
		case 0x06:
			/* X * Y (US) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = (INT64)temp;
			break;
		case 0x07:
			/* X * Y (UU) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = ((UINT16)xop * (UINT16)yop) << shift;
			res = (INT64)temp;
			break;
		case 0x08:
			/* MR + X * Y (SS) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
			break;
		case 0x09:
			/* MR + X * Y (SU) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
			break;
		case 0x0a:
			/* MR + X * Y (US) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
			break;
		case 0x0b:
			/* MR + X * Y (UU) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = ((UINT16)xop * (UINT16)yop) << shift;
			res = CORE(mr.mr) + (INT64)temp;
			break;
		case 0x0c:
			/* MR - X * Y (SS) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
			break;
		case 0x0d:
			/* MR - X * Y (SU) */
			xop = MAC_GETXREG_SIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
			break;
		case 0x0e:
			/* MR - X * Y (US) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_SIGNED(yop);
			temp = (xop * yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
			break;
		case 0x0f:
			/* MR - X * Y (UU) */
			xop = MAC_GETXREG_UNSIGNED(xop);
			yop = MAC_GETYREG_UNSIGNED(yop);
			temp = ((UINT16)xop * (UINT16)yop) << shift;
			res = CORE(mr.mr) - (INT64)temp;
			break;
		default:
			res = 0;	/* just to keep the compiler happy */
			break;
	}

	/* set the final value */
	CORE(mf.u) = (UINT32)res >> 16;
}



/*===========================================================================
	SHIFT operations (result in SR/SE/SB)
===========================================================================*/

void shift_op(int op)
{
	INT8 sc = CORE(se.s);
	INT32 xop = (op >> 8) & 7;
	UINT32 res;
	
	switch ((op >> 11) & 15)
	{
		case 0x00:
			/* LSHIFT (HI) */
			xop = SHIFT_GETXREG_UNSIGNED(xop) << 16;
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? ((UINT32)xop >> -sc) : 0;
			CORE(sr.sr) = res;
			break;
		case 0x01:
			/* LSHIFT (HI, OR) */
			xop = SHIFT_GETXREG_UNSIGNED(xop) << 16;
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? ((UINT32)xop >> -sc) : 0;
			CORE(sr.sr) |= res;
			break;
		case 0x02:
			/* LSHIFT (LO) */
			xop = SHIFT_GETXREG_UNSIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : 0;
			CORE(sr.sr) = res;
			break;
		case 0x03:
			/* LSHIFT (LO, OR) */
			xop = SHIFT_GETXREG_UNSIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : 0;
			CORE(sr.sr) |= res;
			break;
		case 0x04:
			/* ASHIFT (HI) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : (xop >> 31);
			CORE(sr.sr) = res;
			break;
		case 0x05:
			/* ASHIFT (HI, OR) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : (xop >> 31);
			CORE(sr.sr) |= res;
			break;
		case 0x06:
			/* ASHIFT (LO) */
			xop = SHIFT_GETXREG_SIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : (xop >> 31);
			CORE(sr.sr) = res;
			break;
		case 0x07:
			/* ASHIFT (LO, OR) */
			xop = SHIFT_GETXREG_SIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : (xop >> 31);
			CORE(sr.sr) |= res;
			break;
		case 0x08:
			/* NORM (HI) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			if (sc > 0) 
			{ 
				xop = ((UINT32)xop >> 1) | ((adsp2100.astat & CFLAG) << 28); 
				res = xop >> (sc - 1);
			}
			else res = (sc > -32) ? (xop << -sc) : 0;
			CORE(sr.sr) = res;
			break;
		case 0x09:
			/* NORM (HI, OR) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			if (sc > 0) 
			{ 
				xop = ((UINT32)xop >> 1) | ((adsp2100.astat & CFLAG) << 28); 
				res = xop >> (sc - 1);
			}
			else res = (sc > -32) ? (xop << -sc) : 0;
			CORE(sr.sr) |= res;
			break;
		case 0x0a:
			/* NORM (LO) */
			xop = SHIFT_GETXREG_UNSIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop >> sc) : 0;
			else res = (sc > -32) ? (xop << -sc) : 0;
			CORE(sr.sr) = res;
			break;
		case 0x0b:
			/* NORM (LO, OR) */
			xop = SHIFT_GETXREG_UNSIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop >> sc) : 0;
			else res = (sc > -32) ? (xop << -sc) : 0;
			CORE(sr.sr) |= res;
			break;
		case 0x0c:
			/* EXP (HI) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			res = 0;
			if (xop < 0)
			{
				SET_SS;
				while ((xop & 0x40000000) != 0) res++, xop <<= 1;
			}
			else
			{
				CLR_SS;
				xop |= 0x8000;
				while ((xop & 0x40000000) == 0) res++, xop <<= 1;
			}
			CORE(se.s) = -res;
			break;
		case 0x0d:
			/* EXP (HIX) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			if (GET_V)
			{
				CORE(se.s) = 1;
				if (xop < 0) CLR_SS;
				else SET_SS;
			}
			else
			{
				res = 0;
				if (xop < 0)
				{
					SET_SS;
					while ((xop & 0x40000000) != 0) res++, xop <<= 1;
				}
				else
				{
					CLR_SS;
					xop |= 0x8000;
					while ((xop & 0x40000000) == 0) res++, xop <<= 1;
				}
				CORE(se.s) = -res;
			}
			break;
		case 0x0e:
			/* EXP (LO) */
			if (CORE(se.s) == -15)
			{
				xop = SHIFT_GETXREG_SIGNED(xop);
				res = 15;
				if (GET_SS)
					while ((xop & 0x8000) != 0) res++, xop <<= 1;
				else
				{
					xop = (xop << 1) | 1;
					while ((xop & 0x10000) == 0) res++, xop <<= 1;
				}
				CORE(se.s) = -res;
			}
			break;
		case 0x0f:
			/* EXPADJ */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			res = 0;
			if (xop < 0)
				while ((xop & 0x40000000) != 0) res++, xop <<= 1;
			else
			{
				xop |= 0x8000;
				while ((xop & 0x40000000) == 0) res++, xop <<= 1;
			}
			if (res < -CORE(sb.s))
				CORE(sb.s) = -res;
			break;
	}
}



/*===========================================================================
	Immediate SHIFT operations (result in SR/SE/SB)
===========================================================================*/

void shift_op_imm(int op)
{
	INT8 sc = (INT8)op;
	INT32 xop = (op >> 8) & 7;
	UINT32 res;
	
	switch ((op >> 11) & 15)
	{
		case 0x00:
			/* LSHIFT (HI) */
			xop = SHIFT_GETXREG_UNSIGNED(xop) << 16;
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? ((UINT32)xop >> -sc) : 0;
			CORE(sr.sr) = res;
			break;
		case 0x01:
			/* LSHIFT (HI, OR) */
			xop = SHIFT_GETXREG_UNSIGNED(xop) << 16;
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? ((UINT32)xop >> -sc) : 0;
			CORE(sr.sr) |= res;
			break;
		case 0x02:
			/* LSHIFT (LO) */
			xop = SHIFT_GETXREG_UNSIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : 0;
			CORE(sr.sr) = res;
			break;
		case 0x03:
			/* LSHIFT (LO, OR) */
			xop = SHIFT_GETXREG_UNSIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : 0;
			CORE(sr.sr) |= res;
			break;
		case 0x04:
			/* ASHIFT (HI) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : (xop >> 31);
			CORE(sr.sr) = res;
			break;
		case 0x05:
			/* ASHIFT (HI, OR) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : (xop >> 31);
			CORE(sr.sr) |= res;
			break;
		case 0x06:
			/* ASHIFT (LO) */
			xop = SHIFT_GETXREG_SIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : (xop >> 31);
			CORE(sr.sr) = res;
			break;
		case 0x07:
			/* ASHIFT (LO, OR) */
			xop = SHIFT_GETXREG_SIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop << sc) : 0;
			else res = (sc > -32) ? (xop >> -sc) : (xop >> 31);
			CORE(sr.sr) |= res;
			break;
		case 0x08:
			/* NORM (HI) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			if (sc > 0) 
			{ 
				xop = ((UINT32)xop >> 1) | ((adsp2100.astat & CFLAG) << 28); 
				res = xop >> (sc - 1);
			}
			else res = (sc > -32) ? (xop << -sc) : 0;
			CORE(sr.sr) = res;
			break;
		case 0x09:
			/* NORM (HI, OR) */
			xop = SHIFT_GETXREG_SIGNED(xop) << 16;
			if (sc > 0) 
			{ 
				xop = ((UINT32)xop >> 1) | ((adsp2100.astat & CFLAG) << 28); 
				res = xop >> (sc - 1);
			}
			else res = (sc > -32) ? (xop << -sc) : 0;
			CORE(sr.sr) |= res;
			break;
		case 0x0a:
			/* NORM (LO) */
			xop = SHIFT_GETXREG_UNSIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop >> sc) : 0;
			else res = (sc > -32) ? (xop << -sc) : 0;
			CORE(sr.sr) = res;
			break;
		case 0x0b:
			/* NORM (LO, OR) */
			xop = SHIFT_GETXREG_UNSIGNED(xop);
			if (sc > 0) res = (sc < 32) ? (xop >> sc) : 0;
			else res = (sc > -32) ? (xop << -sc) : 0;
			CORE(sr.sr) |= res;
			break;
	}
}
