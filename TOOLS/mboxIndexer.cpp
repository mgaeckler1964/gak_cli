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
#include <gak/mboxParser.h>
#include <gak/indexer.h>

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

const int FLAG_USE_META		= 0x10;
const int FLAG_STOP_WORDS	= 0x20;
const int FLAG_INDEX_PATH	= 0x40;
const int FLAG_BRAIN_PATH	= 0x80;

const uint32 MBOX_INDEX_MAGIC	= 0x19901993;
const uint16 MBOX_INDEX_VERSION	= 0x1;

const uint32 MAIL_INDEX_MAGIC	= 0x19641964;
const uint16 MAIL_INDEX_VERSION	= 0x1;

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

struct MailAddress
{
	STRING		mboxFile;
	size_t		mailIndex;

	MailAddress(const STRING &mboxFile="", size_t mailIndex=0) : mboxFile(mboxFile), mailIndex(mailIndex) {}

	int compare( const MailAddress &other ) const
	{
		int result = gak::compare( mboxFile, other.mboxFile );
		return result ? result : gak::compare( mailIndex, other.mailIndex );
	}
	void toBinaryStream ( std::ostream &stream ) const
	{
		gak::toBinaryStream( stream, mboxFile );
		gak::toBinaryStream( stream, uint64(mailIndex) );
	}
	void fromBinaryStream ( std::istream &stream )
	{
		gak::fromBinaryStream( stream, &mboxFile );
		uint64 mailIndex;
		gak::fromBinaryStream( stream, &mailIndex );
		this->mailIndex = size_t(mailIndex);
	}
};

typedef gak::Index<size_t>			MboxIndex;
typedef gak::Index<MailAddress>		MailIndex;
typedef MboxIndex::RelevantHits		MboxSearchResult;

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
	static Locker			s_locker;

	void process( const MailIndexerCmd &cmd )
	{
		doEnterFunctionEx(gakLogging::llInfo,"ProcessorType<MailIndexerCmd>::mergeIndex");
		LockGuard guard( s_locker );

		if( guard )
		{
			s_mailIndex.mergeIndexPositions( MailAddress(cmd.theName, cmd.idx), cmd.index );
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
			STRING stopWordsFile = cmdLine.parameter['S'][0];
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
		s_indexPath = cmdLine.parameter['I'][0];
		if( !s_indexPath.endsWith(DIRECTORY_DELIMITER) )
		{
			s_indexPath += DIRECTORY_DELIMITER;
		}
		s_flags = cmdLine.flags;
	}
	static void setArg( const gak::STRING &path )
	{
		s_arg = path;
		if( !s_arg.endsWith(DIRECTORY_DELIMITER) )
		{
			s_arg += DIRECTORY_DELIMITER;
		}
	}

	void process( const STRING &file )
	{
		doEnterFunctionEx(gakLogging::llInfo,"ProcessorType<STRING>::process");
		doLogValueEx( gakLogging::llInfo, file );

		F_STRING			theName;
		STRING				indexFile;
		size_t				idx=0;
		MboxIndex			mboxIndex;
		gak::mail::Mails	theMails;
		std::cout << "process: " << file << std::endl;
		indexFile = s_indexPath;
		if( !s_arg.isEmpty() )
		{
			theName = file.subString( s_arg.strlen() );
		}
		if( theName.isEmpty() )
		{
			fsplit(file, NULL, &theName);
		}
		indexFile += theName + ".mboxIdx";

		gak::mail::loadMboxFile( file, theMails );
		s_mailCount += theMails.size();
		doLogValueEx(gakLogging::llInfo, s_mailCount );
		std::cout << "read " << theMails.size() << " Mails" << std::endl;

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
			std::cout << idx << '\r';
		}

		std::cout << "writing: " << indexFile << std::endl;
		makePath(indexFile);
		writeToBinaryFile( indexFile, mboxIndex, MBOX_INDEX_MAGIC, MBOX_INDEX_VERSION, ovmShortDown );
		std::cout << "found: " << theMails.size() << '/' << s_mailCount << std::endl;
	}
};

size_t			ProcessorType<STRING>::s_mailCount = 0;
Set<CI_STRING>	ProcessorType<STRING>::s_stopWords;
STRING			ProcessorType<STRING>::s_arg;
STRING			ProcessorType<STRING>::s_indexPath;

MailIndex		ProcessorType<MailIndexerCmd>::s_mailIndex;
Locker			ProcessorType<MailIndexerCmd>::s_locker;

int				ProcessorType<STRING>::s_flags = 0;

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

static gak::CommandLine::Options options[] =
{
	{ 'M', "meta",	0, 1, FLAG_USE_META, "include meta data in index calculation" },
	{ 'S', "stopWords",	0, 1, FLAG_STOP_WORDS|gak::CommandLine::needArg, "file with stop words" },
	{ 'I', "indexPath",	1, 1, FLAG_INDEX_PATH|gak::CommandLine::needArg, "path where to store the index" },
	{ 'B', "brainPath",	0, 1, FLAG_BRAIN_PATH|gak::CommandLine::needArg, "path where to store the AI brain" },
	{ 0 }
};

// --------------------------------------------------------------------- //
// ----- class static data --------------------------------------------- //
// --------------------------------------------------------------------- //

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

	gak::ParalelDirScanner	theScanner("mboxIndexer", cmdLine,32);

	ProcessorType<STRING>::init(cmdLine);
	g_IndexerPool->start();

	for( int i=1; i<cmdLine.argc; ++i )
	{
		ProcessorType<STRING>::setArg(cmdLine.argv[i]);

		theScanner(cmdLine.argv[i]);
	}
	std::cout << "Mbox Indexer completed\nWaiting for full indexer" << std::endl;
	g_IndexerPool->flush();

	const MailIndex &index = ProcessorType<MailIndexerCmd>::s_mailIndex;
	
	STRING indexFile = ProcessorType<STRING>::s_indexPath + ".mailIndex";

	std::cout << "writing: " << indexFile << std::endl;
	makePath(indexFile);
	writeToBinaryFile( indexFile, index, MAIL_INDEX_MAGIC, MAIL_INDEX_VERSION, ovmShortDown );

	std::cout << "writing statistic" << std::endl;
	StatistikData sd = index.getStatistik();
	std::ofstream	of(ProcessorType<STRING>::s_indexPath+"index.log" );
	for(
		StatistikData::const_iterator it = sd.cbegin(), endIT = sd.cend();
		it != endIT;
		++it
	)
	{
		of << it->m_word << ' ' << it->m_count << std::endl;
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

