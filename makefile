Target = myarc
CC = gcc
build = ./build/
OS = linux
FORMAT = c


bin = ./bin/
folders += ${bin}
folders += ${build}

SCR += ${wildcard *.$(FORMAT)}
OBJ += ${patsubst %.$(FORMAT),${bin}%.o, ${SCR}}
OBJ_DEL += ${subst /,\, ${OBJ}}
FOLDERS  = ${subst /,\, ${folders}}

DFlags += -D linux
CFlags += -O3 
LFlags += -L ./ -lz


ifeq ($(OS),windows)
    MKDIR = mkdir
	DEL= del
else
    MKDIR = mkdir -p
	DEL = rm -f
endif



buildApp: binFile clear appBuilder
	${build}${Target} -p -w -f ${build}arhice.arc -d ./testDir
	${build}${Target} -p -r -f ${build}arhice.arc -d ./

appBuilder: ${OBJ}
	${CC} -o ${build}${Target} $^ ${resources} ${Libs} ${CFlags} ${LFlags}

libBuilder: ${OBJ}
	ar rcs ${libpath}lib${Target}.a ${OBJ}

binFile:
	for dir in $(folders) ; do if [ ! -d $$dir ]; then $(MKDIR) $$dir; fi; done

${bin}%.o: %.$(FORMAT)
	${CC} ${IFlags} ${DFlags} -o $@ -c $< ${CFlags}
	
clear:
	${DEL} ${OBJ_DEL}