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

#define USE_PAIR_MAP	1		// for searching pair map is better than TreeMap that is faster for indexing

// --------------------------------------------------------------------- //
// ----- includes ------------------------------------------------------ //
// --------------------------------------------------------------------- //
/*
#include <gak/numericString.h>
#include <gak/directoryEntry.h>
#include <gak/strFiles.h>
*/

#include <gak/indexer.h>
#include <gak/cmdlineParser.h>
#include <gak/mboxParser.h>
#include <gak/exception.h>

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

static const int FLAG_INDEX_PATH	= 0x010;
static const int FLAG_BRAIN_PATH	= 0x020;
static const int FLAG_MBOX_PATH		= 0x040;

static const char CHAR_INDEX_PATH	= 'I';
static const char CHAR_BRAIN_PATH	= 'B';
static const char CHAR_MBOX_PATH	= 'M';

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //


// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

static gak::CommandLine::Options options[] =
{
	{ CHAR_INDEX_PATH,	"indexPath",	1, 1, FLAG_INDEX_PATH|gak::CommandLine::needArg, "path where to find the index" },
	{ CHAR_BRAIN_PATH,	"brainPath",	0, 1, FLAG_BRAIN_PATH|gak::CommandLine::needArg, "path where to store the AI brain" },
	{ CHAR_MBOX_PATH,	"mboxPath",		1, -1, FLAG_MBOX_PATH|gak::CommandLine::needArg, "path where to find the mbox files" },
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

static void mboxSearch( const gak::CommandLine &cmdLine )
{
	doEnterFunctionEx(gakLogging::llInfo, "mboxInmboxSearchdexer");

	MailIndex index;

	STRING indexPath = cmdLine.parameter[CHAR_INDEX_PATH][0];
	indexPath.condAppend(DIRECTORY_DELIMITER);
	STRING indexFile = indexPath + MAIL_INDEX_FILE;
	if( strAccess( indexFile, 0 ) )
	{
		throw gak::OpenReadError(indexFile);
	}

	std::cout << "Reading " << indexFile << std::endl;
	readFromBinaryFile( indexFile, &index, MAIL_INDEX_MAGIC, MAIL_INDEX_VERSION, true );

	std::cout << "Searching in " << indexFile << std::endl;

	const char *argv;
	for( int i=1; (argv = cmdLine.argv[i]) != NULL; ++i )
	{
		std::cout << "\nSearching for " << argv << std::endl;
		MailRelevantHits	result = index.getRelevantHits(argv, false, true, false);
		std::cout << "Found " << result.size() << "items" << std::endl;
		size_t count=0;
		for(
			MailRelevantHits::const_iterator it = result.cbegin(), endIT = result.cend();
			endIT != it && ++count < 5;
			++it
		)
		{
			gak::mail::MAIL	theMail;
			STRING			mboxFile;
			STRING			mboxPositionsFile;

			std::cout << '\n' << it->m_source.mboxFile << std::endl;
			for(
				ArrayOfStrings::const_iterator it2 = cmdLine.parameter[CHAR_MBOX_PATH].cbegin(), endIT2 = cmdLine.parameter[CHAR_MBOX_PATH].cend();
				it2 != endIT2;
				++it2
			)
			{
				mboxFile = *it2;
				mboxFile.condAppend(DIRECTORY_DELIMITER);
				mboxFile += it->m_source.mboxFile;

				mboxPositionsFile = indexPath;
				mboxPositionsFile.condAppend(DIRECTORY_DELIMITER);
				mboxPositionsFile += it->m_source.mboxFile;
				mboxPositionsFile += MBOX_POS_EXT;

				if( !strAccess( mboxFile, 0 ) && !strAccess( mboxPositionsFile, 0 ) )
				{
					break;
				}
			}
			if( !strAccess( mboxFile, 0 ) && !strAccess( mboxPositionsFile, 0 ) )
			{
				Array<int64>	positions;
				readFromBinaryFile( mboxPositionsFile, &positions, MBOX_POS_MAGIC, MBOX_POS_VERSION, false );

				loadMail( mboxFile, it->m_source.mailIndex, positions, &theMail );
				std::cout << "From:    " << theMail.from << std::endl;
				std::cout << "To:      " << theMail.to << std::endl;
				std::cout << "Date:    " << theMail.date.getLocalTime() << std::endl;
				std::cout << "Subject: " << theMail.subject << std::endl;
			}
			else
			{
				std::cout << "Mailbox " << it->m_source.mboxFile << " not found." << std::endl;
			}
		}
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
	doEnableLogEx( gakLogging::llInfo );
	doEnterFunctionEx(gakLogging::llInfo, "main");
	int result = EXIT_FAILURE;

	try
	{
		gak::CommandLine cmdLine( options, argv );
		mboxSearch( cmdLine );
		result = EXIT_SUCCESS;
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

