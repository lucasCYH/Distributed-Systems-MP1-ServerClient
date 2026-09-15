.PHONY: all test clean

CXX      = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17 -pthread

TESTDIR = test
BINDIR  = bins
LOGDIR  = logs

BINS = $(BINDIR)/server $(BINDIR)/client \
       $(BINDIR)/LogGenerator $(BINDIR)/UnitTest \
	   $(BINDIR)/dispatch_log
OBJS = $(BINDIR)/machine.o $(BINDIR)/metrics.o

all: $(BINS)

$(BINDIR) $(LOGDIR):
	mkdir -p $@

$(BINDIR)/machine.o: machine.cpp machine.hpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) -c machine.cpp -o $@

$(BINDIR)/metrics.o: metrics.cpp metrics.hpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) -c metrics.cpp -o $@

$(BINDIR)/server: server.cpp machine.hpp protocol.hpp $(OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) server.cpp $(OBJS) -o $@

$(BINDIR)/client: client.cpp machine.hpp metrics.hpp $(OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) client.cpp $(OBJS) -o $@

$(BINDIR)/LogGenerator: $(TESTDIR)/LogGenerator.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BINDIR)/UnitTest: $(TESTDIR)/UnitTest.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BINDIR)/dispatch_log: $(TESTDIR)/dispatch_log.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $< -o $@

test: $(BINS) | $(LOGDIR)
	./$(BINDIR)/UnitTest

clean:
	rm -rf $(BINDIR) $(LOGDIR) $(OBJS)