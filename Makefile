OUTDIR=/Object/bin/${HOSTNAME}
GAKLIB=/Object/gaklib/libgak${HOSTNAME}.a
SSLLIB=-lcrypto -lssl

DEBUG=-ggdb
NO_DEBUG=-DNDEBUG -O3
CFLAGS=-I../GAKLIB/INCLUDE -D_REENTRANT ${NO_DEBUG}
# -fpermissive

TOOLS=\
	${OUTDIR}/aes \
	${OUTDIR}/dlink \
	${OUTDIR}/dupMails \
	${OUTDIR}/hash \
	${OUTDIR}/iTunesCheck \
	${OUTDIR}/iTunesCompare \
	${OUTDIR}/iTunesCopy \
	${OUTDIR}/mirror \
	${OUTDIR}/tdiff


build: ${TOOLS}

clean:
	-rm ${TOOLS}

${OUTDIR}/aes: TOOLS/aes.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^  ${SSLLIB}

${OUTDIR}/dlink: TOOLS/dlink.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^  ${SSLLIB}

${OUTDIR}/dupMails: TOOLS/dupMails.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^  ${SSLLIB}

${OUTDIR}/hash: TOOLS/hash.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^  ${SSLLIB}

${OUTDIR}/iTunesCheck: TOOLS/iTunesCheck.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^  ${SSLLIB}

${OUTDIR}/iTunesCompare: TOOLS/iTunesCompare.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^  ${SSLLIB}

${OUTDIR}/iTunesCopy: TOOLS/iTunesCopy.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^  ${SSLLIB}

${OUTDIR}/mirror: TOOLS/mirror.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ ${SSLLIB}

${OUTDIR}/tdiff: TOOLS/tdiff.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^ 

${OUTDIR}/xmlGalery: TOOLS/xmlGalery.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^ 

${OUTDIR}/%.o: %.cpp
	g++ ${CFLAGS} -c $< -o $@

