
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
LDFLAGS = -lm -lpthread -lrt `pkg-config fuse --cflags --libs`


EXE_FILES = fuse_client
all: ${EXE_FILES}
CLIENT_OBJ=fuse_client.o

OBJS = fuse_client.o

.c.o:
		${CC} -Wall ${CFLAGS} ${EXTRA_CFLAGS} -g -c $*.c

fuse_client: fuse_client.o
		${CC} -Wall  -O2 -g -o $@ ${CLIENT_OBJ} ${LDFLAGS} ${LDFLAGS_AMP} 

clean:
	${RM} *.o core ~* *.cpp ${EXE_FILES}

