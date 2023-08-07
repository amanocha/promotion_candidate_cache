###############################################################################
#
# Copyright 1996-2017 Standard Performance Evaluation Corporation
#
# This makefile should NOT be changed.
# $Id: Makefile.defaults 5687 2017-05-08 14:47:07Z CloyceS $
#
###############################################################################
#	BENCHMARK	-- standard definitions for this benchmark

# Set some "safe" defaults
LIBS=
TUNE=peak
EXT=.none

###############################################################################
#
# Clear variables to avoid "accidentally" inheriting them from the environment.
#
# This is done because there's no way to make make ignore the environment; this
# is reasonable, and what we do for the SPEC benchmarks is in many cases
# counter to common practice of letting environment variables control
# compilation.
#
# And if this list looks big and stupid to you, that's because it is.  I can
# find NO WAY that make will let me iterate over values.  Any use of $(call)
# or even $(foreach x,x,var assignment) results in a "commands commence before
# first target.  Stop." error message.  Thanks a lot, GNU make.
BENCH_FLAGS=
CPUFLAGS=
EXTRA_LDFLAGS=
EXTRA_LIBS=
LD=
LDFLAGS=
LDOPTIONS=
LDPORTABILITY=
LIBS=
OPTIMIZE=
LDOPTIMIZE=
EXTRA_OPTIMIZE=
OS_LIBS=
PORTABILITY=
EXTRA_PORTABILITY=
PORTABILITY_LIBS=
MATH_LIBS=
OPTIMIZATION_LIBS=
RM_SOURCES=
# C
EXTRA_CFLAGS=
EXTRA_CLIBS=
PORTABILITY_CLIBS=
CLD=
LDCFLAGS=
COPTIMIZE=
EXTRA_COPTIMIZE=
CPORTABILITY=
EXTRA_CPORTABILITY=
BENCH_CFLAGS=
OPTIMIZATION_CLIBS=
# C++
EXTRA_CXXFLAGS=
EXTRA_CXXLIBS=
PORTABILITY_CXXLIBS=
CXXLD=
LDCXXFLAGS=
CXXOPTIMIZE=
EXTRA_CXXOPTIMIZE=
CXXPORTABILITY=
EXTRA_CXXPORTABILITY=
BENCH_CXXFLAGS=
OPTIMIZATION_CXXLIBS=
# Fortran 90/95
EXTRA_FFLAGS=
EXTRA_FLIBS=
PORTABILITY_FLIBS=
FLD=
LDFFLAGS=
FOPTIMIZE=
EXTRA_FOPTIMIZE=
FPORTABILITY=
EXTRA_FPORTABILITY=
BENCH_FFLAGS=
OPTIMIZATION_FLIBS=
# Fortran preprocessor
EXTRA_FPPFLAGS=
FPPPORTABILITY=
FPPFLAGS=
BENCH_FPPFLAGS=

ifeq (PASS,$(findstring PASS,$(FDO)))
  # General
  $(FDO)_FLAGS=
  $(FDO)_LDFLAGS=
  $(FDO)_OPTIMIZE=
  $(FDO)_LDOPTIONS=
  # C
  $(FDO)_CFLAGS=
  $(FDO)_COPTIMIZE=
  $(FDO)_LDCFLAGS=
  # C++
  $(FDO)_CXXFLAGS=
  $(FDO)_CXXOPTIMIZE=
  $(FDO)_LDCXXFLAGS=
  # Fortran 90/95
  $(FDO)_FFLAGS=
  $(FDO)_FPPFLAGS=
  $(FDO)_FOPTIMIZE=
  $(FDO)_LDFFLAGS=
endif

###############################################################################
#	DEFINES		-- standard definitions for SPEC
#                          some of these may be overriden in Makefile.spec
BENCHMARK     = $(NUMBER).$(NAME)
TMP_SOURCES   = $(filter-out $(RM_SOURCES),$(SOURCES)) $(EXTRA_SOURCES)
TMP1_SOURCES  = $(TMP_SOURCES:.F95=.fppized.f95)
TMP2_SOURCES  = $(TMP1_SOURCES:.F90=.fppized.f90)
TMP3_SOURCES  = $(TMP2_SOURCES:.F77=.fppized.f77)
FINAL_SOURCES = $(TMP3_SOURCES:.F=.fppized.f)
PP_SOURCES    = $(addsuffix .i,$(basename $(filter-out %.f95 %.f90 %.f77 %.f,$(FINAL_SOURCES)))) $(filter %.fppized.f95 %.fppized.f90 %.fppized.f77 %.fppized.f,$(FINAL_SOURCES))
OBJS          = $(addsuffix $(OBJ),$(basename $(FINAL_SOURCES)))
FPP_USED      = $(findstring fppized,$(OBJS))

###############################################################################
#	VENDOR-- where we include any vendor/platform specific information, via Makefile.spec

empty:=
space:= $(empty) $(empty)
ifeq ($(TARGET),)
   lookfor=Makefile.spec
else
   lookfor=Makefile.$(TARGET).spec
endif
ifeq ($(MAKECMDGOALS),)
   goals=
else
   goals=$(space)$(MAKECMDGOALS)
endif
ifeq ($(wildcard $(lookfor)),)
   $(info $(space))
   maybe=$(wildcard Makefile*spec)
   ifeq ($(maybe),)
      $(info Cannot find $(lookfor).  You might try:)
      $(info $(space)   runspec --action buildsetup)
   else
      $(info $(lookfor) does not exist.) 
      ifeq ($(maybe), Makefile.spec)
         $(info Maybe you should try:)
         $(info $(space)    $(MAKE)$(goals))
      else
         $(info Maybe you should try one or more of:)
         $(foreach target, $(maybe), $(info $(space)    $(patsubst Makefile.%.spec,$(MAKE)$(goals) TARGET=%,$(target))))
      endif
   endif
   $(info $(space))
   $(error Cannot find makefile)
endif

ifneq ($(TARGET),)
   include Makefile.$(TARGET).spec
   OBJNAMES = objects_$(TARGET)
else
   include Makefile.spec
   OBJNAMES = objects
endif

PRIMARY_BENCHLANG := $(firstword $(BENCHLANG))

CPUFLAGS       = -DSPEC -DNDEBUG
ifeq ($(OS),windows_nt)
    OBJ           ?= .obj
    OBJOPT        ?= -c -Fo$@
    LDOUT         ?= -Fe$@
    MATHLIBOPT    ?=
    # specsh gets upset when it can't glob things, so for RMRF (used in
    # clean targets), call cmd.exe directly instead
    RMRF          ?= specsh -o NOGLOB -c "cmd /C del /q /f /s #"
    CPUFLAGS      += -DSPEC_WINDOWS
    _defines_filter = -D% /D% -U% /U%
else
    _defines_filter = -D% -U%
    OBJ           ?= .o
    OBJOPT        ?= -c -o $@
    LDOUT         ?= -o $@
    MATHLIBOPT    ?= -lm
    RMRF          ?= rm -rf
endif
PPFLAGS           ?= -C -E
PPCFLAGS          ?= $(PPFLAGS)
PPCXXFLAGS        ?= $(PPFLAGS)

ifneq ($(origin CC), file)
CC         = cc
endif
COBJOPT   ?= $(OBJOPT)

ifneq ($(origin CXX), file)
CXX	   = CC
endif
CXXOBJOPT ?= $(OBJOPT)

ifneq ($(origin FC), file)
FC         = f90
endif
FOBJOPT   ?= $(OBJOPT)

# Here's the part that adds -lm (for non-Windows systems) to the link command,
# but it only does it if the primary language of the benchmark is _NOT_ C++.
ifneq ($(strip $(NEED_MATH)),)
  ifneq (CXX,$(PRIMARY_BENCHLANG))
    MATH_LIBS       = $(MATHLIBOPT)
  endif
endif

###############################################################################
#      LANGUAGE and FLAGS SELECTION -- choose flags for specific language
#
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# If you update any variables below, be SURE to update the output and the
# comments in the options sections!  Otherwise the flag reporting will not
# get the contents of all variables to examine.
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#

# FC is used for all Fortran builds, so substitute F for F77 in BENCHLANG
tmpBENCHLANG := $(subst F77,F,$(BENCHLANG))
BENCHLANG=$(tmpBENCHLANG)
PRIMARY_BENCHLANG := $(firstword $(BENCHLANG))
CXXC = $(CXX)
CXXC_VERSION_OPTION = $(CXX_VERSION_OPTION)

LD   = $($(PRIMARY_BENCHLANG)C)
ifneq (,$($(PRIMARY_BENCHLANG)LD))
    LD = $($(PRIMARY_BENCHLANG)LD)
endif

# Set up FDO-related flags
ifeq (PASS,$(findstring PASS,$(FDO)))
  FDO_OPTIMIZE    =$($(FDO)_OPTIMIZE)
  FDO_COPTIMIZE   =$($(FDO)_COPTIMIZE)
  FDO_CXXOPTIMIZE =$($(FDO)_CXXOPTIMIZE)
  FDO_FOPTIMIZE   =$($(FDO)_FOPTIMIZE)
  FDO_FPPFLAGS    =$($(FDO)_FPPFLAGS)
  FDO_LDOPTIONS   =$($(FDO)_LDOPTIONS)

  FDO_LDFLAGS     =                $($(FDO)_LDFLAGS)
  FDO_LDCFLAGS    =$($(FDO)_FLAGS) $($(FDO)_LDCFLAGS)
  FDO_LDCXXFLAGS  =$($(FDO)_FLAGS) $($(FDO)_LDCXXFLAGS)
  FDO_LDFFLAGS    =$($(FDO)_FLAGS) $($(FDO)_LDFFLAGS)
else
  # If FDO isn't being used, then these variables should not be usable:
  FDO_OPTIMIZE    =
  FDO_COPTIMIZE   =
  FDO_CXXOPTIMIZE =
  FDO_FOPTIMIZE   =
  FDO_FPPFLAGS    =
  FDO_LDOPTIONS   =

  FDO_LDFLAGS     =
  FDO_LDCFLAGS    =
  FDO_LDCXXFLAGS  =
  FDO_LDFFLAGS    =
endif

# This section may not be compressed into one BENCHLANG-specific assignment
# because there may be files not in PRIMARY_BENCHLANG that also need to
# be built.

# First, the "flags"
FINAL_CFLAGS   =$(CPUFLAGS) $(BENCH_FLAGS) $(BENCH_CFLAGS)
FINAL_CXXFLAGS =$(CPUFLAGS) $(BENCH_FLAGS) $(BENCH_CXXFLAGS)
RAW_FFLAGS     =            $(BENCH_FLAGS) $(BENCH_FFLAGS)

# Add PASSn_<lang>FLAGS if FDO is going on
ifeq (PASS,$(findstring PASS,$(FDO)))
  FINAL_CFLAGS   +=$($(FDO)_FLAGS) $($(FDO)_CFLAGS)
  FINAL_CXXFLAGS +=$($(FDO)_FLAGS) $($(FDO)_CXXFLAGS)
  RAW_FFLAGS     +=$($(FDO)_FLAGS) $($(FDO)_FFLAGS)
endif

# Next, optimization and portability flags
FINAL_CFLAGS   += $(OPTIMIZE) $(COPTIMIZE)   $(FDO_OPTIMIZE) $(FDO_COPTIMIZE)   $(PORTABILITY) $(CPORTABILITY)
FINAL_CXXFLAGS += $(OPTIMIZE) $(CXXOPTIMIZE) $(FDO_OPTIMIZE) $(FDO_CXXOPTIMIZE) $(PORTABILITY) $(CXXPORTABILITY)
RAW_FFLAGS     += $(OPTIMIZE) $(FOPTIMIZE)   $(FDO_OPTIMIZE) $(FDO_FOPTIMIZE)   $(PORTABILITY) $(FPORTABILITY)

# Now add all of the "extra" flags on the end.
FINAL_CFLAGS   += $(EXTRA_CFLAGS)
FINAL_CXXFLAGS += $(EXTRA_CXXFLAGS)
RAW_FFLAGS     += $(EXTRA_FFLAGS)

# And then the "extra" optimization and portability flags
FINAL_CFLAGS   += $(EXTRA_OPTIMIZE) $(EXTRA_COPTIMIZE)   $(EXTRA_PORTABILITY) $(EXTRA_CPORTABILITY)
FINAL_CXXFLAGS += $(EXTRA_OPTIMIZE) $(EXTRA_CXXOPTIMIZE) $(EXTRA_PORTABILITY) $(EXTRA_CXXPORTABILITY)
RAW_FFLAGS     += $(EXTRA_OPTIMIZE) $(EXTRA_FOPTIMIZE)   $(EXTRA_PORTABILITY) $(EXTRA_FPORTABILITY)

# Filter out defines and undefines from Fortran flags
FINAL_FFLAGS = $(filter-out $(_defines_filter), $(RAW_FFLAGS))

FINAL_LIBS     =$(LIBS) $(OS_LIBS) $(MATH_LIBS) $(OPTIMIZATION_LIBS) $(OPTIMIZATION_$(PRIMARY_BENCHLANG)LIBS) $(PORTABILITY_LIBS) $(PORTABILITY_$(PRIMARY_BENCHLANG)LIBS) $(EXTRA_LIBS) $(EXTRA_$(PRIMARY_BENCHLANG)LIBS)

###############
# specpp flags; order is expected to conform to RAW_FFLAGS ordering
###############
FINAL_FPPFLAGS =$(strip \
		  $(FPPFLAGS) \
		  $(filter $(_defines_filter), $(CPUFLAGS) $(BENCH_FLAGS)) \
		  $(BENCH_FPPFLAGS) $(FDO_FPPFLAGS) \
		  $(filter $(_defines_filter), $(OPTIMIZE)          $(foreach lang,$(filter F%, $(BENCHLANG)),$($(lang)OPTIMIZE))) \
		  $(filter $(_defines_filter), $(FDO_OPTIMIZE)      $(foreach lang,$(filter F%, $(BENCHLANG)),$(FDO_$(lang)OPTIMIZE))) \
		  $(FPPOPTIMIZE) \
		  $(filter $(_defines_filter), $(PORTABILITY)       $(foreach lang,$(filter F%, $(BENCHLANG)),$($(lang)PORTABILITY))) \
		  $(FPPPORTABILITY) \
		  $(EXTRA_FPPFLAGS) \
		  $(filter $(_defines_filter), $(EXTRA_OPTIMIZE)    $(foreach lang,$(filter F%, $(BENCHLANG)),$(EXTRA_$(lang)OPTIMIZE))) \
		  $(EXTRA_FPPOPTIMIZE) \
		  $(filter $(_defines_filter), $(EXTRA_PORTABILITY) $(foreach lang,$(filter F%, $(BENCHLANG)),$(EXTRA_$(lang)PORTABILITY))) \
		  $(EXTRA_FPPPORTABILITY) \
		)


##############
# Linker flags
##############
# User-supplied linker options
FINAL_LDOPTIONS  =$(LDOPTIONS) $(FDO_LDOPTIONS)

# Used only for regular builds
LDOPTFLAGS  = $(OPTIMIZE) $($(PRIMARY_BENCHLANG)OPTIMIZE) $(FDO_OPTIMIZE) $(FDO_$(PRIMARY_BENCHLANG)OPTIMIZE) $(LDOPTIMIZE) $(PORTABILITY) $($(PRIMARY_BENCHLANG)PORTABILITY) $(LDPORTABILITY) $(EXTRA_OPTIMIZE) $(EXTRA_$(PRIMARY_BENCHLANG)OPTIMIZE)

###############################################################################
#	TARGETS		-- Rules to build benchmark executables

all: build

$(EXEBASE): $(OBJS)
ifndef NEEDATFILE
	$(LD) $(LDFLAGS) $(LD$(PRIMARY_BENCHLANG)FLAGS) $(EXTRA_LDFLAGS) $(FDO_LDFLAGS) $(FDO_LD$(PRIMARY_BENCHLANG)FLAGS) $(LDOPTFLAGS) $^           $(FINAL_LIBS) $(FINAL_LDOPTIONS) $(LDOUT) $(LDOUT_OPTIONS) $(LDOUT_EXTRA_OPTIONS)
else
	$(LD) $(LDFLAGS) $(LD$(PRIMARY_BENCHLANG)FLAGS) $(EXTRA_LDFLAGS) $(FDO_LDFLAGS) $(FDO_LD$(PRIMARY_BENCHLANG)FLAGS) $(LDOPTFLAGS) @$(OBJNAMES) $(FINAL_LIBS) $(FINAL_LDOPTIONS) $(LDOUT) $(LDOUT_OPTIONS) $(LDOUT_EXTRA_OPTIONS)
endif

preprocess: $(PP_SOURCES)

.SUFFIXES: $(OBJ) .i
# C
%$(OBJ): %.c
	$(CC) $(COBJOPT) $(FINAL_CFLAGS) $<
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif
%.i: %.c
	$(CC) $(PPCFLAGS) $(FINAL_CFLAGS) $< > $@

# C++
%$(OBJ): %.cc
	$(CXXC) $(CXXOBJOPT) $(FINAL_CXXFLAGS) $<
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif
%$(OBJ): %.C
	$(CXXC) $(CXXOBJOPT) $(FINAL_CXXFLAGS) $<
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif
%$(OBJ): %.cpp
	$(CXXC) $(CXXOBJOPT) $(FINAL_CXXFLAGS) $<
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif
%$(OBJ): %.cxx
	$(CXXC) $(CXXOBJOPT) $(FINAL_CXXFLAGS) $<
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif
%.i: %.cc
	$(CXXC) $(PPCXXFLAGS) $(FINAL_CXXFLAGS) $< > $@
%.i: %.C
	$(CXXC) $(PPCXXFLAGS) $(FINAL_CXXFLAGS) $< > $@
%.i: %.cpp
	$(CXXC) $(PPCXXFLAGS) $(FINAL_CXXFLAGS) $< > $@
%.i: %.cxx
	$(CXXC) $(PPCXXFLAGS) $(FINAL_CXXFLAGS) $< > $@

# Regular FORTRAN
%$(OBJ): %.f90
	$(FC) $(FOBJOPT) $(FINAL_FFLAGS) $<
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif
%$(OBJ): %.f
	$(FC) $(FOBJOPT) $(FINAL_FFLAGS) $<
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif

.PRECIOUS: %.fppized.f95 %.fppized.f90 %.fppized.f

# Pre-processed FORTRAN95
%.fppized.f95: %.F95
	$(SPEC)/bin/specperl $(SPEC)/bin/harness/specpp $(FINAL_FPPFLAGS) $< -o $(addsuffix .fppized.f95,$(basename $<))

%$(OBJ): %.fppized.f95
	$(FC) $(FOBJOPT) $(FINAL_FFLAGS) $<
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif

# Pre-processed FORTRAN90
%.fppized.f90: %.F90
	$(SPEC)/bin/specperl $(SPEC)/bin/harness/specpp $(FINAL_FPPFLAGS) $< -o $(addsuffix .fppized.f90,$(basename $<))

%$(OBJ): %.fppized.f90
	$(FC) $(FOBJOPT) $(FINAL_FFLAGS) $<
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif

# Pre-processed FORTRAN
%.fppized.f: %.F
	$(SPEC)/bin/specperl $(SPEC)/bin/harness/specpp $(FINAL_FPPFLAGS) $< -o $(addsuffix .fppized.f,$(basename $<))

%$(OBJ): %.fppized.f
	$(FC) $(FOBJOPT) $(FINAL_FFLAGS) $(addsuffix .fppized.f,$(basename $<))
ifdef NEEDATFILE
	$(file >>$(OBJNAMES),$@)
endif

.PHONY: fdoclean clean install options compiler-version
.SILENT: options compiler-version
.IGNORE: options compiler-version

# On Windows, del /s removes specified patterns in subdirectories; UNIX doesn't,
# and thus the find | xargs
objclean:
# Removal of object files for inter-build cleaning
ifeq ($(OS),windows_nt)
# The trailing quotes are intentional; they match the opening quote in the
# definition of RMRF
	-$(RMRF) *$(OBJ) $(EXTRA_RMFILES) $(OUTPUT_RMFILES)"
	-$(RMRF) *.i *.fppized.f* *.mod"
ifdef NEEDATFILE
	-$(RMRF) $(OBJNAMES)
# "
endif
else
	-$(RMRF) *$(OBJ) $(EXTRA_RMFILES) $(OUTPUT_RMFILES)
	-find . \( -name \*$(OBJ) -o -name '*.fppized.f*' -o -name '*.i' -o -name '*.mod' \) -print | xargs $(RMRF)
ifdef NEEDATFILE
	-$(RMRF) $(OBJNAMES)
endif
endif

fdoclean: objclean
ifeq ($(OS),windows_nt)
# The trailing quotes are intentional; they match the opening quote in the
# definition of RMRF
	-$(RMRF) $(EXEBASE)"
	-$(RMRF) $(EXEBASE).exe"
# "
else
	-$(RMRF) $(EXEBASE)
	-$(RMRF) $(EXEBASE).exe
endif

clean: fdoclean
ifeq ($(OS),windows_nt)
# The trailing quotes are intentional; they match the opening quote in the
# definition of RMRF
	-$(RMRF) core"
ifneq ($(filter-out make.clean.err make.clean.out make.objclean.err make.objclean.out build_pre_bench.err build_pre_bench.out,$(wildcard *.err) $(wildcard *.out)),)
	-$(RMRF) $(filter-out make.clean.err make.clean.out make.objclean.err make.objclean.out build_pre_bench.err build_pre_bench.out,$(wildcard *.err) $(wildcard *.out))"
endif
else
	-$(RMRF) core
ifneq ($(filter-out make.clean.err make.clean.out make.objclean.err make.objclean.out build_pre_bench.err build_pre_bench.out,$(wildcard *.err) $(wildcard *.out)),)
	-$(RMRF) $(filter-out make.clean.err make.clean.out make.objclean.err make.objclean.out build_pre_bench.err build_pre_bench.out,$(wildcard *.err) $(wildcard *.out))
endif
endif

build: $(EXEBASE)

ifeq ($(OS),windows_nt)
define dump_tmpout
	+cmd /c "type $@.tmpout"
	+$(RMRF) $@.tmpout" > NUL:
endef
else
define dump_tmpout
	+cat "$@.tmpout"
	+$(RMRF) $@.tmpout
endef
endif

define compiler_version_info
	$(file >>$@.tmpout,$(FDO) $(1)C_VERSION_OPTION:)
	$($(1)C) $(FINAL_$(1)FLAGS) $($(1)C_VERSION_OPTION) >>$@.tmpout 2>&1
	$(file >>$@.tmpout,)
endef

compiler-version:
	$(file >$@.tmpout,)
	$(foreach lang, $(BENCHLANG), $(if $($(lang)C_VERSION_OPTION),$(call compiler_version_info,$(lang)),$(file >>$@.tmpout,$(FDO) $(lang)C_VERSION_OPTION: version information option not set)))
	$(call dump_tmpout)

###############################################################################
#
# Print the options used.
#
# This is super-ugly, but it's the only portable, make-only way I could figure
# out to mostly have only the non-empty variables output.
#
# The blank lines at the end of each definition are important; please don't
# delete them.
define benchlang-comp-line
	$(file >>$@.tmpout,COMP: "$($(1)C) $($(1)OBJOPT)$(OBJ) $(FINAL_$(1)FLAGS) <source>")
	$(file >>$@.tmpout,C: $(1)C="$($(1)C)")
	$(file >>$@.tmpout,C: $(1)OBJOPT="$($(1)OBJOPT)")
	$(file >>$@.tmpout,P: CPUFLAGS="$(CPUFLAGS)")
	$(file >>$@.tmpout,P: BENCH_FLAGS="$(BENCH_FLAGS)")
	$(file >>$@.tmpout,P: BENCH_$(1)FLAGS="$(BENCH_$(1)FLAGS)")
	$(file >>$@.tmpout,O: OPTIMIZE="$(OPTIMIZE)")
	$(file >>$@.tmpout,O: $(1)OPTIMIZE="$($(1)OPTIMIZE)")
	$(file >>$@.tmpout,P: PORTABILITY="$(PORTABILITY)")
	$(file >>$@.tmpout,P: $(1)PORTABILITY="$($(1)PORTABILITY)")
	$(file >>$@.tmpout,O: EXTRA_$(1)FLAGS="$(EXTRA_$(1)FLAGS)")
	$(file >>$@.tmpout,O: EXTRA_OPTIMIZE="$(EXTRA_OPTIMIZE)")
	$(file >>$@.tmpout,O: EXTRA_$(1)OPTIMIZE="$(EXTRA_$(1)OPTIMIZE)")
	$(file >>$@.tmpout,P: EXTRA_PORTABILITY="$(EXTRA_PORTABILITY)")
	$(file >>$@.tmpout,P: EXTRA_$(1)PORTABILITY="$(EXTRA_$(1)PORTABILITY)")

endef
define benchlang-fdo-comp-line
	$(file >>$@.tmpout,COMP: "$($(1)C) $($(1)OBJOPT)$(OBJ) $(FINAL_$(1)FLAGS) <source>")
	$(file >>$@.tmpout,C: $(1)C="$($(1)C)")
	$(file >>$@.tmpout,C: $(1)OBJOPT="$($(1)OBJOPT)")
	$(file >>$@.tmpout,P: CPUFLAGS="$(CPUFLAGS)")
	$(file >>$@.tmpout,P: BENCH_FLAGS="$(BENCH_FLAGS)")
	$(file >>$@.tmpout,P: BENCH_$(1)FLAGS="$(BENCH_$(1)FLAGS)")
	$(file >>$@.tmpout,O: $(FDO)_FLAGS="$($(FDO)_FLAGS)")
	$(file >>$@.tmpout,O: $(FDO)_$(1)FLAGS="$($(FDO)_$(1)FLAGS)")
	$(file >>$@.tmpout,O: OPTIMIZE="$(OPTIMIZE)")
	$(file >>$@.tmpout,O: $(1)OPTIMIZE="$($(1)OPTIMIZE)")
	$(file >>$@.tmpout,O: $(FDO)_OPTIMIZE="$($(FDO)_OPTIMIZE)")
	$(file >>$@.tmpout,O: $(FDO)_$(1)OPTIMIZE="$($(FDO)_$(1)OPTIMIZE)")
	$(file >>$@.tmpout,P: PORTABILITY="$(PORTABILITY)")
	$(file >>$@.tmpout,P: $(1)PORTABILITY="$($(1)PORTABILITY)")
	$(file >>$@.tmpout,O: EXTRA_$(1)FLAGS="$(EXTRA_$(1)FLAGS)")
	$(file >>$@.tmpout,O: EXTRA_OPTIMIZE="$(EXTRA_OPTIMIZE)")
	$(file >>$@.tmpout,O: EXTRA_$(1)OPTIMIZE="$(EXTRA_$(1)OPTIMIZE)")
	$(file >>$@.tmpout,P: EXTRA_PORTABILITY="$(EXTRA_PORTABILITY)")
	$(file >>$@.tmpout,P: EXTRA_$(1)PORTABILITY="$(EXTRA_$(1)PORTABILITY)")

endef

define defines-only-no-extra
	$(file >>$@.tmpout,$(4): $(3)$(2)$(1)="$(strip $(filter $(_defines_filter), $($(3)$(2)$(1))))")

endef

define defines-only
	$(call defines-only-no-extra,$(1),$(2),$(3),$(4),$(5))
	$(file >>$@.tmpout,$(4): EXTRA_$(2)$(1)="$(strip $(filter $(_defines_filter), $(EXTRA_$(2)$(1))))")

endef

options:
	$(file >$@.tmpout,)

#
# specpp options output (only if specpp was used)
#
ifeq (fppized,$(FPP_USED))
	$(file >>$@.tmpout,FPP: "specpp $(FINAL_FPPFLAGS) <source> -o <source>.fppized.f9x")
ifneq (,$(strip $(FPPFLAGS)))
	$(file >>$@.tmpout,O: FPPFLAGS="$(strip $(FPPFLAGS))")
endif
ifneq (,$(strip $(filter $(_defines_filter), $(CPUFLAGS))))
	$(file >>$@.tmpout,P: CPUFLAGS="$(strip $(filter $(_defines_filter), $(CPUFLAGS)))")
endif
ifneq (,$(strip $(filter $(_defines_filter), $(BENCH_FLAGS))))
	$(file >>$@.tmpout,P: BENCH_FLAGS="$(strip $(filter $(_defines_filter), $(BENCH_FLAGS)))")
endif
ifneq (,$(strip $(BENCH_FPPFLAGS)))
	$(file >>$@.tmpout,P: BENCH_FPPFLAGS="$(strip $(BENCH_FPPFLAGS))")
endif
ifeq (PASS,$(findstring PASS,$(FDO)))
  ifneq (,$(strip $($(FDO)_FPPFLAGS)))
	$(file >>$@.tmpout,O: $(FDO)_FPPFLAGS="$(strip $($(FDO)_FPPFLAGS))")
  endif
endif
	$(call defines-only,OPTIMIZE,,,O)
	$(foreach lang, FPP $(filter F%, $(BENCHLANG)),$(call defines-only,OPTIMIZE,$(lang),,O))
ifeq (PASS,$(findstring PASS,$(FDO)))
  ifneq (,$(strip $(filter $(_defines_filter), $($(FDO)_OPTIMIZE))))
	$(file >>$@.tmpout,O: $(FDO)_OPTIMIZE="$(strip $(filter $(_defines_filter), $($(FDO)_OPTIMIZE)))")
  endif
	$(foreach lang, $(filter F%, $(BENCHLANG)),$(call defines-only-no-extra,OPTIMIZE,$(lang),$(FDO)_,O))
endif
	$(call defines-only,PORTABILITY,,,P)
	$(foreach lang, $(filter F%, $(BENCHLANG)),$(call defines-only,PORTABILITY,$(lang),,P))
ifneq (,$(strip $(FPPPORTABILITY)))
	$(file >>$@.tmpout,P: FPPPORTABILITY="$(strip $(FPPPORTABILITY))")
endif
ifneq (,$(strip $($(EXTRA_FPPFLAGS))))
	$(file >>$@.tmpout,O: EXTRA_FPPFLAGS="$(strip $(EXTRA_FPPFLAGS))")
endif
ifneq (,$(strip $(EXTRA_FPPPORTABILITY)))
	$(file >>$@.tmpout,P: EXTRA_FPPPORTABILITY="$(strip $(EXTRA_FPPPORTABILITY))")
endif
endif

# COMP flags breakdown:
#  <lang>C
#  <lang>OBJOPT
#  FINAL_<lang>FLAGS
#    CPUFLAGS (for C/C++ only)
#    BENCH_FLAGS
#    BENCH_<lang>FLAGS
#    PASS<n>_FLAGS (if FDO is used)
#    PASS<n>_<lang>FLAGS (if FDO is used)
#    OPTIMIZE
#    <lang>OPTIMIZE
#    PASS<n>_OPTIMIZE (if FDO is used)
#    PASS<n>_<lang>OPTIMIZE (if FDO is used)
#    PORTABILITY
#    <lang>PORTABILITY
#    EXTRA_<lang>FLAGS
#    EXTRA_OPTIMIZE
#    EXTRA_<lang>OPTIMIZE
#    EXTRA_PORTABILITY
#    EXTRA_<lang>PORTABILITY
ifeq (PASS,$(findstring PASS,$(FDO)))
	$(foreach lang,$(BENCHLANG),$(call benchlang-fdo-comp-line,$(lang)))
else
	$(foreach lang,$(BENCHLANG),$(call benchlang-comp-line,$(lang)))
endif

	$(file >>$@.tmpout,LINK: "$(LD) $(LDFLAGS) $(LD$(PRIMARY_BENCHLANG)FLAGS) $(EXTRA_LDFLAGS) $(FDO_LDFLAGS) $(FDO_LD$(PRIMARY_BENCHLANG)FLAGS) $(LDOPTFLAGS) <objects> $(FINAL_LIBS) $(FINAL_LDOPTIONS) $(LDOUT) $(LDOUT_OPTIONS) $(LDOUT_EXTRA_OPTIONS)")
# LINK flags breakdown:
#  LD
#  LDFLAGS
#  LD<lang>FLAGS
#  EXTRA_LDFLAGS
#  FDO_LDFLAGS
#    PASS<n>_LDFLAGS (if FDO is used)
#  FDO_LD<lang>FLAGS
#    PASS<n>_LD<lang>FLAGS (if FDO is used)
#  LDOPTFLAGS
#    OPTIMIZE
#    <lang>OPTIMIZE
#    FDO_OPTIMIZE
#      PASS<n>_OPTIMIZE (if FDO is used)
#    FDO_<lang>OPTIMIZE
#      PASS<n>_<lang>OPTIMIZE (if FDO is used)
#    LDOPTIMIZE
#    PORTABILITY
#    <lang>PORTABILITY
#    LDPORTABILITY
#    EXTRA_OPTIMIZE
#    EXTRA_<lang>OPTIMIZE
#  FINAL_LIBS
#    LIBS
#    OS_LIBS
#    MATH_LIBS
#    OPTIMIZATION_LIBS
#    OPTIMIZATION_<lang>LIBS
#    PORTABILITY_LIBS
#    PORTABILITY_<lang>LIBS
#    EXTRA_LIBS
#    EXTRA_<lang>LIBS
#  FINAL_LDOPTIONS
#    LDOPTIONS
#    FDO_LDOPTIONS
#      PASS<n>_LDOPTIONS (if FDO is used)
#  LDOUT
#  LDOUT_OPTIONS
#  LDOUT_EXTRA_OPTIONS

# LD:
ifneq (,$(strip $(LD)))
	$(file >>$@.tmpout,C: LD="$(LD)")
endif
# LDFLAGS:
ifneq (,$(strip $(LDFLAGS)))
	$(file >>$@.tmpout,O: LDFLAGS="$(LDFLAGS)")
endif
# LD<lang>FLAGS:
ifneq (,$(strip $(LD$(PRIMARY_BENCHLANG)FLAGS)))
	$(file >>$@.tmpout,O: LD$(PRIMARY_BENCHLANG)FLAGS="$(LD$(PRIMARY_BENCHLANG)FLAGS)")
endif
# EXTRA_LDFLAGS:
ifneq (,$(strip $(EXTRA_LDFLAGS)))
	$(file >>$@.tmpout,O: EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)")
endif
ifeq (PASS,$(findstring PASS,$(FDO)))
# FDO_LDFLAGS:
  ifneq (,$(strip $($(FDO)_LDFLAGS)))
	$(file >>$@.tmpout,O: $(FDO)_LDFLAGS="$($(FDO)_LDFLAGS)")
  endif
# FDO_LD<lang>FLAGS:
  ifneq (,$(strip $($(FDO)_LD$(PRIMARY_BENCHLANG)FLAGS)))
	$(file >>$@.tmpout,O: $(FDO)_LD$(PRIMARY_BENCHLANG)FLAGS="$($(FDO)_FLAGS) $($(FDO)_LD$(PRIMARY_BENCHLANG)FLAGS)")
  endif
endif
# LDOPTFLAGS:
ifneq (,$(strip $(OPTIMIZE)))
	$(file >>$@.tmpout,O: OPTIMIZE="$(OPTIMIZE)")
endif
ifneq (,$(strip $($(PRIMARY_BENCHLANG)OPTIMIZE)))
	$(file >>$@.tmpout,O: $(PRIMARY_BENCHLANG)OPTIMIZE="$($(PRIMARY_BENCHLANG)OPTIMIZE)")
endif
ifeq (PASS,$(findstring PASS,$(FDO)))
  ifneq (,$(strip $($(FDO)_OPTIMIZE)))
	$(file >>$@.tmpout,O: $(FDO)_OPTIMIZE="$($(FDO)_OPTIMIZE)")
  endif
  ifneq (,$(strip $($(FDO)_$(PRIMARY_BENCHLANG)OPTIMIZE)))
	$(file >>$@.tmpout,O: $(FDO)_OPTIMIZE="$($(FDO)_OPTIMIZE)")
  endif
endif
ifneq (,$(strip $(LDOPTIMIZE)))
	$(file >>$@.tmpout,O: LDOPTIMIZE="$(LDOPTIMIZE)")
endif
ifneq (,$(strip $(PORTABILITY)))
	$(file >>$@.tmpout,P: PORTABILITY="$(PORTABILITY)")
endif
ifneq (,$(strip $($(PRIMARY_BENCHLANG)PORTABILITY)))
	$(file >>$@.tmpout,P: $(PRIMARY_BENCHLANG)PORTABILITY="$($(PRIMARY_BENCHLANG)PORTABILITY)")
endif
ifneq (,$(strip $(LDPORTABILITY)))
	$(file >>$@.tmpout,P: LDPORTABILITY="$(LDPORTABILITY)")
endif
ifneq (,$(strip $(EXTRA_OPTIMIZE)))
	$(file >>$@.tmpout,O: EXTRA_OPTIMIZE="$(EXTRA_OPTIMIZE)")
endif
ifneq (,$(strip $(EXTRA_$(PRIMARY_BENCHLANG)OPTIMIZE)))
	$(file >>$@.tmpout,O: EXTRA_$(PRIMARY_BENCHLANG)OPTIMIZE="$(EXTRA_$(PRIMARY_BENCHLANG)OPTIMIZE)")
endif
# FINAL_LIBS:
ifneq (,$(strip $(LIBS)))
	$(file >>$@.tmpout,O: LIBS="$(LIBS)")
endif
ifneq (,$(strip $(OS_LIBS)))
	$(file >>$@.tmpout,O: OS_LIBS="$(OS_LIBS)")
endif
ifneq (,$(strip $(MATH_LIBS)))
	$(file >>$@.tmpout,C: MATH_LIBS="$(MATH_LIBS)")
endif
ifneq (,$(strip $(OPTIMIZATION_LIBS)))
	$(file >>$@.tmpout,O: OPTIMIZATION_LIBS="$(OPTIMIZATION_LIBS)")
endif
ifneq (,$(strip $(OPTIMIZATION_$(PRIMARY_BENCHLANG)LIBS)))
	$(file >>$@.tmpout,O: OPTIMIZATION_$(PRIMARY_BENCHLANG)LIBS="$(OPTIMIZATION_$(PRIMARY_BENCHLANG)LIBS)")
endif
ifneq (,$(strip $(PORTABILITY_LIBS)))
	$(file >>$@.tmpout,P: PORTABILITY_LIBS="$(PORTABILITY_LIBS)")
endif
ifneq (,$(strip $(PORTABILITY_$(PRIMARY_BENCHLANG)LIBS)))
	$(file >>$@.tmpout,P: PORTABILITY_$(PRIMARY_BENCHLANG)LIBS="$(PORTABILITY_$(PRIMARY_BENCHLANG)LIBS)")
endif
ifneq (,$(strip $(EXTRA_LIBS)))
	$(file >>$@.tmpout,O: EXTRA_LIBS="$(EXTRA_LIBS)")
endif
ifneq (,$(strip $(EXTRA_$(PRIMARY_BENCHLANG)LIBS)))
	$(file >>$@.tmpout,O: EXTRA_$(PRIMARY_BENCHLANG)LIBS="$(EXTRA_$(PRIMARY_BENCHLANG)LIBS)")
endif
# FINAL_LDOPTIONS:
ifneq (,$(strip $(LDOPTIONS)))
	$(file >>$@.tmpout,C: LDOPTIONS="$(LDOPTIONS)")
endif
ifeq (PASS,$(findstring PASS,$(FDO)))
ifneq (,$(strip $($(FDO)_LDOPTIONS)))
	$(file >>$@.tmpout,C: $(FDO)_LDOPTIONS="$($(FDO)_LDOPTIONS)")
endif
endif
# LDOUT:
ifneq (,$(strip $(LDOUT)))
	$(file >>$@.tmpout,C: LDOUT="$(LDOUT)")
endif
# LDOUT_OPTIONS:
ifneq (,$(strip $(LDOUT_OPTIONS)))
	$(file >>$@.tmpout,C: LDOUT_OPTIONS="$(LDOUT_OPTIONS)")
endif
# LDOUT_EXTRA_OPTIONS:
ifneq (,$(strip $(LDOUT_EXTRA_OPTIONS)))
	$(file >>$@.tmpout,C: LDOUT_EXTRA_OPTIONS="$(LDOUT_EXTRA_OPTIONS)")
endif
	$(call dump_tmpout)

ifneq ($(TARGET),)
include Makefile.$(TARGET).deps
else
include Makefile.deps
endif

# Editor settings: (please leave this at the end of the file)
# vim: set filetype=make syntax=make shiftwidth=8 tabstop=8 softtabstop=0 noexpandtab nosmarttab list:
