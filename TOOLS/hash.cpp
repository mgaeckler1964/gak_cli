/*
		Project:		GAK_CLI
		Module:			hash.cpp
		Description:	calculate the hash of a file
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

#include <gak/arrayFile.h>
#include <gak/cmdlineParser.h>
#include <gak/hash.h>
#include <gak/map.h>
#include <gak/directory.h>

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

static const int UPDATE			= 0x010;
static const int SILENT			= 0x020;
static const int HASH_FILE		= 0x040;
static const int EXECUTABLES	= 0x080;

static const uint32 magic = ('h'<<24) | ('a'<<16) | ('s'<<8) | 'h';

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

struct Digests
{
	MD5Hash::Digest		md5Digest;
	SHA224Hash::Digest	sha224Digest;
	SHA256Hash::Digest	sha256Digest;
	SHA384Hash::Digest	sha384Digest;
	SHA512Hash::Digest	sha512Digest;

	bool operator == ( const Digests &oper )
	{
		return md5Digest == oper.md5Digest
		&& sha224Digest == oper.sha224Digest
		&& sha256Digest == oper.sha256Digest
		&& sha384Digest == oper.sha384Digest
		&& sha512Digest == oper.sha512Digest;
	}
	bool operator != ( const Digests &oper )
	{
		return !(*this == oper);
	}
	void toBinaryStream( std::ostream &stream ) const
	{
		binaryToBinaryStream( stream, *this );
	}
	void fromBinaryStream( std::istream &stream )
	{
		binaryFromBinaryStream( stream, this );
	}
};

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
	{ 'U', "update",		0, 1, UPDATE },
	{ 'S', "silent",		0, 1, SILENT },
	{ 'E', "excecutables",	0, 1, EXECUTABLES },
	{ 'H', "hashFile",		0, 1, HASH_FILE|gak::CommandLine::needArg, "hash file" },
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

static void hash( 
	const STRING &fileName, bool silent, bool update, TreeMap<F_STRING, Digests> &allDigests 
)
{
	ArrayOfData	buffer;
	readFromFile( &buffer, fileName );

	Digests	digests;

	{
		MD5Hash	md5;

		md5.hash_data( buffer );

		digests.md5Digest = md5.getDigest();
	}
	{
		SHA224Hash	sha224;

		sha224.hash_data( buffer );

		digests.sha224Digest = sha224.getDigest();
	}
	{
		SHA256Hash	sha256;

		sha256.hash_data( buffer );

		digests.sha256Digest = sha256.getDigest();
	}
	{
		SHA384Hash	sha384;

		sha384.hash_data( buffer );

		digests.sha384Digest = sha384.getDigest();
	}
	{
		SHA512Hash	sha512;

		sha512.hash_data( buffer );

		digests.sha512Digest = sha512.getDigest();
	}

	if( !silent )
	{
		std::cout << fileName << ':' << std::endl;
		std::cout << "MD5   : " << digests.md5Digest << std::endl;
		std::cout << "SHA224: " << digests.sha224Digest << std::endl;
		std::cout << "SHA256: " << digests.sha256Digest << std::endl;
		std::cout << "SHA384: " << digests.sha384Digest << std::endl;
		std::cout << "SHA512: " << digests.sha512Digest << std::endl;
	}

	if( allDigests.hasElement( fileName ) )
	{
		if( allDigests[fileName] != digests )
		{
			std::cerr << "File has changed hash " << fileName << std::endl;
			if( update )
			{
				allDigests[fileName] = digests;
			}
		}
	}
	else
	{
		allDigests[fileName] = digests;
	}
}

static void hashDirectory( const STRING &directoryName, bool silent, bool update, bool executables, TreeMap<F_STRING, Digests> &allDigests )
{
	DirectoryList	dirList;
	F_STRING	path = directoryName;
	if( !path.endsWith( DIRECTORY_DELIMITER ) )
	{
		path += DIRECTORY_DELIMITER;
	}

	std::cout << std::left << std::setw( 70 ) << (directoryName.strlen() < 70 ? directoryName : directoryName.leftString( 67 ) + "...") << "   \r";
	std::cout.flush();

	try
	{
		dirList.dirlist( path );
	}
	catch( std::exception &e )
	{
		std::cerr << "Cannot read directory " << path << ": " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << "Cannot read directory " << path << std::endl;
	}
	for( 
		DirectoryList::const_iterator it = dirList.cbegin(), endIT = dirList.cend();
		it != endIT;
		++it
	)
	{
		if( it->fileName == "." || it->fileName == ".." )
			continue;

		F_STRING	fileName = path + it->fileName;

		if( isFile( fileName ) )
		{
			F_STRING	extension = fsplit( fileName );

			if( !executables 
			|| extension == "exe" || extension == "dll" || extension == "com" || extension == "sys" 
			|| extension == "cmd" || extension == "bat" 
			|| extension == "ttf" || extension == "fon" )
			{
				hash( fileName,  silent, update, allDigests );
			}
		}
		else
		{
			hashDirectory( fileName, silent, update, executables, allDigests );
		}
	}
}

static int hash( const CommandLine &cmdLine )
{
	const char	**argv = cmdLine.argv + 1;
	const char	*arg;
	F_STRING	fileArg;

	bool		silent = cmdLine.flags & SILENT;
	bool		update = cmdLine.flags & UPDATE;
	bool		executables = cmdLine.flags & EXECUTABLES;

	F_STRING	hashFile = cmdLine.flags & HASH_FILE ? cmdLine.parameter['H'][0] : NULL_STRING;

	TreeMap<F_STRING, Digests>	allDigests;

	if( !hashFile.isEmpty() && isFile( hashFile ) )
	{
		readFromFile( hashFile, &allDigests, magic );
	}

	while( (arg = *argv++) != NULL )
	{
		fileArg = fullPath( arg );

		if( isFile( fileArg ) )
		{
			hash( fileArg, silent, update, allDigests );
		}
		else
		{
			hashDirectory( fileArg, silent, update, executables, allDigests );
		}
	}

	if( fileArg.isEmpty() )
	{
		throw CmdlineError();
	}

	if( !hashFile.isEmpty() && allDigests.size() )
	{
		writeToFile( hashFile, allDigests, magic );
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

// "f:\Nikon Transfer\Cordobar" d:\mirror\NikonTransfer\Cordobar
// d:\mirror\NikonTransfer\Unsortiert\2014-02-15_18-37-10_10453.NEF "f:\Nikon Transfer\Unsortiert\2014-02-15_18-37-10_10453.NEF"
int main( int argc, const char *argv[] )
{
	int result = EXIT_FAILURE;

	try
	{
		CommandLine cmdLine( options, argv );
		result = hash( cmdLine );
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
#	pragma option -RT.
#	pragma option -b.
#	pragma option -a.
#	pragma option -p.
#endif
