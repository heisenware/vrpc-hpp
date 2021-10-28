TARGET = vrpc-bar-agent
CPPFLAGS = -pthread -fPIC -m64 -O3 -std=c++14 -Wall -Wextra
LDFLAGS = -pthread

SRCS := $(shell find ./src -name *.cpp)
OBJS := $(addsuffix .o,$(basename $(SRCS)))
DEPS := $(OBJS:.o=.d)

$(TARGET): $(OBJS)
		$(CXX) $(LDFLAGS) $(OBJS) -o $@ $(LOADLIBES) $(LDLIBS)

.PHONY: clean
clean:
		$(RM) $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
