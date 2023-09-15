NAME = rssr

CC       ?= gcc
CPPFLAGS += -Imods/markov/chains/deps
CFLAGS   += -g
LDFLAGS  += -lcurl

SRC  = ${NAME}.c
SRC += $(wildcard utils/*.c)
OBJ  = ${SRC:.c=.o}

.PHONY: update_mods

.c.o:
	@echo CC -c $<
	@${CC} -c ${CFLAGS} ${CPPFLAGS} $< -o ${<:.c=.o}

${NAME}: ${SRC} ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${CFLAGS} ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning...
	@rm -f ${NAME} ${OBJ} mods/modtape.*

install: ${NAME}
	@echo installing executable file to ${DESTDIR}/bin
	@mkdir -p ${DESTDIR}/bin
	@cp -f ${NAME} ${DESTDIR}/bin/${NAME}
	@chmod 755 ${DESTDIR}/bin/${NAME}

uninstall: ${NAME}
	@echo removing executable file from ${DESTDIR}/bin
	@rm -f ${DESTDIR}/bin/${NAME}
