TARGET = vrpc-bar-agent
CPPFLAGS = -I. -pthread -fPIC -m64 -O2 -std=c++14
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
