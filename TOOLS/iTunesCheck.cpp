/*
		Project:		GAK_CLI
		Module:			iTunesCheck.cpp
		Description:	check an iTunes database for errors
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

#include <errno.h>

#include <gak/set.h>
#include <gak/string.h>
#include <gak/wideString.h>
#include <gak/fieldSet.h>
#include <gak/xmlParser.h>
#include <gak/strFiles.h>
#include <gak/html.h>
#include <gak/directory.h>
#include <gak/numericString.h>
#include <gak/http.h>

// --------------------------------------------------------------------- //
// ----- imported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

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

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

static inline void showError( 
	const char *error, 
	const STRING &genre, const STRING &artist, 
	const STRING &record, const STRING &song, 
	const STRING &location 
)
{
	std::cout 
		<< error << ": "
		<< genre.convertToTerminal() << ':'
		<< artist.convertToTerminal() << ':'
		<< record.convertToTerminal() << ':'
		<< song.convertToTerminal() << ':'
		<< location.convertToTerminal() << std::endl
	;
}

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

// --------------------------------------------------------------------- //
// ----- class static data --------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- prototypes ---------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module functions ---------------------------------------------- //
// --------------------------------------------------------------------- //

static void readSongList( xml::Document *theTemplate )
{
	size_t		numSongs, numErrors, fileSize;
	STRING		record, genre, artist, song, location, tmp;
	struct stat	statBuff;

	numSongs = numErrors = 0;
	Set<CI_STRING>	artists;

	xml::Element	*plist = theTemplate->getElement( "plist" );
	if( plist )
	{
		xml::Element *outerDict = plist->getElement( "dict" );
		if( outerDict )
		{
			xml::Element *innerDict = outerDict->getElement( "dict" );
			if( innerDict )
			{
				size_t	numElements1 = innerDict->getNumObjects();
				for( size_t i=0; i<numElements1; i++ )
				{
					record = NULL_STRING;
					artist = NULL_STRING;
					genre = NULL_STRING;
					song = NULL_STRING;
					location = NULL_STRING;

					xml::Element	*recordDict = innerDict->getElement( i );
					if( recordDict && recordDict->getTag() == "dict" )
					{
						numSongs++;
						size_t	numElements2 = recordDict->getNumObjects();
						for( size_t j=0; j<numElements2 && (!genre[0U] || !artist[0U] || !record[0U] || !song[0U] || !location[0U]); j += 2 )
						{
							xml::Element	*key = recordDict->getElement(j);
							xml::Element	*value = recordDict->getElement(j+1);
							if( key
							&& value
							&& key->getTag() == "key" )
							{
								if( value->getTag() == "string" )
								{
									if( key->getValue( xml::PLAIN_MODE ) == "Album" )
										record = value->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Artist" )
										artist = value->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Genre" )
										genre = value->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Name" )
										song = value->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Location" )
										location = value->getValue( xml::PLAIN_MODE );
								}
								else if( value->getTag() == "integer" )
								{
									if( key->getValue( xml::PLAIN_MODE ) == "Size" )
									{
										tmp = value->getValue( xml::PLAIN_MODE );
										fileSize = tmp.getValueE<size_t>();
									}
								}
							}
						}
						if( !location.isEmpty() )
						{
							if( location.beginsWith( "file://localhost" ) )
								location += size_t(16);
#ifdef _Windows
							if( location[0U] == '/' && location[2U] == ':' )
								location += size_t(1);
#endif

							location = net::webUnEscape( location, true );
							location.setCharSet( STR_UTF8 );

							if( '/' != DIRECTORY_DELIMITER )
								location.replaceChar( '/', DIRECTORY_DELIMITER );

							if( !exists( location ) )
							{
#ifdef _Windows
								location = location.decodeUTF8();
#endif
								showError( 
									"Missing file",
									genre, artist, record, song,
									location
								);
								numErrors++;
							}
							else
							{
								strStat( location, &statBuff );
								if( size_t(statBuff.st_size) != fileSize )
								{
									showError( 
										"Bad size",
										genre, artist, record, song,
										location
									);
									std::cout << "expected: " << fileSize << "found: " << statBuff.st_size << '\n';
									numErrors++;
								}
							}
						}
						else
						{
							numErrors++;
							showError( 
								"Unknown location",
								genre, artist, record, song,
								location
							);
						}
						if( record.isEmpty() )
						{
							numErrors++;
							showError( 
								"Unknown record title",
								genre, artist, record, song,
								location
							);
						}
						else if( isspace( (unsigned char)record[0U] )
						|| isspace( (unsigned char)record[record.strlen()-1] ) )
						{
							numErrors++;
							showError( 
								"Record title not stripped",
								genre, artist, record, song,
								location
							);
						}
						if( artist.isEmpty() )
						{
							numErrors++;
							showError( 
								"Unknown artist",
								genre, artist, record, song,
								location
							);
						}
						else if( isspace( (unsigned char)artist[0U] )
						|| isspace( (unsigned char)artist[artist.strlen()-1] ) )
						{
							numErrors++;
							showError( 
								"Artist not stripped",
								genre, artist, record, song,
								location
							);
						}
						if( genre.isEmpty() )
						{
							numErrors++;
							showError( 
								"Unknown genre",
								genre, artist, record, song,
								location
							);
						}
						else if( isspace( (unsigned char)genre[0U] )
						|| isspace( (unsigned char)genre[genre.strlen()-1] ) )
						{
							numErrors++;
							showError( 
								"Genre not stripped",
								genre, artist, record, song,
								location
							);
						}
						if( !artist.isEmpty() )
						{
							const STRING	&existingArtist = artists.addElement( artist );
							if( artist != existingArtist )
							{
								numErrors++;
								showError( 
									"Existing artist differs case sensitive",
									genre, artist, record, song,
									location
								);
							}
						}

						if( song.isEmpty() )
						{
							numErrors++;
							showError( 
								"Unknown song title",
								genre, artist, record, song,
								location
							);
						}
						else if( isspace( (unsigned char)song[0U] )
						|| isspace( (unsigned char)song[song.strlen()-1] ) )
						{
							numErrors++;
							showError( 
								"Song title not stripped",
								genre, artist, record, song,
								location
							);
						}
					}
				}
			}
		}
	}

	std::cout << numErrors << " error(s) " << numSongs << " song(s) found\n";
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

int main( int argc, const char *argv[] )
{
	if( argc != 2 )
	{
		std::cerr << "usage: " << argv[0] << " <lib>\n";
		exit( -1 );
	}
	STRING		library = argv[1];

	doEnableLogEx(gakLogging::llInfo);

	xml::Parser	theParser( library );

	xml::Document *theXmlTemplate = theParser.readFile( false );
	if( !theXmlTemplate )
	{
		std::cerr << "Invalid iTunes Library\n";
		exit( -1 );
	}

	readSongList( theXmlTemplate );


	return 0;
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -a.
#	pragma option -p.
#endif