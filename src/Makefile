#
# Make file for AgFS programs
#

OPTFLAGS = -O2
CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++11 $(OPTFLAGS)

LDFLAGS = -lboost_system -lpthread

TARGETS = agfs-keygen


all: $(TARGETS)

agfs-keygen: agfs-keygen.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)


# Make rule to clean compiled binaries

clean:
	rm *.o $(TARGETS)

# Auto generated using g++ -MM *.cpp

agfs-keygen.o: agfs-keygen.cpp