OUTDIR=/Object/bin/${HOSTNAME}
GAKLIB=/Object/gaklib/libgak${HOSTNAME}.a
SSLLIB=-lcrypto -lssl

DEBUG=-ggdb
NO_DEBUG=-DNDEBUG -O3
CFLAGS=-I../GAKLIB/INCLUDE -D_REENTRANT ${NO_DEBUG}
# -fpermissive


TOOLS=\
	${OUTDIR}/aes \
	${OUTDIR}/cliChess \
	${OUTDIR}/dlink \
	${OUTDIR}/dupMails \
	${OUTDIR}/hash \
	${OUTDIR}/iTunesCheck \
	${OUTDIR}/iTunesCompare \
	${OUTDIR}/iTunesCopy \
	${OUTDIR}/mboxIndexer \
	${OUTDIR}/mboxSearch \
	${OUTDIR}/minMax \
	${OUTDIR}/mirror \
	${OUTDIR}/season \
	${OUTDIR}/tdiff \
	${OUTDIR}/treeSize \
	${OUTDIR}/twins
#	${OUTDIR}/xmlGalery		not yet ported to linux


build: ${TOOLS}

clean:
	-rm ${TOOLS}

${OUTDIR}/aes: TOOLS/aes.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^  ${SSLLIB}

${OUTDIR}/cliChess: TOOLS/cliChess.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^  ${SSLLIB}

${OUTDIR}/dlink: TOOLS/dlink.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^  ${SSLLIB}

${OUTDIR}/dupMails: TOOLS/dupMails.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^  ${SSLLIB}

${OUTDIR}/hash: TOOLS/hash.cpp ${GAKLIB}
	g++ ${CFLAGS} -o $@ $^  ${SSLLIB}

${OUTDIR}/iTunesCheck: TOOLS/iTunesCheck.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^  ${SSLLIB}

${OUTDIR}/iTunesCompare: TOOLS/iTunesCompare.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^  ${SSLLIB}

${OUTDIR}/iTunesCopy: TOOLS/iTunesCopy.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^  ${SSLLIB}

${OUTDIR}/mboxIndexer: TOOLS/mboxIndexer.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ ${SSLLIB}

${OUTDIR}/mboxSearch: TOOLS/mboxSearch.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ ${SSLLIB}

${OUTDIR}/minMax: TOOLS/minMax.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ ${SSLLIB}

${OUTDIR}/mirror: TOOLS/mirror.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ ${SSLLIB}

${OUTDIR}/season: TOOLS/season.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ ${SSLLIB}

${OUTDIR}/tdiff: TOOLS/tdiff.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ 

${OUTDIR}/treeSize: TOOLS/treeSize.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ 

${OUTDIR}/twins: TOOLS/twins.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ 

${OUTDIR}/xmlGalery: TOOLS/xmlGalery.cpp ${GAKLIB}
	g++ ${CFLAGS} -lpthread -o $@ $^ 

${OUTDIR}/%.o: %.cpp
	g++ ${CFLAGS} -c $< -o $@

