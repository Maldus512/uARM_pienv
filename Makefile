hal := hal
app := example

targets := $(hal) $(app)


.PHONY: all clean $(hal) $(app)

all: $(targets)
	cp $(hal)/kernel8.img boot

$(app):
	$(MAKE) --directory=$@
	cp $@/app.elf $(hal)

$(hal): $(app)
	$(MAKE) --directory=$@ APP=app.elf


run:
	$(MAKE) --directory=$(hal) run

clean:
	for dir in $(targets); do \
		$(MAKE) --directory $$dir -f Makefile $@; \
	done