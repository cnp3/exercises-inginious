CC=gcc
EXEC=tests
LDFLAGS=-lcunit -lm -lpthread -ldl -rdynamic
SRC=$(sort $(wildcard *.c)) $(sort $(wildcard CTester/*.c))
OBJ=$(SRC:.c=.o)
CFLAGS = -Wall -Werror -Wextra -Wshadow
CFLAGS += -std=gnu99 # was gnu99 FIXME
#CFLAGS += -DC99 # Don't know why TODO
CFLAGS += -ICTester # to include the folder.
#CFLAGS += -D_POSIX_C_SOURCE=200112L -D_DEFAULT_SOURCE
DEBUGFLAGS = -g -Wno-unused-variable -Wno-unused-parameter
OTHERFLAGS = -fstack-protector-all -D_FORTIFY_SOURCE=2 -fno-omit-frame-pointer
#CFLAGS += $(DEBUGFLAGS)
#CFLAGS += $(OTHERFLAGS)
WRAP += -Wl,-wrap=exit
WRAP += -Wl,-wrap=open -Wl,-wrap=creat -Wl,-wrap=close -Wl,-wrap=read -Wl,-wrap=write -Wl,-wrap=stat -Wl,-wrap=fstat -Wl,-wrap=lseek
WRAP += -Wl,-wrap=getpid
WRAP += -Wl,-wrap=malloc -Wl,-wrap=free -Wl,-wrap=realloc -Wl,-wrap=calloc
WRAP += -Wl,-wrap=pthread_mutex_lock -Wl,-wrap=pthread_mutex_unlock -Wl,-wrap=pthread_mutex_trylock -Wl,-wrap=pthread_mutex_init -Wl,-wrap=pthread_mutex_destroy
WRAP += -Wl,-wrap=getaddrinfo -Wl,-wrap=getnameinfo -Wl,-wrap=freeaddrinfo -Wl,-wrap=gai_strerror
WRAP += -Wl,-wrap=accept -Wl,-wrap=bind -Wl,-wrap=connect -Wl,-wrap=listen -Wl,-wrap=poll -Wl,-wrap=recv -Wl,-wrap=recvfrom -Wl,-wrap=recvmsg -Wl,-wrap=select -Wl,-wrap=send -Wl,-wrap=sendto -Wl,-wrap=sendmsg -Wl,-wrap=shutdown -Wl,-wrap=socket
WRAP += -Wl,-wrap=htons -Wl,-wrap=ntohs -Wl,-wrap=htonl -Wl,-wrap=ntohl
WRAP += -Wl,-wrap=sleep

all: $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

$(EXEC): $(OBJ)
	$(CC) $(WRAP) -o $@ $(OBJ) $(LDFLAGS)

create-po:
	mkdir -p po/fr/
	xgettext --keyword=_ --language=C --add-comments --sort-output --from-code=UTF-8 -o po/tests.pot $(SRC)
	msginit --input=po/tests.pot --locale=fr_BE.utf8 --output=po/fr/tests.po

update-po:
	xgettext --keyword=_ --language=C --add-comments --sort-output --from-code=UTF-8 -o po/tests.pot $(SRC)
	msgmerge --update po/fr/tests.po po/tests.pot

compile-mo:
	msgfmt --no-hash --output-file=po/fr/tests.mo po/fr/tests.po
	mkdir -p fr/LC_MESSAGES/
	cp po/fr/tests.mo fr/LC_MESSAGES/tests.mo

clean:
	rm -f $(EXEC) $(OBJ)
#clr-aux:
#	rm -f $(INC) $(ASM)
rebuild: clean all


.PHONY: tests

