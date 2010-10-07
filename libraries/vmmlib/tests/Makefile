
.PHONY: tests

TESTFILES = $(wildcard *.cpp)
OBJECTS   = $(TESTFILES:%.cpp=%.o)
CXXFLAGS += -I../include

tests: vmmltest
	@echo "----- Running Tests -----"
	@./vmmltest

vmmltest: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS)

clean: 
	rm -rf $(OBJECTS) vmmltest

$(OBJECTS): ../include

