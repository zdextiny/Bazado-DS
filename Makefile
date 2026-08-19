#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES		:=	gfx source data
INCLUDES	:=	include build
MUSIC		:=	maxmod_data
# Musica de fondo (PCM crudo, streameada -- ver setup_music_stream en
# main.c): va aparte del soundbank de maxmod porque es demasiado
# grande para cargarla entera en RAM como un efecto mas (~9MB contra
# los ~4MB de RAM de la DS). NitroFS mete el archivo DENTRO de la ROM
# (a diferencia de FAT/libfat, no depende de que haya una tarjeta SD
# real detras -- funciona igual en melonDS, flashcart o TWiLight
# Menu++) y se lee de a pedacitos en tiempo real, nunca entera.
NITRODATA	:=	nitrofiles

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv5te -mtune=arm946e-s -mthumb

CFLAGS	:=	-g -Wall -O2 -ffunction-sections -fdata-sections\
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= -lm -lmm9 -lfat -lnds9


#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export TOPDIR	:=	$(CURDIR)

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

ifneq ($(strip $(NITRODATA)),)
	export NITRO_FILES	:=	$(CURDIR)/$(NITRODATA)
endif

#---------------------------------------------------------------------------------
# Banner (titulo/icono que se ve en el menu de la DS, TWiLight Menu++,
# etc.) -- ANTES no estaba definido, asi que ndstool armaba el banner
# con un icono vacio. Probablemente la causa real de "An error has
# occurred" al abrirlo por nds-bootstrap en la 3DS (lee el banner para
# su lista de juegos; un banner invalido lo puede hacer fallar) -- en
# melonDS nunca se hubiera notado porque no lo necesita para nada.
# OJO: esto tiene que ir ACA (adentro del ifneq, con export), no mas
# arriba -- este Makefile se re-parsea una segunda vez de forma
# recursiva con -C build (ver mas abajo), y ahi CURDIR ya no es la raiz
# del proyecto sino build/. Fuera de este bloque, $(CURDIR)/gfx/icon.bmp
# se recalculaba mal esa segunda vez (build/gfx/icon.bmp, que no existe).
#---------------------------------------------------------------------------------
# OJO: dejar GAME_SUBTITLE1/2 vacios NO los deja en blanco -- ds_rules
# les pone "built with devkitARM"/"http://devkitpro.org" de relleno si
# estan vacios (ver el ifeq ahi). Las 3 lineas tienen que decir algo
# DISTINTO cada una -- repetir "Bazado" en las 3 (lo de antes) quedaba
# raro en el banner del menu de la 3DS/TWiLight Menu++.
export GAME_TITLE		:=	Bazado
export GAME_SUBTITLE1	:=	Por Agustin Cavalie/Z-Dextiny
export GAME_SUBTITLE2	:=	Dextiny Productions
export GAME_ICON		:=	$(CURDIR)/gfx/icon.bmp

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.bin))) soundbank.bin

# Sonido (maxmod): mmutil junta todos los .wav de maxmod_data/ en un solo
# soundbank.bin + soundbank.h (un ID por archivo, ej. SFX_PLAY para play.wav).
export AUDIOFILES	:=	$(foreach dir,$(notdir $(wildcard $(MUSIC)/*.*)),$(CURDIR)/$(MUSIC)/$(dir))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).nds	: 	$(OUTPUT).elf
ifneq ($(strip $(NITRODATA)),)
$(OUTPUT).nds	: 	$(shell find $(TOPDIR)/$(NITRODATA) -type f 2>/dev/null)
endif
$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# bin2o deja el objeto como NOMBRE.bin.o (no NOMBRE.o -- le agrega el
# sufijo .o a lo que ya tenia, no lo reemplaza), asi que el patron tiene
# que pedir exactamente eso, si no make nunca encuentra el archivo que
# se genero de verdad y lo re-arma en cada build.
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
# arma el soundbank a partir de todos los .wav de maxmod_data/
#---------------------------------------------------------------------------------
soundbank.bin soundbank.h : $(AUDIOFILES)
#---------------------------------------------------------------------------------
	@mmutil $^ -d -osoundbank.bin -hsoundbank.h


-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
