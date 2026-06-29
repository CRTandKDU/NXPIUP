CPP 		= g++
CC              = g++
MSVCPP          = "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.41.34120/bin/Hostx86/arm/cl.exe"

# 1) IUP, CD and IM Section
CFLAGS		= -s -I./include -I./include/cd -I./include/im
LFLAGS		= -I./lib
LIBS_DIR	= ./lib
LIBS_CD		=  $(LIBS_DIR)/cdcontextplus.dll   $(LIBS_DIR)/cd.dll # $(LIBS_DIR)/cdcairo.dll   $(LIBS_DIR)/cddirect2d.dll  $(LIBS_DIR)/cdgl.dll  $(LIBS_DIR)/cdim.dll  $(LIBS_DIR)/cdlua54.dll  $(LIBS_DIR)/cdluacairo54.dll  $(LIBS_DIR)/cdluacontextplus54.dll  $(LIBS_DIR)/cdluadirect2d54.dll  $(LIBS_DIR)/cdluagl54.dll  $(LIBS_DIR)/cdluaim54.dll  $(LIBS_DIR)/cdluapdf54.dll  $(LIBS_DIR)/cdpdf.dll
LIBS_IM         = $(LIBS_DIR)/im.dll $(LIBS_DIR)/iupim.dll
LIBS_IUP	= $(LIBS_DIR)/iup.dll $(LIBS_DIR)/iupcd.dll
LIBS_WEB        = $(LIBS_DIR)/iupweb.dll 
LIBS		=  $(LIBS_DIR)/cdcontextplus.dll $(LIBS_DIR)/iupcd.dll $(LIBS_DIR)/iup.dll $(LIBS_DIR)/cd.dll $(LIBS_DIR)/iupcontrols.dll # ./lib/gdi32.dll ./lib/comdlg32.dll ./lib/comctl32.dll ./lib/uuid.dll ./lib/oleaut32.dll ./lib/ole32.dll

MSV_CFLAGS      = -I"C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.41.34120/include"

LIBS_CURL       = `curl-config --libs`

# 2) NXP DSL (Embed FORTH) Section
DSL_DIR			= C:/cygwin64/home/Moria
DSL_CFLAGS		= -D ENGINE_DSL -D ENGINE_DSL_HOWERJFORTH
DSL_LFLAGS		= $(DSL_DIR)/libcsv/libcsv_la-libcsv.o $(DSL_DIR)/embed-master/util.o -L$(DSL_DIR)/embed-master -lembed # -lm

# 3) NXP (40y, 2025 edition) Section
APIS_DIR		= C:/Users/chauv/Documents/IUP
APIS_OBJS_NXP		= $(APIS_DIR)/sign.o $(APIS_DIR)/rule.o $(APIS_DIR)/hypo.o $(APIS_DIR)/compound.o $(APIS_DIR)/engine.o $(APIS_DIR)/engine_dsl.o $(APIS_DIR)/loadkb.o $(APIS_DIR)/nxp_hash.o $(APIS_DIR)/nxp_evoke.o
APIS_DEPS		= agenda.h Makefile
APIS_CFLAGS		= -I$(APIS_DIR) -I$(DSL_DIR)/libforth -I$(DSL_DIR)/embed-master -I$(DSL_DIR)/libcsv -I$(APIS_DIR)/zhash

CFLAGS_CURL             = `curl-config --cflags`
CFLAGS_BOOST		= -I$(APIS_DIR)/boost_1_91_0
CFLAGS_ZHASH		= -I$(APIS_DIR)/zhash
CFLAGS_WEB              = -I./webview-master/core/include
CFLAGS_NXP		= $(API_CFLAGS) $(CFLAGS_ZHASH) $(CFLAGS_BOOST) $(CFLAGS_CURL)

OBJS_ZHASH		= $(APIS_DIR)/zhash/src/zhash.o
OBJS_CURL               = hypo_remote.o hypo_remote_fopen.o

# 4) Rules Network Section
# CSOURCES_NETW		= netw.c netw_internals.c netw_expansion.c netw_redraw.c
OBJS_NETW		= netw.o netw_internals.o netw_expansion.o netw_redraw.o

# 5) IUP GUI Section
# CSOURCES_NXPIUP	= nxpiup_menu.c nxpiup_ency.c  nxp_layout.cpp layout.cpp
OBJS_NXPIUP		= nxpiup_question.o nxpiup_menu.o nxpiup_ency.o  nxp_layout.o layout.o


# MAIN
canvas3: canvas3.c $(OBJS_NXPIUP) $(OBJS_NETW) $(APIS_OBJS_NXP) $(OBJS_ZHASH) $(OBJS_CURL)
	$(CPP) $^ -o canvas3.exe  $(CFLAGS) $(DSL_CFLAGS) $(CFLAGS_NXP) $(LFLAGS) $(DSL_LFLAGS) $(LIBS) $(LIBS_IM) $(LIBS_CURL)

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

curltest: hypo_remote_fopen.o
	$(CC) hypo_remote_fopen.o -o curltest.exe $(CFLAGS) `curl-config --cflags` `curl-config --libs`

%.o: %.c $(API_DEPS)
	$(CC) -c -o $@ $< $(APIS_CFLAGS) $(CFLAGS) $(DSL_CFLAGS)

%.o: %.cpp $(API_DEPS)
	$(CPP) -c -o $@ $< $(CFLAGS_NXP) $(CFLAGS) $(DSL_CFLAGS)
