/*
		Project:		GAK_CLI
		Module:			twins.cpp
		Description:	search for duplicate files
		Author:			Martin G�ckler
		Address:		Hopfengasse 15, A-4020 Linz
		Web:			https://www.gaeckler.at/

		Copyright:		(c) 1988-2021 Martin G�ckler

		This program is free software: you can redistribute it and/or modify  
		it under the terms of the GNU General Public License as published by  
		the Free Software Foundation, version 3.

		You should have received a copy of the GNU General Public License 
		along with this program. If not, see <http://www.gnu.org/licenses/>.

		THIS SOFTWARE IS PROVIDED BY Martin G�ckler, Germany, Munich ``AS IS''
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

#include <gak/directory.h>
#include <gak/map.h>
#include <gak/fileID.h>
#include <gak/hash.h>

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

// --------------------------------------------------------------------- //
// ----- constants ----------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

typedef	gak::TreeMap<gak::FileID, gak::ArrayOfStrings>	FileIdMap;

typedef gak::MD5Hash								Hash;
typedef Hash::Digest								Digest;
typedef gak::TreeMap<Digest, gak::ArrayOfStrings>	HashMap;

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class static data --------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- prototypes ---------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module functions ---------------------------------------------- //
// --------------------------------------------------------------------- //

static int twins( int argc, char *argv[] )
{
	gak::DirectoryList		content;
	FileIdMap				fileListById;
	HashMap					fileListByHash;

	std::cout << "Scanning drive\n";
	if( argc != 2 )
		throw std::exception( "usage: twins <directory>" );
	content.dirtree( argv[1], "*" );
	std::cout << "Found " << content.size() << " item(s)\n";

	std::cout << "Checking fileIDs\n";
	size_t	itemsProcessed = 0;
	for( 
		gak::DirectoryList::iterator it = content.begin(), endIT = content.end();
		it != endIT;
		++it
	)
	{
		if( ++itemsProcessed % 100 == 0 )
		{
			std::cout << itemsProcessed << " items processed\r";
			std::cout.flush();
		}
		if( !it->directory )
		{
			try
			{
				const gak::STRING	&fileName = it->fileName;

				{
					gak::FileID			theID = gak::getFileID( fileName );
					gak::ArrayOfStrings	&fileList = fileListById[theID];
					fileList.addElement( fileName );
				}
				{
					Hash				hash;
					hash.hash_file( fileName );
					gak::ArrayOfStrings	&fileList = fileListByHash[hash.getDigest()];
					fileList.addElement( fileName );
				}
			}
			catch( std::exception &e )
			{
				std::cerr << e.what() << std::endl;
			}
		}
	}
	std::cout << std::endl;

	for(
		gak::TreeMap<gak::FileID, gak::ArrayOfStrings>::iterator it = fileListById.begin(), endIT = fileListById.end();
		it != endIT;
		++it
	)
	{
		const gak::ArrayOfStrings	&fileList = it->getValue();
		if( fileList.size() > 1 )
		{
			std::cout << it->getKey() << std::endl;
			for(
				gak::ArrayOfStrings::const_iterator it = fileList.cbegin(), endIT = fileList.cend();
				it != endIT;
				++it
			)
			{
				std::cout << it->convertToTerminal() << std::endl;
			}
		}
	}

	for(
		HashMap::iterator it = fileListByHash.begin(), endIT = fileListByHash.end();
		it != endIT;
		++it
	)
	{
		const gak::ArrayOfStrings	&fileList = it->getValue();
		if( fileList.size() > 1 )
		{
			std::cout << "identical files:\n";
			for(
				gak::ArrayOfStrings::const_iterator it = fileList.cbegin(), endIT = fileList.cend();
				it != endIT;
				++it
			)
			{
				std::cout << it->convertToTerminal() << std::endl;
			}
		}
	}

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

int main( int argc, char *argv[] )
{
	int result;

	try
	{
		result = twins( argc, argv );
	}
	catch( std::exception &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		result = -1;
	}

	return result;
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -b.
#	pragma option -a.
#	pragma option -p.
#endif


