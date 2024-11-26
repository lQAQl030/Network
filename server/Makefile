include ../Make.defines

PROGS =	clean server

all:	${PROGS}

server:
		${CXX} -o server server.cpp Timer.cpp
		
clean:
		rm -f ${PROGS} ${CLEANFILES}
