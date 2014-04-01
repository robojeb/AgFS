#
# Make file for AgFS programs
#

OPTFLAGS = -O2
CXX = clang++
CXXFLAGS = -Wall -Wextra -pedantic -g -std=c++11 $(OPTFLAGS) -D_FILE_OFFSET_BITS=64

LDFLAGS = -lboost_system -lpthread -lboost_filesystem -lfuse

TARGETS = agfs-keygen agfsd agfs


all: $(TARGETS)

agfs-keygen: agfs-keygen.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

agfsd: agfsd.o agfs-server.o agfsio.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

agfs: agfs.o serverconnection.o agfsio.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Make rule to clean compiled binaries

clean:
	rm *.o $(TARGETS)

# Auto generated using clang++ -MM *.cpp -D_FILE_OFFSET_BITS=64 -std=c++11

agfs-client.o: agfs-client.cpp
agfs.o: agfs.cpp serverconnection.hpp constants.hpp
agfsd.o: agfsd.cpp constants.hpp agfs-server.hpp
agfsio.o: agfsio.cpp
agfs-keygen.o: agfs-keygen.cpp constants.hpp
agfs-server.o: agfs-server.cpp agfs-server.hpp constants.hpp agfsio.hpp
serverconnection.o: serverconnection.cpp serverconnection.hpp \
  constants.hpp agfsio.hpp
