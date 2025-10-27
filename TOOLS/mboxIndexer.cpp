/*
		Project:		GAK_CLI
		Module:			mboxIndexer.cpp
		Description:	index mbox files for AI and search
		Author:			Martin Gäckler
		Address:		Hofmannsthalweg 14, A-4030 Linz
		Web:			https://www.gaeckler.at/

		Copyright:		(c) 1988-2025 Martin Gäckler

		This program is free software: you can redistribute it and/or modify  
		it under the terms of the GNU General Public License as published by  
		the Free Software Foundation, version 3.

		You should have received a copy of the GNU General Public License 
		along with this program. If not, see <http://www.gnu.org/licenses/>.

		THIS SOFTWARE IS PROVIDED BY Martin Gäckler, Austria, Linz ``AS IS''
		AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
		TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
		PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
		CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
		SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
		LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
		USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
		ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
		OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
		OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
		SUCH DAMAGE.
*/

/*
	command lines:
	-D 5 -T 32 X:\Mail\Onlinedienste -I C:\TEMP\gak\Index -B C:\TEMP\gak\Index -S X:\MailIndex\stopword.txt
	-D 5 -T 32 X:\Mail\Newsletter -I C:\TEMP\gak\Index -B C:\TEMP\gak\Index -S X:\MailIndex\stopword.txt
	-D 5 -B X:\MailIndex -i X:\MailIndex -s X:\MailIndex\stopword.txt -m X:\Mail W:\mail\gak
*/

// --------------------------------------------------------------------- //
// ----- switches ------------------------------------------------------ //
// --------------------------------------------------------------------- //

#define DEBUG_LOG		1
#define PROFILER		1
#define USE_PAIR_MAP	0		// for searching pair map is better than TreeMap that is faster for indexing

// --------------------------------------------------------------------- //
// ----- includes ------------------------------------------------------ //
// --------------------------------------------------------------------- //

#include <gak/threadDirScanner.h>
#include <gak/cmdlineParser.h>
#include <gak/numericString.h>
#include <gak/mboxParser.h>
#include <gak/indexer.h>
#include <gak/directoryEntry.h>
#include <gak/strFiles.h>
#include <gak/stopWatch.h>
#include <gak/shared.h>
#include <gak/aiBrain.h>

#include "mboxIndex.h"

// --------------------------------------------------------------------- //
// ----- imported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module switches ----------------------------------------------- //
// --------------------------------------------------------------------- //

#ifdef __BORLANDC__
#	pragma option -RT-
#	pragma option -b
#	pragma option -a4
#	pragma option -pc
#endif

using namespace gak;
using namespace gak::mail;
using namespace gak::ai;

// --------------------------------------------------------------------- //
// ----- constants ----------------------------------------------------- //
// --------------------------------------------------------------------- //

static const int FLAG_USE_META		= 0x010;
static const int OPT_STOP_WORDS		= 0x020;
static const int OPT_INDEX_PATH		= 0x040;
static const int OPT_BRAIN_PATH		= 0x080;
static const int OPT_THREAD_COUNT	= 0x100;
static const int FLAG_FORCE			= 0x200;
static const int OPT_WORD_DISTANCE	= 0x400;

static const char CHAR_USE_META		= 'M';
static const char CHAR_STOP_WORDS	= 'S';
static const char CHAR_INDEX_PATH	= 'I';
static const char CHAR_BRAIN_PATH	= 'B';
static const char CHAR_THREAD_COUNT	= 'T';
static const char CHAR_FORCE		= 'F';
static const char CHAR_DISTANCE		= 'D';

static const size_t DEF_THREAD_COUNT = 8UL;
static const size_t DEF_WORD_DISTANCE = 3UL;

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

static void showProgress( size_t idx, size_t max )
{
	static Locker		s_locker;
	static StopWatch	s_watch(true);

	if( s_watch.getMillis() > 1000 )
	{
		LockGuard guard( s_locker );
		if( guard )
		{
			if( s_watch.getMillis() > 1000 )
			{
				std::cout << idx << '/' << max << "     \r";
				s_watch.start();
			}
		}
	}
}

struct MailIndexerCmd
{
	STRING		theName;
	size_t		idx;
	StringIndex	index;
	
	MailIndexerCmd(const STRING &theName, size_t idx, const StringIndex &index) : theName(theName),idx(idx), index(index) {}
};

typedef SharedPointer<MailIndexerCmd> MailIndexerPtr;

inline MailIndexerPtr createIndexerCmd(const STRING &theName, size_t idx, const StringIndex &index)
{
	return MailIndexerPtr::makeShared(theName, idx, index);
}

template <>
struct ProcessorType<MailIndexerPtr>
{
	typedef MailIndexerPtr object_type;

	static MailIndex		s_mailIndex;
	static bool				s_changed;
	static Locker			s_locker;

	void process( const MailIndexerPtr &ptr )
	{
		doEnterFunctionEx(gakLogging::llInfo,"ProcessorType<MailIndexerCmd>::mergeIndex");
		LockGuard guard( s_locker );

		if( guard )
		{
			const MailIndexerCmd &cmd = *ptr;
			s_mailIndex.mergeIndexPositions( MailAddress(cmd.theName, cmd.idx), cmd.index );
			s_changed = true;
		}
		else
		{
			doLogMessageEx( gakLogging::llError, "Cannot lock" );
		}
	}
};


static ThreadPool<MailIndexerPtr>	*g_IndexerPool=NULL;

template <>
struct ProcessorType<STRING>
{
	typedef STRING object_type;

	static size_t			s_mailCount;
	static Set<CI_STRING>	s_stopWords;
	static STRING			s_arg;
	static STRING			s_indexPath;

	static int				s_flags;

	static Locker			s_locker;
	static Brain			s_Brain;
	static bool				s_changed;
	static STRING			s_brainFile;
	static size_t			s_wordDistance;

	static void init(const CommandLine	&cmdLine)
	{
		doEnterFunctionEx(gakLogging::llInfo, "ProcessorType<STRING>::init");
		s_mailCount = 0;
		if( cmdLine.flags & OPT_STOP_WORDS )
		{
			STRING stopWordsFile = cmdLine.parameter[CHAR_STOP_WORDS][0];
			std::cout << "Reading " << stopWordsFile << '\n';
			std::ifstream	in(stopWordsFile);
			while( in )
			{
				CI_STRING word;
				in >> word;
				word.stripBlanks();
				if( !word.isEmpty() )
				{
					word.lowerCase();
					s_stopWords.addElement( word );
				}
			}
			in.close();
			std::ofstream 	fp( stopWordsFile );
			fp << s_stopWords;
			fp.close();
		}
		s_indexPath = cmdLine.parameter[CHAR_INDEX_PATH][0];
		s_indexPath.condAppend(DIRECTORY_DELIMITER);

		s_brainFile = cmdLine.parameter[CHAR_BRAIN_PATH][0];
		s_brainFile.condAppend(DIRECTORY_DELIMITER);
		s_brainFile += BRAIN_FILE;

		s_flags = cmdLine.flags;
		if( cmdLine.flags & OPT_WORD_DISTANCE )
		{
			s_wordDistance = getValueE<size_t>(cmdLine.parameter[CHAR_DISTANCE][0]);
			if( !s_wordDistance )
			{
				s_wordDistance = DEF_WORD_DISTANCE;
			}
		}
	}
	static void setArg( const STRING &path )
	{
		s_arg = path;
		s_arg.condAppend(DIRECTORY_DELIMITER);
	}

	void process( const STRING &file )
	{
		doEnterFunctionEx(gakLogging::llInfo,"ProcessorType<STRING>::process");
		doLogValueEx( gakLogging::llInfo, file );

		F_STRING	theName;
		STRING		indexFile;
		STRING		posFile;
		size_t		idx=0;
		MboxIndex	mboxIndex;
		Mails		theMails;
		std::cout << "process: " << file << std::endl;
		posFile = indexFile = s_indexPath;
		if( !s_arg.isEmpty() )
		{
			theName = file.subString( s_arg.strlen() );
		}
		if( theName.isEmpty() )
		{
			fsplit(file, NULL, &theName);
		}
		indexFile += theName + MBOX_INDEX_EXT;
		posFile += theName + MBOX_POS_EXT;

		if( !(s_flags & FLAG_FORCE) && !strAccess(indexFile,0) && !strAccess(posFile,0) )
		{
			DirectoryEntry theFileEntry(file);
			DirectoryEntry theIndexEntry(indexFile);

			if( theFileEntry.modifiedDate < theIndexEntry.modifiedDate )
			{
				DirectoryEntry thePosEntry(posFile);
				if( theFileEntry.modifiedDate < thePosEntry.modifiedDate )
				{
					return;
				}
			}
		}

		Array<int64> positions;
		loadMboxFile( file, theMails, &positions );
		s_mailCount += theMails.size();
		doLogValueEx(gakLogging::llInfo, s_mailCount );
		std::cout << "read " << theMails.size() << " Mails from " << file << std::endl;

		for( 
			Mails::iterator it = theMails.begin(), endIT = theMails.end();
			it != endIT;
			++it
		)
		{
			const MAIL &theMail = *it;
			STRING text = theMail.extractPlainText();
			if( !text.isEmpty() )
			{
				if( s_flags & FLAG_USE_META )
				{
					doLogValueEx(gakLogging::llDetail, it->from);
					text += it->from;
					doLogValueEx(gakLogging::llDetail, it->to);
					text += it->to;
					doLogValueEx(gakLogging::llDetail, it->subject);
					text += it->subject;
					text += it->date.getOriginalTime();
				}
				StringTokens tokens = tokenString( text, s_stopWords );
				StringIndex index = processPositions(text, tokens);
				MailIndexerPtr cmd = createIndexerCmd(theName, idx, index);
				g_IndexerPool->process(cmd);
				mboxIndex.mergeIndexPositions( idx, index );
				doLogValueEx(gakLogging::llInfo, mboxIndex.size() );

				if( s_flags & OPT_BRAIN_PATH )
				{
					LockGuard guard(s_locker);
					if( guard )
					{
						s_changed = true;
						s_Brain.learnFromTokens(text, tokens, s_wordDistance);
					}
				}
			}
			idx++;
			showProgress( idx, theMails.size() );
		}

		std::cout << "writing: " << indexFile << std::endl;
		makePath(indexFile);
		writeToBinaryFile( indexFile, mboxIndex, MBOX_INDEX_MAGIC, MBOX_INDEX_VERSION, ovmShortDown );
		std::cout << "writing: " << posFile << std::endl;
		makePath(posFile);
		writeToBinaryFile( posFile, positions, MBOX_POS_MAGIC, MBOX_POS_VERSION, ovmShortDown );
		std::cout << "found: " << theMails.size() << '/' << s_mailCount << " int " << file << std::endl;
	}
};

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

static CommandLine::Options options[] =
{
	{ CHAR_USE_META,		"meta",			0, 1, FLAG_USE_META, "include meta data in index" },
	{ CHAR_STOP_WORDS,		"stopWords",	0, 1, OPT_STOP_WORDS|CommandLine::needArg, "file with stop words" },
	{ CHAR_INDEX_PATH,		"indexPath",	1, 1, OPT_INDEX_PATH|CommandLine::needArg, "path where to store the index" },
	{ CHAR_BRAIN_PATH,		"brainPath",	0, 1, OPT_BRAIN_PATH|CommandLine::needArg, "path where to store the AI brain" },
	{ CHAR_THREAD_COUNT,	"threadCount",	0, 1, OPT_THREAD_COUNT|CommandLine::needArg, "number of threads (<32>)" },
	{ CHAR_FORCE,			"force",		0, 1, FLAG_FORCE, "force indexing" },
	{ CHAR_DISTANCE,		"maxDistance",	0, 1, OPT_WORD_DISTANCE|CommandLine::needArg, "max. word distance for AI (<3>)" },
	{ 0 }
};

// --------------------------------------------------------------------- //
// ----- class static data --------------------------------------------- //
// --------------------------------------------------------------------- //

size_t			ProcessorType<STRING>::s_mailCount = 0;
Set<CI_STRING>	ProcessorType<STRING>::s_stopWords;
STRING			ProcessorType<STRING>::s_arg;
STRING			ProcessorType<STRING>::s_indexPath;
Locker			ProcessorType<STRING>::s_locker;
Brain			ProcessorType<STRING>::s_Brain;
bool			ProcessorType<STRING>::s_changed = false;
STRING			ProcessorType<STRING>::s_brainFile;
int				ProcessorType<STRING>::s_flags = 0;
size_t			ProcessorType<STRING>::s_wordDistance = DEF_WORD_DISTANCE;

MailIndex		ProcessorType<MailIndexerPtr>::s_mailIndex;
bool			ProcessorType<MailIndexerPtr>::s_changed = false;
Locker			ProcessorType<MailIndexerPtr>::s_locker;


// --------------------------------------------------------------------- //
// ----- prototypes ---------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module functions ---------------------------------------------- //
// --------------------------------------------------------------------- //

static int mboxIndexer( const CommandLine &cmdLine )
{
	doEnterFunctionEx(gakLogging::llInfo, "mboxIndexer");
	int result = EXIT_SUCCESS;

	size_t threadCount = DEF_THREAD_COUNT;
	if( cmdLine.flags & OPT_THREAD_COUNT )
	{
		threadCount = getValueE<size_t>(cmdLine.parameter[CHAR_THREAD_COUNT][0]);
		if( !threadCount )
		{
			threadCount = DEF_THREAD_COUNT;
		}
	}
	ParalelDirScanner	theScanner("mboxIndexer", cmdLine,threadCount);

	ProcessorType<STRING>::init(cmdLine);

	MailIndex &index = ProcessorType<MailIndexerPtr>::s_mailIndex;
	STRING indexFile = ProcessorType<STRING>::s_indexPath + MAIL_INDEX_FILE;
	if( !(cmdLine.flags&FLAG_FORCE) && !strAccess( indexFile, 0 ) )
	{
		std::cout << "Reading index " << indexFile << std::endl;
		readFromBinaryFile( indexFile, &index, MAIL_INDEX_MAGIC, MAIL_INDEX_VERSION, false );
	}

	Brain &brain = ProcessorType<STRING>::s_Brain;
	const STRING &brainFile = ProcessorType<STRING>::s_brainFile;
	if( cmdLine.flags & OPT_BRAIN_PATH && !(cmdLine.flags&FLAG_FORCE) && !strAccess( brainFile, 0 ))
	{
		std::cout << "Reading brain " << brainFile << std::endl;
		readFromBinaryFile( brainFile, &brain, BRAIN_MAGIC, BRAIN_VERSION, false );
	}

	g_IndexerPool->start();
	for( int i=1; i<cmdLine.argc; ++i )
	{
		ProcessorType<STRING>::setArg(cmdLine.argv[i]);

		theScanner(cmdLine.argv[i]);
	}
	std::cout << "Mbox Indexer completed\nWaiting for full indexer " << g_IndexerPool->size() << std::endl;
	g_IndexerPool->flush();

	std::cout << __FILE__ << __LINE__ << ' ' << ProcessorType<STRING>::s_stopWords.size() << std::endl;
	ProcessorType<STRING>::s_stopWords.clear();

	doLogPositionEx( gakLogging::llInfo );
	if( ProcessorType<STRING>::s_changed )
	{
		doLogPositionEx( gakLogging::llInfo );
		std::cout << "writing: " << brainFile << std::endl;
		makePath(indexFile);
		writeToBinaryFile( brainFile, brain, BRAIN_MAGIC, BRAIN_VERSION, ovmShortDown );
		doLogPositionEx( gakLogging::llInfo );
		brain.clear();
		doLogPositionEx( gakLogging::llInfo );
	}
	else
	{
		doLogPositionEx( gakLogging::llInfo );
		std::cout << "Not writing: " << brainFile << std::endl;
	}

	if( ProcessorType<MailIndexerPtr>::s_changed )
	{
		std::cout << "writing: " << indexFile << std::endl;
		makePath(indexFile);
		writeToBinaryFile( indexFile, index, MAIL_INDEX_MAGIC, MAIL_INDEX_VERSION, ovmShortDown );

		std::cout << "Creating statistic" << std::endl;
		StatistikData sd = index.getStatistik();
		std::cout << "Writing statistic " << sd.size() << std::endl;
		std::size_t count = 0;
		std::ofstream	of(ProcessorType<STRING>::s_indexPath+"index.log" );
		for(
			StatistikData::const_iterator it = sd.cbegin(), endIT = sd.cend();
			it != endIT;
			++it
		)
		{
			++count;
			of << it->m_word << ' ' << it->m_count << '\n';
			showProgress( count, sd.size() );
		}
		doLogPositionEx( gakLogging::llInfo );
		sd.clear();
		doLogPositionEx( gakLogging::llInfo );
		index.clear();
		doLogPositionEx( gakLogging::llInfo );
	}
	return result;
}


// --------------------------------------------------------------------- //
// ----- class inlines ------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class constructors/destructors -------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class static functions ---------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class privates ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class protected ----------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class virtuals ------------------------------------------------ //
// --------------------------------------------------------------------- //
   
// --------------------------------------------------------------------- //
// ----- class publics ------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- entry points -------------------------------------------------- //
// --------------------------------------------------------------------- //

int main( int , const char *argv[] )
{
	doEnableLogEx( gakLogging::llInfo );
	doEnterFunctionEx(gakLogging::llInfo, "main");

	g_IndexerPool=new ThreadPool<MailIndexerPtr>(1,"MailIndexer");

	int result = EXIT_FAILURE;

	try
	{
		CommandLine cmdLine( options, argv );
		result = mboxIndexer( cmdLine );
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options> <mboxfile> ...\n" << options;
	}
	catch( std::exception &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << argv[0] << ": Unkown error" << std::endl;
	}

	return result;
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -b.
#	pragma option -a.
#	pragma option -p.
#endif

