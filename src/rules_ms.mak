# Rules for MESS CPU's

CPU=$(strip $(findstring APEXC@,$(CPUS)))
ifneq ($(CPU),)
APEXCD = mess/cpu/apexc
OBJDIRS += $(OBJ)/$(APEXCD)
CPUDEFS += -DHAS_APEXC=1
CPUOBJS += $(OBJ)/$(APEXCD)/apexc.o
DBGOBJS += $(OBJ)/$(APEXCD)/apexcdsm.o
$(OBJ)/$(APEXCD)/apexc.o: $(APEXCD)/apexc.c $(APEXCD)/apexc.h
else
CPUDEFS += -DHAS_APEXC=0
endif


CPU=$(strip $(findstring ARM@,$(CPUS)))
ifneq ($(CPU),)
ARMD = mess/cpu/arm
OBJDIRS += $(OBJ)/$(ARMD)
CPUDEFS += -DHAS_ARM=1
CPUOBJS += $(OBJ)/$(ARMD)/arm.o
DBGOBJS += $(OBJ)/$(ARMD)/dasm.o
$(OBJ)/$(ARMD)/arm.o: $(ARMD)/arm.h
else
CPUDEFS += -DHAS_ARM=0
endif


CPU=$(strip $(findstring G65816@,$(CPUS)))
ifneq ($(CPU),)
G6D = mess/cpu/g65816
OBJDIRS += $(OBJ)/$(G6D)
CPUDEFS += -DHAS_G65816=1
CPUOBJS += $(OBJ)/$(G6D)/g65816.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o0.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o1.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o2.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o3.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o4.o
DBGOBJS += $(OBJ)/$(G6D)/g65816ds.o
$(OBJ)/$(G6D)/g65816.o: $(G6D)/g65816.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o0.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o1.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o2.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o3.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o4.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
else
CPUDEFS += -DHAS_G65816=0
endif


CPU=$(strip $(findstring PDP1@,$(CPUS)))
ifneq ($(CPU),)
PDPD = mess/cpu/pdp1
OBJDIRS += $(OBJ)/$(PDPD)
CPUDEFS += -DHAS_PDP1=1
CPUOBJS += $(OBJ)/$(PDPD)/pdp1.o
DBGOBJS += $(OBJ)/$(PDPD)/pdp1dasm.o
$(OBJ)/$(PDPD)/pdp1.o: $(PDPD)/pdp1.c $(PDPD)/pdp1.h
else
CPUDEFS += -DHAS_PDP1=0
endif


CPU=$(strip $(findstring SC61860@,$(CPUS)))
ifneq ($(CPU),)
SCD = mess/cpu/sc61860
OBJDIRS += $(OBJ)/$(SCD)
CPUDEFS += -DHAS_SC61860=1
CPUOBJS += $(OBJ)/$(SCD)/sc61860.o
DBGOBJS += $(OBJ)/$(SCD)/scdasm.o
$(OBJ)/$(SCD)/sc61860.o: $(SCD)/sc61860.h  $(SCD)/sc.h $(SCD)/scops.c $(SCD)/sctable.c
else
CPUDEFS += -DHAS_SC61860=0
endif


CPU=$(strip $(findstring SH2@,$(CPUS)))
ifneq ($(CPU),)
SH2D = mess/cpu/sh2
OBJDIRS += $(OBJ)/$(SH2D)
CPUDEFS += -DHAS_SH2=1
CPUOBJS += $(OBJ)/$(SH2D)/sh2.o
DBGOBJS += $(OBJ)/$(SH2D)/sh2dasm.o
$(OBJ)/$(SH2D)/sh2.o: $(SH2D)/sh2.c $(SH2D)/sh2.h
else
CPUDEFS += -DHAS_SH2=0
endif


