#
# Make file for AgFS programs
#

OPTFLAGS = -O2
CXX = clang++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++11 $(OPTFLAGS) -D_FILE_OFFSET_BITS=64

LDFLAGS = -lboost_system -lpthread -lboost_filesystem -lfuse

TARGETS = agfs-keygen agfsd agfs


all: $(TARGETS)

agfs-keygen: agfs-keygen.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

agfsd: agfsd.o agfs-server.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

agfs: agfs.o serverconnection.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Make rule to clean compiled binaries

clean:
	rm *.o $(TARGETS)

# Auto generated using clang++ -MM *.cpp -D_FILE_OFFSET_BITS=64 -std=c++11

agfs-client.o: agfs-client.cpp
agfs.o: agfs.cpp serverconnection.hpp constants.hpp
agfsd.o: agfsd.cpp constants.hpp agfs-server.hpp
agfs-keygen.o: agfs-keygen.cpp
agfs-server.o: agfs-server.cpp
serverconnection.o: serverconnection.cpp serverconnection.hpp \
  constants.hpp