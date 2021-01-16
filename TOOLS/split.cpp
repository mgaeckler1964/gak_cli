/*
		Project:		GAK_CLI
		Module:			split.cpp
		Description:	split a file
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

/* --------------------------------------------------------------------- */
/* ----- includes ------------------------------------------------------ */
/* --------------------------------------------------------------------- */

#include <iostream>

#include <gak/cmdlineParser.h>
#include <gak/stdlib.h>
#include <gak/fmtNumber.h>

using namespace gak;

/* --------------------------------------------------------------------- */
/* ----- constants ----------------------------------------------------- */
/* --------------------------------------------------------------------- */

const int outputPresent		= 0x010;
const int numBytesPresent	= 0x020;

/* --------------------------------------------------------------------- */
/* ----- type definitions ---------------------------------------------- */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- macros -------------------------------------------------------- */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- prototypes ---------------------------------------------------- */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- imported datas ------------------------------------------------ */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- module statics ------------------------------------------------ */
/* --------------------------------------------------------------------- */

static gak::CommandLine::Options options[] =
{
	{ 'O', "output", 0, 1, outputPresent|CommandLine::needArg, "<output file>" },
	{ 'B', "bytes", 0, 1, numBytesPresent|CommandLine::needArg, "<number of bytes>" },
	{ 0 }
};

/* --------------------------------------------------------------------- */
/* ----- exported datas ------------------------------------------------ */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- module functions ---------------------------------------------- */
/* --------------------------------------------------------------------- */

static int split( const CommandLine &cmdLine )
{
	if( cmdLine.argc != 2 )
	{
		throw CmdlineError();
	}

	size_t	byteCount = cmdLine.flags & numBytesPresent ? cmdLine.parameter['B'][0].getValueE<size_t>() : 1024L*1024L*1024L;
	STRING	destName = cmdLine.flags & outputPresent ? cmdLine.parameter['O'][0] : STRING("split");
	STRING	fileName = cmdLine.argv[1];

	STDfile fpSrc( fileName, "rb" );
	if( fpSrc )
	{
		Buffer<void> fileBuffer( std::malloc( byteCount ) );
		if( fileBuffer )
		{
			unsigned i = 0;
			while( !feof( fpSrc ) )
			{
				STRING	dest = destName + formatNumber( ++i, 4 ) + ".spt";

				STDfile	fpDest( dest, "wb" );
				if( fpDest )
				{
					std::size_t tmpCount = fread( fileBuffer, 1, byteCount, fpSrc );
					fwrite( fileBuffer, 1, tmpCount, fpDest );

					if( tmpCount < byteCount )
						break;
				}
				else
				{
					throw OpenWriteError( dest ).addCerror();
				}
			}
		}
		else
		{
			throw AllocError();
		}
	}
	else
	{
		throw OpenReadError( fileName ).addCerror();
	}

	return EXIT_SUCCESS;
}

/* --------------------------------------------------------------------- */
/* ----- entry points -------------------------------------------------- */
/* --------------------------------------------------------------------- */

int main( int , const char *argv[] )
{
	int result = EXIT_FAILURE;

	try
	{
		CommandLine cmdLine( options, argv );
		result = split( cmdLine );
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... <file>\n" << options;
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
