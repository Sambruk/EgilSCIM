TARGET = SimpleSCIM
TARGETDIR = src
PREFIX = /usr/bin
CONFDIR = /etc/SimpleSCIM

INSTALL = install
CP = cp
RM = rm -f

all: $(TARGET)

$(TARGET): $(TARGETDIR)/$(TARGET)
	@echo "CP $(TARGETDIR)/$(TARGET) $(TARGET)"
	@$(CP) $(TARGETDIR)/$(TARGET) $(TARGET)

install: $(PREFIX)/$(TARGET) $(CONFDIR)

$(PREFIX)/$(TARGET):
	@echo "INSTALL $(TARGET) $(PREFIX)/$(TARGET)"
	@$(INSTALL) $(TARGET) $(PREFIX)/$(TARGET)

$(CONFDIR):
	mkdir $(CONFDIR)
	mkdir $(CONFDIR)/conf
	mkdir $(CONFDIR)/cache
	mkdir $(CONFDIR)/cert
	$(CP) res/template.conf $(CONFDIR)/conf/

.PHONY: $(TARGETDIR)/$(TARGET) clean uninstall

uninstall:
	@echo "RM $(PREFIX)/$(TARGET)"
	@$(RM) $(PREFIX)/$(TARGET)

$(TARGETDIR)/$(TARGET):
	@echo "MAKE $(TARGETDIR)/$(TARGET)"
	@$(MAKE) -C $(TARGETDIR)

clean:
	@echo "MAKE $(TARGETDIR)/$(TARGET)" clean
	@$(MAKE) -C $(TARGETDIR) clean
	@echo "RM $(TARGET)"
	@$(RM) $(TARGET)
