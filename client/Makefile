include ../Make.defines

CXX = g++
CXXFLAGS = -fsanitize=address -g -pthread
LDFLAGS = -lncursesw
PROGS =	clean client

all:	${PROGS}

client: client.o
	$(CXX) $(CXXFLAGS) -o client client.o $(LDFLAGS)

client.o: client.cpp
	$(CXX) $(CXXFLAGS) -c client.cpp
		
clean:
		rm -f ${PROGS} ${CLEANFILES}
