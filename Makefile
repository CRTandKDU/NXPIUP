# Define compilers and tools
# --------------------------------------------------------------------------------
CPP 		= g++
CC              = g++
MSVCPP          = "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.41.34120/bin/Hostx86/arm/cl.exe"

# Directories
# --------------------------------------------------------------------------------
APIS_DIR	= C:/Users/chauv/Documents/IUP
DSL_DIR		= C:/cygwin64/home/Moria
LIBS_DIR	= ./lib

# Include flags
# --------------------------------------------------------------------------------
CFLAGS_IUP      = -I./include -I./include/cd -I./include/im
CFLAGS_CURL     = `curl-config --cflags`
CFLAGS_BOOST	= -I$(APIS_DIR)/boost_1_91_0
CFLAGS_ZHASH	= -I$(APIS_DIR)/zhash/src
CFLAGS_WEB      = -I./webview-master/core/include

MSV_CFLAGS      = -I"C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.41.34120/include"

APIS_CFLAGS	= -I$(APIS_DIR) -I$(DSL_DIR)/libforth -I$(DSL_DIR)/embed-master -I$(DSL_DIR)/libcsv -I$(APIS_DIR)/zhash

CFLAGS_NXP	= $(API_CFLAGS) $(CFLAGS_ZHASH) $(CFLAGS_BOOST) $(CFLAGS_CURL)
CFLAGS		= -s


# Linker Flags
# --------------------------------------------------------------------------------
LIBS_CD		=  $(LIBS_DIR)/cdcontextplus.dll   $(LIBS_DIR)/cd.dll
# $(LIBS_DIR)/cdcairo.dll   $(LIBS_DIR)/cddirect2d.dll  $(LIBS_DIR)/cdgl.dll  $(LIBS_DIR)/cdim.dll  $(LIBS_DIR)/cdlua54.dll  $(LIBS_DIR)/cdluacairo54.dll  $(LIBS_DIR)/cdluacontextplus54.dll  $(LIBS_DIR)/cdluadirect2d54.dll  $(LIBS_DIR)/cdluagl54.dll  $(LIBS_DIR)/cdluaim54.dll  $(LIBS_DIR)/cdluapdf54.dll  $(LIBS_DIR)/cdpdf.dll

LIBS_IM         = $(LIBS_DIR)/im.dll $(LIBS_DIR)/iupim.dll
LIBS_IUP	= $(LIBS_DIR)/iup.dll $(LIBS_DIR)/iupcd.dll $(LIBS_DIR)/iupcontrols.dll
LIBS_WEB        = $(LIBS_DIR)/iupweb.dll 
LIBS_CURL       = `curl-config --libs`
LIBS            = $(LIBS_CD) $(LIBS_IUP) $(LIBS_IM) $(LIBS_CURL)

LFLAGS		= -L./lib


# NXP DSL (Embed FORTH) Flags Section
# --------------------------------------------------------------------------------
DSL_CFLAGS	= -D ENGINE_DSL -D ENGINE_DSL_HOWERJFORTH
DSL_LFLAGS	= $(DSL_DIR)/libcsv/libcsv_la-libcsv.o $(DSL_DIR)/embed-master/util.o -L$(DSL_DIR)/embed-master -lembed # -lm

# 3) NXP (40y, 2025 edition) Object files Section
# --------------------------------------------------------------------------------
APIS_OBJS_NXP	= $(APIS_DIR)/sign.o $(APIS_DIR)/rule.o $(APIS_DIR)/hypo.o $(APIS_DIR)/compound.o $(APIS_DIR)/engine.o $(APIS_DIR)/engine_dsl.o $(APIS_DIR)/loadkb.o $(APIS_DIR)/nxp_hash.o $(APIS_DIR)/nxp_evoke.o

APIS_DEPS	= $(APIS_DIR)/agenda.h $(APIS_DIR)/Makefile

OBJS_ZHASH	= $(APIS_DIR)/zhash/src/zhash.o
OBJS_CURL       = $(APIS_DIR)/hypo_remote.o $(APIS_DIR)/hypo_remote_get.o

# Rules Network Section
# --------------------------------------------------------------------------------
# CSOURCES_NETW		= netw.c netw_internals.c netw_expansion.c netw_redraw.c
OBJS_NETW		= $(APIS_DIR)/netw.o $(APIS_DIR)/netw_internals.o $(APIS_DIR)/netw_expansion.o $(APIS_DIR)/netw_redraw.o

# IUP GUI Section
# --------------------------------------------------------------------------------
# CSOURCES_NXPIUP	= nxpiup_menu.c nxpiup_ency.c  nxp_layout.cpp layout.cpp
OBJS_NXPIUP		= $(APIS_DIR)/nxpiup_question.o $(APIS_DIR)/nxpiup_menu.o $(APIS_DIR)/nxpiup_ency.o  $(APIS_DIR)/nxp_layout.o $(APIS_DIR)/layout.o


# MAIN
canvas3: canvas3.c $(OBJS_NXPIUP) $(OBJS_NETW) $(APIS_OBJS_NXP) $(OBJS_ZHASH) $(OBJS_CURL) 
	$(CPP) $^ -o canvas3.exe  $(CFLAGS) $(DSL_CFLAGS) $(CFLAGS_NXP) $(CFLAGS_IUP) $(LFLAGS) $(DSL_LFLAGS) $(LIBS)

clean_nxp:
	rm -i $(APIS_OBJS_NXP)

# canvas2: canvas2.c
# 	gcc canvas2.c -o canvas2.exe $(CFLAGS) $(LFLAGS) $(LIBS)

# canvas1: canvas1.c
# 	gcc canvas1.c -o canvas1.exe $(CFLAGS) $(LFLAGS) $(LIBS)

# canvas4: canvas4.c layout.cpp C:/Users/chauv/Documents/IUP/boost_1_91_0/boost/graph/kamada_kawai_spring_layout.hpp
# 	g++ canvas4.c layout.cpp -o canvas4.exe $(CFLAGS) $(LFLAGS) $(LIBS) -IC:/Users/chauv/Documents/IUP/boost_1_91_0

# list1: examples/C/list1.c
# 	gcc examples/C/list1.c -o list1.exe $(CFLAGS) $(LFLAGS) $(LIBS)

# list2: examples/C/matrixlist.c
# 	gcc examples/C/matrixlist.c -o list2.exe $(CFLAGS) $(LFLAGS) $(LIBS)

# textformat: examples/C/textformat.c
# 	gcc examples/C/textformat.c -o textformat.exe $(CFLAGS) $(LFLAGS) $(LIBS)

# grid1: examples/C/gridbox.c
# 	gcc examples/C/gridbox.c -o grid1.exe $(CFLAGS) $(LFLAGS) $(LIBS)

# grid2: examples/C/gridbox2.c
# 	gcc examples/C/gridbox2.c -o grid2.exe $(CFLAGS) $(LFLAGS) $(LIBS)

# menu: examples/C/menu.c
# 	gcc examples/C/menu.c -o menu.exe $(CFLAGS) $(LFLAGS) $(LIBS)

# nxpiupmenu: nxpiup_menu.c
# 	gcc nxpiup_menu.c -o menu.exe $(CFLAGS) $(CFLAGS_NXP) $(LFLAGS) $(LIBS)

# helloz: helloz.c nxp_hash.c 
# 	gcc helloz.c nxp_hash.c $(APIS_DIR)/zhash/src/zhash.c $(APIS_DIR)/zhash/src/zsorted_hash.c -o helloz.exe $(CFLAGS_ZHASH)

# webbrowser: examples/C/webbrowser.c Makefile
# 	$(CC) examples/C/webbrowser.c -o webbrowser.exe $(CFLAGS) $(LFLAGS) $(LIBS) $(LIBS_WEB)

# wv: wv.c Makefile
# 	$(CC) wv.c -o wv.exe $(CFLAGS) $(CFLAGS_WEB) $(LFLAGS) $(LIBS) $(LIBS_WEB)

# layout: layout.cpp
# 	g++ layout.cpp -o layout.exe -IC:/Users/chauv/Documents/IUP/boost_1_91_0

# zbox: examples/C/zbox.c Makefile
# 	$(CC) examples/C/zbox.c -o zbox.exe $(CFLAGS) $(LFLAGS) $(LIBS) $(LIBS_WEB)

# curltest: curltest.c
# 	$(CC) curltest.c -o curltest.exe  $(APIS_CFLAGS) $(CFLAGS) $(DSL_CFLAGS) `curl-config --cflags` `curl-config --libs`

# curltest: hypo_remote_get.o
# 	$(CC) hypo_remote_get.o -o curltest.exe $(CFLAGS) `curl-config --cflags` `curl-config --libs`

# Generics
# --------------------------------------------------------------------------------
%.o: %.c  $(API_DEPS)
	$(CC) -c -o $@ $< $(APIS_CFLAGS) $(CFLAGS) $(CFLAGS_ZHASH) $(DSL_CFLAGS)

%.o: %.cpp $(API_DEPS) 
	$(CPP) -c -o $@ $< $(CFLAGS_NXP) $(CFLAGS) $(CFLAGS_ZHASH) $(DSL_CFLAGS)
