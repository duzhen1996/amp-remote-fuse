
CC = cc
CP = cp
RM = rm -f
MV = mv -f
# 当前目录就是顶层目录
TOP_DIR = .
INC_AMP = ${TOP_DIR}/include/
INC_THIS=./
CFLAGS = -I${INC_AMP} -I${INC_THIS}

LDPATH_AMP = ${TOP_DIR}/lib/
LIBNAME_AMP = amp
LDFLAGS_AMP = -L${LDPATH_AMP} -l${LIBNAME_AMP}
LDFLAGS = -lm -lrt -lpthread -lfuse -ldl


EXE_FILES = fuse_client fuse_server

all: ${EXE_FILES}
CLIENT_OBJ=fuse_client.o
SERVER_OBJ=fuse_server.o

OBJS = fuse_client.o fuse_server.o

.c.o:
		${CC} -Wall -D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse -L/usr/local/lib -lfuse -pthread -ldl ${CFLAGS} ${EXTRA_CFLAGS} -g -c $*.c

fuse_client: fuse_client.o
		${CC} -O2 -g -o $@ ${CLIENT_OBJ} ${LDFLAGS} ${LDFLAGS_AMP}

fuse_server: fuse_server.o
		${CC} -O2 -g -o $@ ${SERVER_OBJ} ${LDFLAGS} ${LDFLAGS_AMP}

clean:
		${RM} *.o core ~* *.cpp ${EXE_FILES}

