/*
		Project:		GAK_CLI
		Module:			treeSize.cpp
		Description:	check size of directories
		Author:			Martin Gäckler
		Address:		Hofmannsthalweg 14, A-4030 Linz
		Web:			https://www.gaeckler.at/

		Copyright:		(c) 1988-2025 Martin Gäckler

		This program is free software: you can redistribute it and/or modify  
		it under the terms of the GNU General Public License as published by  
		the Free Software Foundation, version 3.

		You should have received a copy of the GNU General Public License 
		along with this program. If not, see <http://www.gnu.org/licenses/>.

		THIS SOFTWARE IS PROVIDED BY Martin Gäckler, Linz, Austria ``AS IS''
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

// --------------------------------------------------------------------- //
// ----- includes ------------------------------------------------------ //
// --------------------------------------------------------------------- //

#include <gak/logfile.h>
#include <gak/cmdlineParser.h>
#include <gak/dirScanner.h>
#include <gak/map.h>
#include <gak/mboxParser.h>
#include <gak/stopWatch.h>

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

// --------------------------------------------------------------------- //
// ----- constants ----------------------------------------------------- //
// --------------------------------------------------------------------- //

const int FLAG_MAIL_REPORT		= 0x010;
const int FLAG_FOLLOW_REPARSE	= 0x020;
const int FLAG_VERBOSE			= 0x040;
const int FLAG_UPDATE			= 0x080;

const int CHAR_MAIL_REPORT		= 'M';
const int CHAR_FOLLOW_REPARSE	= 'X';
const int CHAR_VERBOSE			= 'V';
const int CHAR_UPDATE			= 'U';

static const uint32 magic = ('D'<<24) | ('i'<<16) | ('T'<<8) | 'r';
static const uint16 version = 1;

#define TREE_SIZE	".treeSize"
#define UPDATE		".update"

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

struct TreeEntry
{
	typedef STRING key_type;

	STRING			m_name;
	uint64			m_treeSize;
	uint64			m_fileCount;
	uint64			m_dirCount;

	const STRING &getKey() const
	{
		return m_name;
	}
	TreeEntry( const STRING &name=nullptr) : m_name(name), m_treeSize(0), m_fileCount(0), m_dirCount(0)
	{}

	void toBinaryStream( std::ostream &stream ) const
	{
		m_name.toBinaryStream( stream );
		binaryToBinaryStream( stream, m_treeSize );
		binaryToBinaryStream( stream, m_fileCount );
		binaryToBinaryStream( stream, m_dirCount );
	}
	void fromBinaryStream( std::istream &stream )
	{
		m_name.fromBinaryStream( stream );
		binaryFromBinaryStream( stream, &m_treeSize );
		binaryFromBinaryStream( stream, &m_fileCount );
		binaryFromBinaryStream( stream, &m_dirCount );
	}
};

struct ScanningDirectory : public TreeEntry
{
	size_t			m_parentIdx;
	ScanningDirectory( const STRING &name=nullptr, size_t parentIdx=0 ) : TreeEntry(name), m_parentIdx(parentIdx)
	{}
};


class TheProcessor : public FileProcessor
{
	Array<ScanningDirectory>	m_directoryTree;
	Stack<size_t>				m_runningItems;
	size_t						m_currentItem;
	size_t						m_errorCount;

	public:
	TheProcessor(const CommandLine	&cmdLine) : FileProcessor(cmdLine), m_currentItem(m_directoryTree.no_index), m_errorCount(0)
	{
	}
	~TheProcessor()
	{
	}

	void start( const STRING &path )
	{
		if( m_cmdLine.flags & FLAG_VERBOSE )
		{
			static StopWatch	sw(true);
			if( sw.getMillis() > 1000 )
			{
				std::cout << path << std::endl;
				sw.start();
				assert( sw.getMillis() < 1000 );
			}
		}

		size_t	currentItem = m_currentItem;
		while( currentItem != m_directoryTree.no_index )
		{
			ScanningDirectory &cur = m_directoryTree[currentItem];
			cur.m_dirCount++;
			currentItem = cur.m_parentIdx;
		}

		m_runningItems.push( m_currentItem );
		m_directoryTree.addElement(ScanningDirectory(path,m_currentItem));
		m_currentItem = m_directoryTree.size() -1;
	}
	void process( const DirectoryEntry &theEntry, const STRING &file )
	{
		try
		{
			uint64			size = theEntry.fileSize;

			size_t	currentItem = m_currentItem;
			while( currentItem != m_directoryTree.no_index )
			{
				ScanningDirectory &cur = m_directoryTree[currentItem];
				cur.m_fileCount++;
				cur.m_treeSize += size;
				currentItem = cur.m_parentIdx;
			}
		}
		catch( ... )
		{
			m_errorCount++;
		}
	}
	void end( const STRING &path )
	{
		m_currentItem = m_runningItems.pop();
	}
	const Array<ScanningDirectory> &tree()
	{
		return m_directoryTree;
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
	{ CHAR_MAIL_REPORT,		"fatalMail",		0, 1,				FLAG_MAIL_REPORT,					"send mail in case of extreme size change" },
	{ CHAR_FOLLOW_REPARSE,	"followReparse",	0, 1,				FLAG_FOLLOW_REPARSE,				"follow reparse points" },
	{ CHAR_VERBOSE,			"verbose",			0, 1,				FLAG_VERBOSE,						"list directory stats" },
	{ CHAR_UPDATE,			"update",			0, unsigned(-1),	FLAG_UPDATE|CommandLine::needArg,	"<dir> update database entry" },
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

static void treeSize( const CommandLine &cmdLine, const STRING &root )
{
	doEnterFunctionEx(gakLogging::llInfo, "treeSize( const CommandLine &cmdLine )" );
	StopWatch sw( true );
	DirectoryScanner<TheProcessor> theScanner(cmdLine);

	bool hasChanged = false;
	bool doList = true;

	std::cout << "scanning tree " << root << ' ' << sw.get<Minutes<> >().toString() << std::endl;
	theScanner( root, "", IGNORE_ERROR );

	STRING sizeFile = root;
	sizeFile.condAppend(DIRECTORY_DELIMITER);
	sizeFile +=  TREE_SIZE; 
	STRING updateFile = sizeFile;
	updateFile += UPDATE;
	ArrayOfStrings updateList;

	if( !exists( updateFile ) )
	{
		updateFile = getGlobalConfig() + DIRECTORY_DELIMITER_STRING TREE_SIZE UPDATE;
	}
	if( exists( updateFile ) )
	{
		updateList.readFromFile(updateFile);
	}
	size_t numFileUpdates = updateList.size();

	Map<TreeEntry>	theTreeSize;

	if( isFile(sizeFile) )
	{
		std::cout << "reading database " <<sizeFile << ' ' << sw.get<Minutes<> >().toString() << std::endl;
		readFromBinaryFile( sizeFile, &theTreeSize, magic, version, cmdLine.flags & FLAG_FOLLOW_REPARSE );
	}

	const Array<ScanningDirectory> &tree = theScanner.processor().tree();
	theTreeSize.setChunkSize( tree.size() );
	size_t i=0;
	for(
		Array<ScanningDirectory>::const_iterator it = tree.cbegin(), endIT = tree.cend();
		it != endIT;
		++it
	)
	{
		gakLogging::doShowProgress( 0, ++i, tree.size() );
		if( doList )
		{
			std::cout << it->m_name << ' ' << it->m_fileCount << " files " << it->m_dirCount << " directories " << it->m_treeSize << " bytes" << std::endl;
			if( !(cmdLine.flags & FLAG_VERBOSE) )
			{
				doList = false;
			}
		}

		size_t idx = theTreeSize.getElementIndex(it->m_name);
		if( idx != theTreeSize.no_index )
		{
			STRING warningText;
			TreeEntry &old = theTreeSize.getElementAt(idx);
			if( old.m_treeSize > 1024*1024 && old.m_treeSize < it->m_treeSize/2 )
				warningText += "more bytes\n";
			if( old.m_fileCount > 64 && old.m_fileCount < it->m_fileCount/2 )
				warningText += "more files\n";
			if( old.m_dirCount > 64 && old.m_dirCount < it->m_dirCount/2 )
				warningText += "more directories\n";

			if( !warningText.isEmpty() )
			{
				STRING messageText = it->m_name + '\n' + 
					formatNumber(it->m_fileCount) + " files " + formatNumber(it->m_dirCount) + " directories " + formatNumber(it->m_treeSize, 0, 0, ' ') + " bytes\n"
					" (old)" + formatNumber(old.m_fileCount) + " files " + formatNumber(old.m_dirCount) + " directories " + formatNumber(old.m_treeSize, 0, 0, ' ') + " bytes\n" +
					warningText;

				if( (cmdLine.flags & FLAG_UPDATE) || updateList.hasElement(it->m_name) )
				{
					if( updateList.hasElement(it->m_name) || cmdLine.parameter[CHAR_UPDATE].hasElement(it->m_name) ) 
					{
						hasChanged = true;
						old.m_dirCount = it->m_dirCount;
						old.m_fileCount = it->m_fileCount;
						old.m_treeSize = it->m_treeSize;
						updateList.removeElementVal(it->m_name);
						messageText += "\nupdating.";
					}
					else
						messageText += "\nNo updating.";
				}

				std::cout << messageText << std::endl;

				if( cmdLine.flags & FLAG_MAIL_REPORT )
					mail::appendMail("treeSize warning",messageText);

			}
		}
		else
		{
			theTreeSize.addElement(*it);
			hasChanged = true;
		}
	}

	if(hasChanged)
	{
		std::cout << "writing database " <<sizeFile << ' ' << sw.get<Minutes<> >().toString() << std::endl;

		writeToBinaryFile( sizeFile, theTreeSize, magic, version, owmOverwrite );
	}
	if( numFileUpdates != updateList.size() )
	{
		if( updateList.size() )
			updateList.writeToFile( updateFile );
		else
			strRemove( updateFile );
	}

	std::cout << "ready " << sw.get<Minutes<> >().toString() << std::endl;
}

static void treeSize( const CommandLine &cmdLine )
{
	const char **argv = cmdLine.argv;
	while( *++argv )
	{
		treeSize( cmdLine, *argv );
	}
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
	int		result = EXIT_FAILURE;
	STRING	errorMessage;
	bool	mailReport = false;

	gakLogging::enableDefaultProgressShow();
	doDisableLog();
	//doEnableLogEx(gakLogging::llInfo);
	//doEnableProfile(gakLogging::llDetail);
	//doImmediateLog();
	doEnterFunctionEx(gakLogging::llInfo,"main");

	try
	{
		CommandLine cmdLine( options, argv );
		mailReport = cmdLine.flags & FLAG_MAIL_REPORT;

		treeSize( cmdLine );
		result = EXIT_SUCCESS;
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... <Source Path> <Destination Path>\n" << options;
	}
	catch( std::exception &e )
	{
		errorMessage = e.what();
	}
	catch( ... )
	{
		errorMessage = "Unkown error";
	}
	if( !errorMessage.isEmpty() )
	{
		std::cerr << argv[0] << ": " << errorMessage << std::endl;
		if( mailReport )
		{
			mail::appendMail( STRING(argv[0])+ " error", errorMessage );
		}
	}

	doFlushLogs();
	doDisableLog();
	return result;
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -b.
#	pragma option -a.
#	pragma option -p.
#endif
