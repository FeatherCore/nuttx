############################################################################
# boards/arm/stm32n6/stm32n6570-dk/kernel/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

include $(TOPDIR)/Make.defs

CONFIG_INIT_ENTRYPOINT ?= user_start
ENTRYPT = $(patsubst "%",%,$(CONFIG_INIT_ENTRYPOINT))

USER_LIBPATHS = $(addprefix -L,$(call CONVERT_PATH,$(addprefix $(TOPDIR)$(DELIM),$(dir $(USERLIBS)))))
USER_LDSCRIPT = $(call CONVERT_PATH,$(BOARD_DIR)$(DELIM)scripts$(DELIM)memory.ld)
USER_LDSCRIPT += $(call CONVERT_PATH,$(BOARD_DIR)$(DELIM)scripts$(DELIM)user-space.ld)
USER_HEXFILE += $(call CONVERT_PATH,$(TOPDIR)$(DELIM)nuttx_user.hex)
USER_SRECFILE += $(call CONVERT_PATH,$(TOPDIR)$(DELIM)nuttx_user.srec)
USER_BINFILE += $(call CONVERT_PATH,$(TOPDIR)$(DELIM)nuttx_user.bin)

USER_LDFLAGS = --undefined=$(ENTRYPT) --entry=$(ENTRYPT)
USER_LDFLAGS += $(foreach script,$(addsuffix .tmp,$(USER_LDSCRIPT)),-T $(script))
USER_LDLIBS = $(patsubst lib%,-l%,$(basename $(notdir $(USERLIBS))))
USER_LIBGCC = "${shell "$(CC)" $(ARCHCPUFLAGS) -print-libgcc-file-name}"

CSRCS = stm32n6_userspace.c
COBJS = $(CSRCS:.c=$(OBJEXT))
OBJS  = $(COBJS)

all: $(TOPDIR)$(DELIM)nuttx_user.elf $(TOPDIR)$(DELIM)User.map
.PHONY: nuttx_user.elf depend clean distclean

$(COBJS): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

$(addsuffix .tmp,$(USER_LDSCRIPT)): $(USER_LDSCRIPT)
	$(call PREPROCESS,$(patsubst %.tmp,%,$@),$@)

nuttx_user.elf: $(OBJS) $(addsuffix .tmp,$(USER_LDSCRIPT))
	$(Q) $(LD) -o $@ $(USER_LDFLAGS) $(USER_LIBPATHS) $(OBJS) --start-group $(USER_LDLIBS) --end-group $(USER_LIBGCC)
	$(Q) $(call DELFILE, $(addsuffix .tmp,$(USER_LDSCRIPT)))

$(TOPDIR)$(DELIM)nuttx_user.elf: nuttx_user.elf
	@echo "LD: nuttx_user.elf"
	$(Q) cp -a nuttx_user.elf $(TOPDIR)$(DELIM)nuttx_user.elf
ifeq ($(CONFIG_INTELHEX_BINARY),y)
	@echo "CP: nuttx_user.hex"
	$(Q) $(OBJCOPY) $(OBJCOPYARGS) -O ihex nuttx_user.elf $(USER_HEXFILE)
endif
ifeq ($(CONFIG_MOTOROLA_SREC),y)
	@echo "CP: nuttx_user.srec"
	$(Q) $(OBJCOPY) $(OBJCOPYARGS) -O srec nuttx_user.elf $(USER_SRECFILE)
endif
ifeq ($(CONFIG_RAW_BINARY),y)
	@echo "CP: nuttx_user.bin"
	$(Q) $(OBJCOPY) $(OBJCOPYARGS) -O binary nuttx_user.elf $(USER_BINFILE)
endif

$(TOPDIR)$(DELIM)User.map: nuttx_user.elf
	@echo "MK: User.map"
	$(Q) $(NM) nuttx_user.elf >$(TOPDIR)$(DELIM)User.map
	$(Q) $(CROSSDEV)size nuttx_user.elf

.depend:

depend: .depend

clean:
	$(call DELFILE, nuttx_user.elf)
	$(call DELFILE, $(addsuffix .tmp,$(USER_LDSCRIPT)))
	$(call DELFILE, "$(TOPDIR)$(DELIM)nuttx_user.*")
	$(call DELFILE, "$(TOPDIR)$(DELIM)User.map")
	$(call CLEAN)

distclean: clean
