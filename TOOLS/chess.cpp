/*
		Project:		
		Module:			
		Description:	
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

// --------------------------------------------------------------------- //
// ----- includes ------------------------------------------------------ //
// --------------------------------------------------------------------- //

#include <gak/cmdlineParser.h>
#include <gak/chess.h>

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

const int OPT_BOARD		= 0x010;
const int OPT_MATE		= 0x020;
const int OPT_DEPTH		= 0x040;

const int CHAR_BOARD	= 'B';
const int CHAR_MATE		= 'M';
const int CHAR_DEPTH	= 'D';

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

static CommandLine::Options options[] =
{
	{ CHAR_BOARD,	"board", 1, 1, OPT_BOARD|CommandLine::needArg, "the chess board to start" },
	{ CHAR_MATE,	"mate",  1, 1, OPT_MATE|CommandLine::needArg,  "the max number of  moves till mate" },
	{ CHAR_DEPTH,	"depth", 1, 1, OPT_DEPTH|CommandLine::needArg,  "the search depth" },
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

static int chess( const CommandLine &cmdLine )
{
	int count = 0;
	chess::Board	theBoard;
	STRING			board = cmdLine.parameter[CHAR_BOARD][0];
	theBoard.generateFromString(board);
	theBoard.print();

	int maxMoves = cmdLine.parameter[CHAR_MATE][0].getValueE<unsigned>();
	int maxDepth = cmdLine.parameter[CHAR_DEPTH][0].getValueE<unsigned>();

	maxMoves *= 2;
	--maxMoves;

	if( theBoard.isBlackTurn() )
	{
		std::cout << ++count << ": ...";
	}

	while( maxMoves-- > 0 )
	{
		if( theBoard.isWhiteTurn() )
		{
			std::cout << ++count << ": ";
		}
		else
		{
			std::cout << ", ";
		}
		int quality;
		chess::Movement move = theBoard.findBest(maxDepth, &quality);
//		theBoard.print();
		if( !quality )
		{
			break;
		}
		if( move.promotionType != chess::Figure::ftNone )
		{
			theBoard.promote(move.fig, move.promotionType, move.dest );
		}
		else
		{
			theBoard.moveTo( move.fig, move.dest);
		}
		std::cout << move.toString();
		if( theBoard.isWhiteTurn() )
		{
			std::cout << std::endl;
		}

#if 0
		std::cout << std::endl;
		theBoard.print();
#endif
	}
	if( theBoard.isBlackTurn() )
	{
		std::cout << std::endl;
	}

	theBoard.print();
	return EXIT_SUCCESS;
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

int main( int , const char *_argv[] )
{
	int result = EXIT_FAILURE;

	doEnableLogEx(gakLogging::llInfo);
	doImmediateLog();
	doEnterFunctionEx(gakLogging::llInfo,"main");

#if 0
	const char *argv[] = {
		"chess.exe",
		"-B", 
				"     K  "
				"        "
				"     k  "
				"        "
				"        "
				"        "
				"d       "
				"        "
				"S",
		"-M", "1",
		"-D", "2",
		NULL
	};
#endif
#if 1
	const char *argv[] = {
		"chess.exe",
		"-B", 
				"    K   "
				"        "
				"        "
				"        "
				"        "
				"   D    "
				"   B    "
				"   k    "
				"W",
		"-M", "4",
		"-D", "4",
		NULL
	};
#endif

	try
	{
		CommandLine cmdLine( options, argv );
		result = ::chess( cmdLine );
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... <arg1> <arg2>\n" << options;
	}
	catch( std::exception &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << argv[0] << ": Unkown error" << std::endl;
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

