Target = myarc
CC = gcc
build = ./build/

bin = ./bin/
folders += ${bin}
folders += ${build}

SCR += ${wildcard *.c}
OBJ += ${patsubst %.c,${bin}%.o, ${SCR}}
OBJ_DEL += ${subst /,\, ${OBJ}}

DFlags += -D windows
CFlags += -O3 

buildApp: binFile clear appBuilder
	${build}${Target} -p -w -f ${build}arhice.arc -d ./testDir
	${build}${Target} -p -r -f ${build}arhice.arc -d ./

appBuilder: ${OBJ}
	${CC} -o ${build}${Target} $^ ${resources} ${Libs} ${CFlags} ${LFlags}

libBuilder: ${OBJ}
	ar rcs ${libpath}lib${Target}.a ${OBJ}

binFile: 
	@mkdir -p ${folders}

${bin}%.o: %.c
	${CC} ${IFlags} ${DFlags} -o $@ -c $< ${CFlags}
	
clear:
	rm -f ${OBJ} ${build}${Target}
	rm -f ${build}archive.arc