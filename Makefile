CC	= @g++.exe
RC	= @windres.exe

##############################################################################
# std::filesystemが使える場合は17，使えなければ14を指定
##############################################################################
CPPVER  = 17

##############################################################################
# スレッド関連選択
# 0 : std::thread使用
# 1 : SDLのスレッド関数を使用
# 2 : Win32APIのスレッド関数を使用
##############################################################################
THREAD	= 0



TARGET	= pc6001v.exe

OBJC	= breakpoint common config console cpum cpus d88 debug device disk error graph ini intr		\
	  io joystick keyboard memory memblk movie p6t2 p6el p6vm pc6001v pio psgfm replay schedule	\
	  sound status tape voice vdg vsurface romaji
OBJDEV	= ay8910 mc6847 pd7752 pd8255 ym2203 z80 z80-dbg fmgen/fmgen fmgen/psg fmgen/opna fmgen/fmtimer
OBJSDL	= osdSDL
OBJOSD	= guiWin32 osdWin32
OBJTRD	= csemaphore cthread
OBJFSY	= filesystem
OBJRES	= p6v

DIRTRG	= $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
DIRSRC	= $(DIRTRG)src
DIROBJ	= $(DIRTRG)o
DIRDEV	= device
DIRSDL	= SDL
DIROSD	= Win32
DIRRES	= Win32

PKGCNFG = sdl2 libpng libavcodec libavformat libswscale libswresample

#-----------------------------------------------------------------------------
ifeq ($(CPPVER), 17)
OBJC	+= $(OBJFSY)
else
OBJOSD	+= $(OBJFSY)
endif

#-----------------------------------------------------------------------------
ifeq ($(THREAD), 1)
OBJSDL	+= $(OBJTRD)
else ifeq ($(THREAD), 2)
OBJOSD	+= $(OBJTRD)
else
OBJC	+= $(OBJTRD)
endif



DIRSRCS	= $(DIRSRC) $(addprefix $(DIRSRC)/, $(DIRDEV) $(DIRSDL) $(DIROSD))

OBJS	= $(OBJC)				\
	  $(addprefix $(DIRDEV)/, $(OBJDEV))	\
	  $(addprefix $(DIRSDL)/, $(OBJSDL))	\
	  $(addprefix $(DIROSD)/, $(OBJOSD))
OBJALL	= $(addprefix $(DIROBJ)/, $(addsuffix .o,   $(OBJS)))

RESS	= $(addprefix $(DIRRES)/, $(OBJRES))
RESALL	= $(addprefix $(DIROBJ)/, $(addsuffix .ro,  $(RESS)))

DEPENDS	= $(OBJALL:.o=.d)


CFLAGS	= -std=c++$(CPPVER) -Wall -Wno-unused-parameter -Wextra -Wno-pmf-conversions -fno-strict-aliasing -mms-bitfields -MMD -MP	\
	  -finput-charset=utf-8 -fexec-charset=utf-8 $(addprefix -I , $(DIRSRCS))	\
	  $(shell pkg-config $(PKGCNFG) --cflags)
LFLAGS	= -static-libgcc -static-libstdc++ --static
RFLAGS	= -J rc -O coff -I $(DIRSRC)/$(DIROSD)
LIBS	= $(shell pkg-config $(PKGCNFG) --static --libs) -lcomctl32 -lwinmm -lshlwapi -limm32 -lole32 -loleaut32 -lsetupapi -lversion -lvfw32 -lintl -liconv

#-----------------------------------------------------------------------------
ifeq ($(CPPVER), 17)
CFLAGS	+= -DUSEFILESYSTEM
LIBS	+= -lstdc++fs
else
LIBS	+= -limagehlp
endif

#-----------------------------------------------------------------------------
ifndef DEBUG
CFLAGS += -O3 -fomit-frame-pointer
LFLAGS += -s
else
CFLAGS += -g -pg -O0
LFLAGS += -g -pg
endif






#=============================================================================
.PHONY: all build clean

all : build

build : $(TARGET)

$(TARGET) : $(OBJALL) $(RESALL)
	@echo Linking $@...
	@mkdir -p $(dir $@)
	$(CC) $(LFLAGS) -o $@ $(OBJALL) $(RESALL) $(LIBS)
ifndef DEBUG
	@strip -s $@
endif

$(DIROBJ)/%.o : $(DIRSRC)/%.cpp #Makefile
	@echo Compiling $<...
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

$(DIROBJ)/%.ro : $(DIRSRC)/%.rc #Makefile
	@echo Compiling resources $<...
	@mkdir -p $(dir $@)
	$(RC) $(RFLAGS) -i $< -o $@

clean :
	@echo Deleting objects ...
	-@$(RM) $(TARGET)
	-@$(RM) -r $(DIROBJ)


-include $(DEPENDS)
