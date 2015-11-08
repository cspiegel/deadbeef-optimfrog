OBJS=		$(SRCS:%.cpp=%.o)

ifneq ($(PKG),)
ifneq ($(shell pkg-config $(PKG) > /dev/null 2>&1 ; echo $$?),0)
  $(error "Cannot find all packages: $(PKG)")
endif
endif

CXXFLAGS+=	-I$(PREFIX)/include $(shell pkg-config $(PKG) --cflags) -g
LDADD=		$(shell pkg-config $(PKG) --libs)
PLUGDIR=	$(PREFIX)/lib/deadbeef

%.o: %.cpp
	$(CXX) $(OPT) $(CXXFLAGS) -fPIC -c $< -o $@

all: $(TARGET).so

$(TARGET).so: $(OBJS)
	$(CXX) $(OPT) -shared $^ -o $@ $(LDADD)

clean:
	rm -f *.o *.so

install: $(TARGET).so
	mkdir -p $(DESTDIR)$(PLUGDIR)
	install -m755 $^ $(DESTDIR)$(PLUGDIR)
