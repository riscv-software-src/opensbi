#
# Copyright (c) 2018 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#

# Current Version
MAJOR = 0
MINOR = 1

# Select Make Options:
# o  Do not use make's built-in rules and variables
# o  Do not print "Entering directory ...";
MAKEFLAGS += -rR --no-print-directory

# Find out source, build, and install directories
src_dir=$(CURDIR)
ifdef O
 build_dir=$(shell readlink -f $(O))
else
 build_dir=$(CURDIR)/build
endif
ifeq ($(build_dir),$(CURDIR))
$(error Build directory is same as source directory.)
endif
ifdef I
 install_dir=$(shell readlink -f $(I))
else
 install_dir=$(CURDIR)/install
endif
ifeq ($(install_dir),$(CURDIR))
$(error Install directory is same as source directory.)
endif
ifeq ($(install_dir),$(build_dir))
$(error Install directory is same as build directory.)
endif

# Check if verbosity is ON for build process
VERBOSE_DEFAULT    := 0
CMD_PREFIX_DEFAULT := @
ifdef VERBOSE
	ifeq ("$(origin VERBOSE)", "command line")
		VB := $(VERBOSE)
	else
		VB := $(VERBOSE_DEFAULT)
	endif
else
	VB := $(VERBOSE_DEFAULT)
endif
ifeq ($(VB), 1)
	override V :=
else
	override V := $(CMD_PREFIX_DEFAULT)
endif

# Setup path of directories
export platform_subdir=platform/$(PLATFORM)
export platform_dir=$(CURDIR)/$(platform_subdir)
export platform_common_dir=$(CURDIR)/platform/common
export include_dir=$(CURDIR)/include
export lib_dir=$(CURDIR)/lib
export firmware_dir=$(CURDIR)/firmware

# Setup list of objects.mk files
ifdef PLATFORM
platform-object-mks=$(shell if [ -d $(platform_dir) ]; then find $(platform_dir) -iname "objects.mk" | sort -r; fi)
platform-common-object-mks=$(shell if [ -d $(platform_common_dir) ]; then find $(platform_common_dir) -iname "objects.mk" | sort -r; fi)
endif
lib-object-mks=$(shell if [ -d $(lib_dir) ]; then find $(lib_dir) -iname "objects.mk" | sort -r; fi)
firmware-object-mks=$(shell if [ -d $(firmware_dir) ]; then find $(firmware_dir) -iname "objects.mk" | sort -r; fi)

# Include platform specifig config.mk
ifdef PLATFORM
include $(platform_dir)/config.mk
endif

# Include all object.mk files
ifdef PLATFORM
include $(platform-object-mks)
include $(platform-common-object-mks)
endif
include $(lib-object-mks)
include $(firmware-object-mks)

# Setup list of objects
lib-objs-path-y=$(foreach obj,$(lib-objs-y),$(build_dir)/lib/$(obj))
ifdef PLATFORM
platform-objs-path-y=$(foreach obj,$(platform-objs-y),$(build_dir)/$(platform_subdir)/$(obj))
platform-common-objs-path-y=$(foreach obj,$(platform-common-objs-y),$(build_dir)/platform/common/$(obj))
firmware-bins-path-y=$(foreach bin,$(firmware-bins-y),$(build_dir)/$(platform_subdir)/firmware/$(bin))
endif
firmware-elfs-path-y=$(firmware-bins-path-y:.bin=.elf)
firmware-objs-path-y=$(firmware-bins-path-y:.bin=.o)

# Setup list of deps files for objects
deps-y=$(platform-objs-path-y:.o=.dep)
deps-y+=$(platform-common-objs-path-y:.o=.dep)
deps-y+=$(lib-objs-path-y:.o=.dep)
deps-y+=$(firmware-objs-path-y:.o=.dep)

# Setup compilation environment
cpp=$(CROSS_COMPILE)cpp
cppflags+=-DOPENSBI_MAJOR=$(MAJOR)
cppflags+=-DOPENSBI_MINOR=$(MINOR)
cppflags+=-I$(platform_dir)/include
cppflags+=-I$(platform_common_dir)/include
cppflags+=-I$(include_dir)
cppflags+=$(platform-cppflags-y)
cppflags+=$(firmware-cppflags-y)
cc=$(CROSS_COMPILE)gcc
cflags=-g -Wall -Werror -nostdlib -fno-strict-aliasing -O2
cflags+=-fno-omit-frame-pointer -fno-optimize-sibling-calls
cflags+=-mno-save-restore -mstrict-align
cflags+=$(cppflags)
cflags+=$(platform-cflags-y)
cflags+=$(firmware-cflags-y)
cflags+=$(EXTRA_CFLAGS)
as=$(CROSS_COMPILE)gcc
asflags=-g -Wall -nostdlib -D__ASSEMBLY__
asflags+=-fno-omit-frame-pointer -fno-optimize-sibling-calls
asflags+=-mno-save-restore -mstrict-align
asflags+=$(cppflags)
asflags+=$(platform-asflags-y)
asflags+=$(firmware-asflags-y)
asflags+=$(EXTRA_ASFLAGS)
ar=$(CROSS_COMPILE)ar
arflags=rcs
ld=$(CROSS_COMPILE)gcc
ldflags=-g -Wall -nostdlib -Wl,--build-id=none
ldflags+=$(platform-ldflags-y)
ldflags+=$(firmware-ldflags-y)
merge=$(CROSS_COMPILE)ld
mergeflags=-r
objcopy=$(CROSS_COMPILE)objcopy

# Setup functions for compilation
define dynamic_flags
-I$(shell dirname $(2)) -D__OBJNAME__=$(subst -,_,$(shell basename $(1) .o))
endef
merge_objs = $(V)mkdir -p `dirname $(1)`; \
	     echo " MERGE     $(subst $(build_dir)/,,$(1))"; \
	     $(merge) $(mergeflags) $(2) -o $(1)
merge_deps = $(V)mkdir -p `dirname $(1)`; \
	     echo " MERGE-DEP $(subst $(build_dir)/,,$(1))"; \
	     cat $(2) > $(1)
copy_file =  $(V)mkdir -p `dirname $(1)`; \
	     echo " COPY      $(subst $(build_dir)/,,$(1))"; \
	     cp -f $(2) $(1)
inst_file =  $(V)mkdir -p `dirname $(1)`; \
	     echo " INSTALL   $(subst $(install_dir)/,,$(1))"; \
	     cp -f $(2) $(1)
inst_file_list = $(V)if [ ! -z "$(3)" ]; then \
	     mkdir -p $(1); \
	     for f in $(3) ; do \
	     echo " INSTALL   "$(2)"/"`basename $$f`; \
	     cp -f $$f $(1); \
	     done \
	     fi
inst_header_dir =  $(V)mkdir -p $(1); \
	     echo " INSTALL   $(subst $(install_dir)/,,$(1))"; \
	     cp -rf $(2) $(1)
compile_cpp = $(V)mkdir -p `dirname $(1)`; \
	     echo " CPP       $(subst $(build_dir)/,,$(1))"; \
	     $(cpp) $(cppflags) $(2) | grep -v "\#" > $(1)
compile_cc_dep = $(V)mkdir -p `dirname $(1)`; \
	     echo " CC-DEP    $(subst $(build_dir)/,,$(1))"; \
	     echo -n `dirname $(1)`/ > $(1) && \
	     $(cc) $(cflags) $(call dynamic_flags,$(1),$(2))   \
	       -MM $(2) >> $(1) || rm -f $(1)
compile_cc = $(V)mkdir -p `dirname $(1)`; \
	     echo " CC        $(subst $(build_dir)/,,$(1))"; \
	     $(cc) $(cflags) $(call dynamic_flags,$(1),$(2)) -c $(2) -o $(1)
compile_as_dep = $(V)mkdir -p `dirname $(1)`; \
	     echo " AS-DEP    $(subst $(build_dir)/,,$(1))"; \
	     echo -n `dirname $(1)`/ > $(1) && \
	     $(as) $(asflags) $(call dynamic_flags,$(1),$(2))  \
	       -MM $(2) >> $(1) || rm -f $(1)
compile_as = $(V)mkdir -p `dirname $(1)`; \
	     echo " AS        $(subst $(build_dir)/,,$(1))"; \
	     $(as) $(asflags) $(call dynamic_flags,$(1),$(2)) -c $(2) -o $(1)
compile_ld = $(V)mkdir -p `dirname $(1)`; \
	     echo " LD        $(subst $(build_dir)/,,$(1))"; \
	     $(ld) $(3) $(ldflags) -Wl,-T$(2) -o $(1)
compile_ar = $(V)mkdir -p `dirname $(1)`; \
	     echo " AR        $(subst $(build_dir)/,,$(1))"; \
	     $(ar) $(arflags) $(1) $(2)
compile_objcopy = $(V)mkdir -p `dirname $(1)`; \
	     echo " OBJCOPY   $(subst $(build_dir)/,,$(1))"; \
	     $(objcopy) -S -O binary $(2) $(1)

targets-y  = $(build_dir)/lib/libsbi.a
ifdef PLATFORM
targets-y += $(build_dir)/$(platform_subdir)/lib/libplatsbi.a
endif
targets-y += $(firmware-bins-path-y)

# Default rule "make" should always be first rule
.PHONY: all
all: $(targets-y)

# Preserve all intermediate files
.SECONDARY:

$(build_dir)/%.bin: $(build_dir)/%.elf
	$(call compile_objcopy,$@,$<)

$(build_dir)/%.elf: $(build_dir)/%.o $(build_dir)/%.elf.ld $(build_dir)/$(platform_subdir)/lib/libplatsbi.a
	$(call compile_ld,$@,$@.ld,$< $(build_dir)/$(platform_subdir)/lib/libplatsbi.a)

$(build_dir)/$(platform_subdir)/%.ld: $(src_dir)/%.ldS
	$(call compile_cpp,$@,$<)

$(build_dir)/lib/libsbi.a: $(lib-objs-path-y)
	$(call compile_ar,$@,$^)

$(build_dir)/$(platform_subdir)/lib/libplatsbi.a: $(lib-objs-path-y) $(platform-common-objs-path-y) $(platform-objs-path-y)
	$(call compile_ar,$@,$^)

$(build_dir)/%.dep: $(src_dir)/%.c
	$(call compile_cc_dep,$@,$<)

$(build_dir)/%.o: $(src_dir)/%.c
	$(call compile_cc,$@,$<)

$(build_dir)/%.dep: $(src_dir)/%.S
	$(call compile_as_dep,$@,$<)

$(build_dir)/%.o: $(src_dir)/%.S
	$(call compile_as,$@,$<)

$(build_dir)/$(platform_subdir)/%.dep: $(src_dir)/%.c
	$(call compile_cc_dep,$@,$<)

$(build_dir)/$(platform_subdir)/%.o: $(src_dir)/%.c
	$(call compile_cc,$@,$<)

$(build_dir)/$(platform_subdir)/%.dep: $(src_dir)/%.S
	$(call compile_as_dep,$@,$<)

$(build_dir)/$(platform_subdir)/%.o: $(src_dir)/%.S
	$(call compile_as,$@,$<)

# Dependency files should only be included after default Makefile rules
# They should not be included for any "xxxconfig" or "xxxclean" rule
all-deps-1 = $(if $(findstring config,$(MAKECMDGOALS)),,$(deps-y))
all-deps-2 = $(if $(findstring clean,$(MAKECMDGOALS)),,$(all-deps-1))
-include $(all-deps-2)

# Include external dependency of firmwares after default Makefile rules
include $(src_dir)/firmware/external_deps.mk

install_targets-y  = install_libsbi
ifdef PLATFORM
install_targets-y += install_libplatsbi
install_targets-y += install_firmwares
endif

# Rule for "make install"
.PHONY: install
install: $(install_targets-y)

.PHONY: install_libsbi
install_libsbi: $(build_dir)/lib/libsbi.a
	$(call inst_header_dir,$(install_dir)/include,$(include_dir)/sbi)
	$(call inst_file,$(install_dir)/lib/libsbi.a,$(build_dir)/lib/libsbi.a)

.PHONY: install_libplatsbi
install_libplatsbi: $(build_dir)/$(platform_subdir)/lib/libplatsbi.a $(build_dir)/lib/libsbi.a
	$(call inst_header_dir,$(install_dir)/$(platform_subdir)/include,$(include_dir)/sbi)
	$(call inst_file,$(install_dir)/$(platform_subdir)/lib/libplatsbi.a,$(build_dir)/$(platform_subdir)/lib/libplatsbi.a)

.PHONY: install_firmwares
install_firmwares: $(build_dir)/$(platform_subdir)/lib/libplatsbi.a $(build_dir)/lib/libsbi.a $(firmware-bins-path-y)
	$(call inst_file_list,$(install_dir)/$(platform_subdir)/firmware,$(platform_subdir)/firmware,$(firmware-elfs-path-y))
	$(call inst_file_list,$(install_dir)/$(platform_subdir)/firmware,$(platform_subdir)/firmware,$(firmware-bins-path-y))

# Rule for "make clean"
.PHONY: clean
clean:
ifeq ($(build_dir),$(CURDIR)/build)
	$(V)mkdir -p $(build_dir)
	$(if $(V), @echo " CLEAN     $(build_dir)")
	$(V)find $(build_dir) -regex ".*\.\(o\|a\|elf\|bin\)" -type f -exec rm -rf {} +
endif

# Rule for "make distclean"
.PHONY: distclean
distclean:
ifeq ($(build_dir),$(CURDIR)/build)
	$(if $(V), @echo " RM        $(build_dir)")
	$(V)rm -rf $(build_dir)
endif
ifeq ($(install_dir),$(CURDIR)/install)
	$(if $(V), @echo " RM        $(install_dir)")
	$(V)rm -rf $(install_dir)
endif
