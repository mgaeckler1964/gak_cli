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


// --------------------------------------------------------------------- //
// ----- switches ------------------------------------------------------ //
// --------------------------------------------------------------------- //

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
using gak::mail::Mails;

// --------------------------------------------------------------------- //
// ----- constants ----------------------------------------------------- //
// --------------------------------------------------------------------- //

static const int FLAG_USE_META		= 0x010;
static const int FLAG_STOP_WORDS	= 0x020;
static const int FLAG_INDEX_PATH	= 0x040;
static const int FLAG_BRAIN_PATH	= 0x080;
static const int FLAG_THREAD_COUNT	= 0x100;
static const int FLAG_FORCE			= 0x200;

static const char CHAR_USE_META		= 'M';
static const char CHAR_STOP_WORDS	= 'S';
static const char CHAR_INDEX_PATH	= 'I';
static const char CHAR_BRAIN_PATH	= 'B';
static const char CHAR_THREAD_COUNT	= 'T';
static const char CHAR_FORCE		= 'F';

static const size_t DEF_THREAD_COUNT = 32UL;

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

struct MailIndexerCmd
{
	STRING				theName;
	size_t				idx;
	gak::StringIndex	index;
	
	MailIndexerCmd() : idx(0) {}
	MailIndexerCmd(const STRING &theName,size_t idx, const gak::StringIndex &index) : theName(theName),idx(idx), index(index) {}
};


template <>
struct ProcessorType<MailIndexerCmd>
{
	typedef MailIndexerCmd object_type;

	static MailIndex		s_mailIndex;
	static bool				s_changed;
	static Locker			s_locker;

	void process( const MailIndexerCmd &cmd )
	{
		doEnterFunctionEx(gakLogging::llInfo,"ProcessorType<MailIndexerCmd>::mergeIndex");
		LockGuard guard( s_locker );

		if( guard )
		{
			s_mailIndex.mergeIndexPositions( MailAddress(cmd.theName, cmd.idx), cmd.index );
			s_changed = true;
		}
		else
		{
			doLogMessageEx( gakLogging::llError, "Cannot lock" );
		}
	}
};


static ThreadPool<MailIndexerCmd>	*g_IndexerPool=NULL;

template <>
struct ProcessorType<STRING>
{
	typedef STRING object_type;

	static size_t			s_mailCount;
	static Set<CI_STRING>	s_stopWords;
	static STRING			s_arg;
	static STRING			s_indexPath;

	static int				s_flags;

	static void init(const gak::CommandLine	&cmdLine)
	{
		doEnterFunctionEx(gakLogging::llInfo, "ProcessorType<STRING>::init");
		s_mailCount = 0;
		if( cmdLine.flags & FLAG_STOP_WORDS )
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
		s_flags = cmdLine.flags;
	}
	static void setArg( const gak::STRING &path )
	{
		s_arg = path;
		s_arg.condAppend(DIRECTORY_DELIMITER);
	}

	void process( const STRING &file )
	{
		doEnterFunctionEx(gakLogging::llInfo,"ProcessorType<STRING>::process");
		doLogValueEx( gakLogging::llInfo, file );

		F_STRING			theName;
		STRING				indexFile;
		STRING				posFile;
		size_t				idx=0;
		MboxIndex			mboxIndex;
		gak::mail::Mails	theMails;
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
		gak::mail::loadMboxFile( file, theMails, &positions );
		s_mailCount += theMails.size();
		doLogValueEx(gakLogging::llInfo, s_mailCount );
		std::cout << "read " << theMails.size() << " Mails from " << file << std::endl;

		gak::StopWatch	watch(true);

		for( 
			Mails::iterator it = theMails.begin(), endIT = theMails.end();
			it != endIT;
			++it
		)
		{
			const gak::mail::MAIL &theMail = *it;
			doLogPositionEx(gakLogging::llInfo);
			STRING text = theMail.extractPlainText();
			doLogPositionEx(gakLogging::llInfo);
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

				doLogPositionEx(gakLogging::llInfo);
				gak::StringIndex index = gak::indexString(text, s_stopWords);
				/// TODO: fix problem with mergeIndexPositions it does modifiy the index 8-(
				gak::StringIndex index2 = index; 
				doLogPositionEx(gakLogging::llInfo);
				mboxIndex.mergeIndexPositions( idx, index );
				doLogValueEx(gakLogging::llInfo, mboxIndex.size() );
				g_IndexerPool->process(MailIndexerCmd(theName, idx, index2));
				doLogPositionEx(gakLogging::llInfo);
			}
			idx++;
			if( watch.getMillis() > 1000 )
			{
				std::cout << idx << '/' << theMails.size() << "     \r";
				watch.start();
			}
		}

		std::cout << "writing: " << indexFile << std::endl;
		makePath(indexFile);
		writeToBinaryFile( indexFile, mboxIndex, MBOX_INDEX_MAGIC, MBOX_INDEX_VERSION, ovmShortDown );
		std::cout << "writing: " << posFile << std::endl;
		makePath(posFile);
		writeToBinaryFile( posFile, positions, MBOX_POS_MAGIC, MBOX_POS_VERSION, ovmShortDown );
		std::cout << "found: " << theMails.size() << '/' << s_mailCount << std::endl;
	}
};

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

static gak::CommandLine::Options options[] =
{
	{ CHAR_USE_META,		"meta",			0, 1, FLAG_USE_META, "include meta data in index" },
	{ CHAR_STOP_WORDS,		"stopWords",	0, 1, FLAG_STOP_WORDS|gak::CommandLine::needArg, "file with stop words" },
	{ CHAR_INDEX_PATH,		"indexPath",	1, 1, FLAG_INDEX_PATH|gak::CommandLine::needArg, "path where to store the index" },
	{ CHAR_BRAIN_PATH,		"brainPath",	0, 1, FLAG_BRAIN_PATH|gak::CommandLine::needArg, "path where to store the AI brain" },
	{ CHAR_THREAD_COUNT,	"threadCount",	0, 1, FLAG_THREAD_COUNT|gak::CommandLine::needArg, "number of threads (<32>)" },
	{ CHAR_FORCE,			"force",		0, 1, FLAG_FORCE, "force indexing" },
	{ 0 }
};

// --------------------------------------------------------------------- //
// ----- class static data --------------------------------------------- //
// --------------------------------------------------------------------- //

size_t			ProcessorType<STRING>::s_mailCount = 0;
Set<CI_STRING>	ProcessorType<STRING>::s_stopWords;
STRING			ProcessorType<STRING>::s_arg;
STRING			ProcessorType<STRING>::s_indexPath;

MailIndex		ProcessorType<MailIndexerCmd>::s_mailIndex;
bool			ProcessorType<MailIndexerCmd>::s_changed = false;
Locker			ProcessorType<MailIndexerCmd>::s_locker;

int				ProcessorType<STRING>::s_flags = 0;

// --------------------------------------------------------------------- //
// ----- prototypes ---------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module functions ---------------------------------------------- //
// --------------------------------------------------------------------- //

static int mboxIndexer( const gak::CommandLine &cmdLine )
{
	doEnterFunctionEx(gakLogging::llInfo, "mboxIndexer");
	int result = EXIT_SUCCESS;

	size_t threadCount = DEF_THREAD_COUNT;
	if( cmdLine.flags & FLAG_THREAD_COUNT )
	{
		threadCount = gak::getValueE<size_t>(cmdLine.parameter[CHAR_THREAD_COUNT][0]);
		if( !threadCount )
		{
			threadCount = DEF_THREAD_COUNT;
		}
	}
	gak::ParalelDirScanner	theScanner("mboxIndexer", cmdLine,threadCount);

	ProcessorType<STRING>::init(cmdLine);

	MailIndex &index = ProcessorType<MailIndexerCmd>::s_mailIndex;
	STRING indexFile = ProcessorType<STRING>::s_indexPath + MAIL_INDEX_FILE;
	if( !(cmdLine.flags&FLAG_FORCE) && !strAccess( indexFile, 0 ) )
	{
		std::cout << "Reading index" << std::endl;
		readFromBinaryFile( indexFile, &index, MAIL_INDEX_MAGIC, MAIL_INDEX_VERSION, false );
	}

	g_IndexerPool->start();
	for( int i=1; i<cmdLine.argc; ++i )
	{
		ProcessorType<STRING>::setArg(cmdLine.argv[i]);

		theScanner(cmdLine.argv[i]);
	}
	std::cout << "Mbox Indexer completed\nWaiting for full indexer" << std::endl;
	g_IndexerPool->flush();


	if( ProcessorType<MailIndexerCmd>::s_changed )
	{
		std::cout << "writing: " << indexFile << std::endl;
		makePath(indexFile);
		writeToBinaryFile( indexFile, index, MAIL_INDEX_MAGIC, MAIL_INDEX_VERSION, ovmShortDown );

		std::cout << "Creating statistic" << std::endl;
		StatistikData sd = index.getStatistik();
		std::cout << "Writing statistic " << sd.size() << std::endl;
		gak::StopWatch	watch(true);
		std::size_t count = 0;
		std::ofstream	of(ProcessorType<STRING>::s_indexPath+"index.log" );
		for(
			StatistikData::const_iterator it = sd.cbegin(), endIT = sd.cend();
			it != endIT;
			++it
		)
		{
			++count;
			if( watch.getMillis() > 1000 )
			{
				std::cout << count << '\r';
				watch.start();
			}
			of << it->m_word << ' ' << it->m_count << '\n';
		}
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
	g_IndexerPool=new ThreadPool<MailIndexerCmd>(1,"MailIndexer");

	doEnableLogEx( gakLogging::llInfo );
	doEnterFunctionEx(gakLogging::llInfo, "main");
	int result = EXIT_FAILURE;

	try
	{
		gak::CommandLine cmdLine( options, argv );
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

