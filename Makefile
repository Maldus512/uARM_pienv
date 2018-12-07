hal := hal
app := example

targets :=  $(app) $(hal)


.PHONY: all clean $(hal) $(app)

all: $(targets)
	cp $(hal)/kernel8.img boot

$(app):
	$(MAKE) --directory=$@
	cp $@/app.elf $(hal)

$(hal): $(app)
	$(MAKE) --directory=$@ APP=app.elf

run: $(targets)
	$(MAKE) --directory=$(hal) run APP=app.elf

run2: $(targets)
	$(MAKE) --directory=$(hal) run2 APP=app.elf

debug: $(targets)
	$(MAKE) --directory=$(hal) debug APP=app.elf

clean:
	for dir in $(targets); do \
		$(MAKE) --directory $$dir -f Makefile $@; \
	done