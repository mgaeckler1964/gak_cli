/*
		Project:		GAK_CLI
		Module:			ls.cpp
		Description:	list content of a directory
		Author:			Martin Gäckler
		Address:		Hopfengasse 15, A-4020 Linz
		Web:			https://www.gaeckler.at/

		Copyright:		(c) 1988-2021 Martin Gäckler

		This program is free software: you can redistribute it and/or modify  
		it under the terms of the GNU General Public License as published by  
		the Free Software Foundation, version 3.

		You should have received a copy of the GNU General Public License 
		along with this program. If not, see <http://www.gnu.org/licenses/>.

		THIS SOFTWARE IS PROVIDED BY Martin Gäckler, Germany, Munich ``AS IS''
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

#include <iostream>
#include <iomanip>
#include <exception>

#include <gak/cmdlineParser.h>
#include <gak/directory.h>
#include <gak/fmtNumber.h>

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

const int LONG_LIST		= 0x010;
const int LINK_COUNT	= 0x020;
const int INODES		= 0x040;
const int SORT_OPTION	= 0x080;
const int ALL_ITEMS		= 0x100;

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
	{ 'A', "all",		0, 1, ALL_ITEMS },
	{ 'L', "long",		0, 1, LONG_LIST },
	{ 'C', "linkCount",	0, 1, LINK_COUNT },
	{ 'I', "inodes",	0, 1, INODES },
	{ 'S', "sort",		0, 1, SORT_OPTION|CommandLine::needArg,	"<sort option>" },
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

static inline void skipSpecial( int flags )
{
	if( flags & INODES )
		std::cout << std::setw( 21 ) << ' ';
	if( flags & LINK_COUNT )
		std::cout << std::setw( 3 ) << ' ';
}

static void showListing( int flags, const STRING &sort, const STRING &fPattern )
{
	DirectoryList	dList;

	dList.findFiles( fPattern );

	if( toupper(sort[0U]) == 'M' )
		dList.resort( DirectoryEntry::SORT_MODIFIED_DATE );
	else if( toupper(sort[0U]) == 'S' )
		dList.resort( DirectoryEntry::SORT_SIZE );
	else if( toupper(sort[0U]) == 'T' )
		dList.resort( DirectoryEntry::SORT_TYPE );

	for( 
		DirectoryList::iterator it = dList.begin(), endIT = dList.end();
		it != endIT;
		++it
	)
	{
		if( flags & ALL_ITEMS || !it->hidden )
		{
			if( flags & (LONG_LIST|INODES|LINK_COUNT) )
			{
				std::cout << (it->directory ? 'd' : '-');
				std::cout << (it->hidden    ? 'h' : '-');
				std::cout << ' ';

				std::cout << std::setw( 25 ) << it->fileName.convertToTerminal().leftString(25) << ' ';
				if( flags & (INODES|LINK_COUNT) )
				{
					if( !it->directory )
					{
						STRING	fileName = makeFullPath( fPattern, it->fileName );
						DirectoryEntry	entry;
						entry.findFile( fileName );

						if( flags & INODES )
							std::cout << std::setw( 20 ) << entry.fileID.fileIndex << ' ';
						if( flags & LINK_COUNT )
							std::cout << std::setw( 2 ) << entry.numLinks << ' ';
					}
					else
					{
						skipSpecial( flags );
					}
				}
				if( !(flags & INODES) )
					std::cout << formatNumber( it->fileSize, 15, ' ', ',' ) << ' ';
				std::cout << it->modifiedDate.getOriginalTime() << std::endl; 
			}
			else
			{
				std::cout << std::setw( 9 ) << it->fileName.convertToTerminal() << ' ';
			}
		}
	}
}

static int ls( const CommandLine &cmdLine )
{
	int 		flags = cmdLine.flags;
	const char	**argv = cmdLine.argv+1;
	const char	*arg;
	bool		needDefault = true;
	STRING		sortOption = flags & SORT_OPTION ? cmdLine.parameter['S'][0] : STRING();

	while( (arg = *argv++) != NULL )
	{
		needDefault = false;
		showListing( flags, sortOption, arg );
	}
	if( needDefault )
		showListing( flags, sortOption, ALL_FILES_PATTERN );

	return 0;
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
	int result = EXIT_FAILURE;

	try
	{
		CommandLine cmdLine( options, argv );
		result = ls( cmdLine );
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... <directory>\n" << options;
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

