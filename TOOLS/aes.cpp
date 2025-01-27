/*
		Project:		GAK_CLI
		Module:			aes.cpp
		Description:	encrypt a file using aes
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
#include <gak/aes.h>
#include <gak/gaklib.h>
#include <gak/string.h>
#include <gak/strFiles.h>

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

static const int AES_DO_ENCRYPT			= 0x10;
static const int AES_DO_DECRYPT			= 0x20;
static const int AES_HAS_KEY			= 0x40;
static const int AES_HAS_DESTINATION	= 0x80;

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

class MissingOptionError : public CmdlineError
{
	public:
	MissingOptionError()
	: CmdlineError( "missing -d and -e option" ) {}
};

class MissingFilesError : public CmdlineError
{
	public:
	MissingFilesError() 
	: CmdlineError( "No input files" ) {}
};

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

static gak::CommandLine::Options options[] =
{
	{ 'D', "decrypt",	0, 1, AES_DO_DECRYPT },
	{ 'E', "encrypt",	0, 1, AES_DO_ENCRYPT },
	{ 'C', "cypher",	0, 1, AES_HAS_KEY|CommandLine::needArg,	"<AES key>" },
	{ 'O', "output",	0, 1, AES_HAS_DESTINATION|CommandLine::needArg,	"<output file>" },
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

static int aes( const CommandLine &cmdLine )
{
	size_t			numFiles = 0;

	const char **argv = cmdLine.argv+1;
	const char *arg;
	int			flags = cmdLine.flags;

	if( !(flags & (AES_DO_DECRYPT|AES_DO_ENCRYPT)) )
/*@*/	throw MissingOptionError();


	STRING	cypher = flags & AES_HAS_KEY ? cmdLine.parameter['C'][0] : STRING();
	STRING	output = flags & AES_HAS_DESTINATION ? cmdLine.parameter['O'][0] : STRING();

	while( (arg = *argv++) != NULL )
	{
		STRING	source = arg;
		numFiles++;
		if( flags & AES_DO_ENCRYPT )
		{
			STRING	destination = (flags & AES_HAS_DESTINATION)
				? output
				: source + ".aes";
			if( flags & AES_HAS_KEY )
			{
				aesEncryptFile(
					cypher, source, destination
				);
			}
			else
			{
				aesEncryptFile(
					source, destination
				);
			}
		}
		else
		{
			STRING	destination = (flags & AES_HAS_DESTINATION)
				? output
				: source;

			if( !(flags & AES_HAS_DESTINATION) )
			{
				if( destination.endsWithI( ".aes" ) )
					destination.cut( destination.strlen()-4 );
				else
					destination += ".plain";
			}

			if( flags & AES_HAS_KEY )
			{
				aesDecryptFile(
					cypher, source, destination
				);
			}
			else
			{
				aesDecryptFile(
					source, destination
				);
			}
		}
	}

	if( !numFiles )
		throw MissingFilesError();

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

int main( int , const char *argv[] )
{
	int result = EXIT_FAILURE;

	try
	{
		CommandLine cmdLine( options, argv );
		result = aes( cmdLine );
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... <Source File> ... \n" << options;
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

