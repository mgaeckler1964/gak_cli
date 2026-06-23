/*
		Project:		GAK_CLI
		Module:			tail.cpp
		Description:	print the last lines of a file
		Author:			Martin Gäckler
		Address:		Hofmannsthalweg 14, A-4030 Linz
		Web:			https://www.gaeckler.at/

		Copyright:		(c) 1988-2026 Martin Gäckler

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

#include <fstream>
#include <cstdlib>
#include <ctype.h>

#include <gak/cmdlineParser.h>
#include <gak/stdlib.h>
#include <gak/string.h>
#include <gak/Queue.h>
#include <gak/exception.h>

// --------------------------------------------------------------------- //
// ----- module switches ----------------------------------------------- //
// --------------------------------------------------------------------- //

#ifdef __BORLANDC__
#	pragma option -RT-
#	pragma option -a4
#	pragma option -pc
#endif

using namespace gak;

// --------------------------------------------------------------------- //
// ----- constants ----------------------------------------------------- //
// --------------------------------------------------------------------- //

static const int numLinesPresent = 0x010;
static const int numBytesPresent = 0x020;

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
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- imported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

static gak::CommandLine::Options options[] =
{
	{ 'N', "lines", 0, 1, numLinesPresent|CommandLine::needArg, "<number of lines>" },
	{ 'B', "bytes", 0, 1, numBytesPresent|CommandLine::needArg, "<number of bytes>" },
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

static void tailLines( size_t lineCount )
{
	Queue<STRING>	container;
	STRING			line;

	while( !feof( stdin ) )
	{
		line << stdin;
		container.push( line );
		while( container.size() > lineCount )
		{
			container.pop();
		}
	}

	while( container.size() )
	{
		line = container.pop();
		line >> stdout;
	}
}

static void tailBytes( size_t byteCount )
{
	size_t	readBytes;

	gak::Buffer<char> buffer1( byteCount );
	gak::Buffer<char> buffer2, tmp;

	while( !std::cin.eof() )
	{
		readBytes = size_t(std::cin.read(buffer1, byteCount).gcount());
		if( readBytes == byteCount )
		{
			if( !buffer2 )
			{
				buffer2.moveFrom( buffer1 );
				buffer1.resize( byteCount );
			}
			else
			{
				tmp.moveFrom( buffer2 );
				buffer2.moveFrom( buffer1 );
				buffer1.moveFrom( tmp );
			}
		}
		else
		{
			if( buffer2 )
			{
				if( readBytes )
				{
					memcpy( buffer2, buffer2+readBytes,byteCount-readBytes );
					memcpy( buffer2+byteCount-readBytes, buffer1, readBytes );
				}
				buffer1.moveFrom( buffer2 );
			}
			else
				byteCount = readBytes;

			break;
		}
	}

	std::cout.write( buffer1, byteCount );
}

static int tail( const CommandLine &cmdLine )
{
	int 		flags = cmdLine.flags;
	const char	**argv = cmdLine.argv+1;
	int			lineCount = flags & numLinesPresent ? cmdLine.parameter['N'][0].getValueE<unsigned>() : 25;
	int			byteCount = flags & numBytesPresent ? cmdLine.parameter['B'][0].getValueE<unsigned>() : 0;

	const char	*arg;

	STRING	fileName;

	while( (arg = *argv++) != NULL  )
	{
		fileName = arg;

		std::ifstream	fp( fileName, std::ios_base::binary );
		if( fp )
		{
			int		c;
			char	lineDelimiter=0;
			char	buffer[512];

			if( lineCount )
			{
				fp.seekg(0, std::ios_base::end );
				while( lineCount )
				{
					fp.seekg( -int(sizeof(buffer)), std::ios_base::cur );
					fp.read( buffer,sizeof(buffer) );
					fp.seekg( -int(sizeof(buffer)), std::ios_base::cur );
					for( int i=sizeof(buffer)-1; i>=0; i--, byteCount++ )
					{
						c = buffer[i];
						if( !lineDelimiter && (c=='\r' || c=='\n') )
							lineDelimiter = char(c);

						if( char(c) == lineDelimiter )
						{
							lineCount--;
							if( !lineCount )
								break;
						}
					}
				}
			}
			if( byteCount )
			{
				fp.seekg( -int(byteCount), std::ios_base::end );
				while( (c = fp.get()) != std::char_traits<char>::eof() )
				{
					if( (c != '\r' && c != '\n') )
						putchar( c );
					else if( c == lineDelimiter )
						putchar( '\n' );
				}
			}
		}
		else
		{
			throw OpenReadError( fileName ).addCerror();
		}
	}

	if( fileName.isEmpty() )
	{
		if( lineCount )
			tailLines( lineCount );
		else if( byteCount )
			tailBytes( byteCount );
	}

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
		result = tail( cmdLine );
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... <file> ...\n" << options;
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
#	pragma option -p.
#	pragma option -a.
#endif
