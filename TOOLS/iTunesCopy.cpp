/*
		Project:		GAK_CLI
		Module:			iTunesCopy.cpp
		Description:	Copy the tracks of an itunes playlist
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

#include <gak/ci_string.h>
#include <gak/array.h>
#include <gak/xml.h>
#include <gak/http.h>
#include <gak/directory.h>
#include <gak/fcopy.h>
#include <gak/xmlParser.h>

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

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

class iTunesSong
{
	public:
	STRING		id, artist, record, location;
	CI_STRING	destination;
};

static Array<iTunesSong> iTunesLibrary;
static Array<iTunesSong> playList;

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

static STRING replaceBadChars( const char *cp )
{
	char	c;
	STRING	file;

	while( (c = *cp++) != 0 )
	{
		if( strchr( "?*:\\/\"", c ) )
			file += '_';
		else
			file += c;
	}

	file.stripBlanks();
	
	if( file.endsWith( '.' ) )
		file[file.strlen()-1] = '_';
	return file;
}

static void readLibrary(
	xml::Document	*theTemplate,
	const STRING	&destPath
)
{
	STRING	id, artist, record, location, fileName, destination;

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
					bool	bad = false;
					id = "";
					record = "";
					artist = "";
					location = "";

					xml::Element	*recordDict = innerDict->getElement( i );
					if( recordDict && recordDict->getTag() == "dict" )
					{
						size_t	numElements2 = recordDict->getNumObjects();
						for(
							size_t j=0;
							j<numElements2 && (!artist[0U] || !record[0U] || !location[0U] || !id[0U]);
							j += 2
						)
						{
							xml::Element	*key = recordDict->getElement(j);
							xml::Element	*value = recordDict->getElement(j+1);
							if( key
							&& value
							&& key->getTag() == "key"  )
							{
								if( value->getTag() == "string" )
								{
									if( key->getValue( xml::PLAIN_MODE ) == "Album" )
										record = value->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Artist" )
										artist = value->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Location" )
										location = value->getValue( xml::PLAIN_MODE );
								}
								else if( value->getTag() == "integer" )
								{
									if( key->getValue( xml::PLAIN_MODE ) == "Track ID" )
										id = value->getValue( xml::PLAIN_MODE );
								}
							}
						}
						if( !location.isEmpty() )
						{
							if( location.beginsWith( "file://localhost" ) )
								location += (size_t)16;

                            #ifdef _Windows
                                if( location[0U] == '/' && location[2U] == ':' )
                                    location += (size_t)1;
                            #endif

							location = net::webUnEscape( location );
							location.setCharSet( STR_UTF8 );

							#ifdef __MACH__
								location = location.deCanonical();
							#endif

							#if( '/' != DIRECTORY_DELIMITER )
								location.replaceChar( '/', DIRECTORY_DELIMITER );
							#endif

							const char *dirPos = location.strrchr( DIRECTORY_DELIMITER );
							if( dirPos )
							{
								fileName = dirPos+1;
								fileName.setCharSet( STR_UTF8 );
							}
							else
								bad = true;

							if( strAccess( location, 0 ) )
								bad=true;
						}
						else
							bad=true;

						if( id.isEmpty() )
							bad = true;

						if( !bad )
						{
							iTunesSong newSong;

							if( record.isEmpty() )
								record = "Unknown Record";
							if( artist.isEmpty() )
								record = "Unknown Artist";

							destination = destPath;
							destination += DIRECTORY_DELIMITER;
							destination += replaceBadChars( artist );
							destination += DIRECTORY_DELIMITER;
							destination += replaceBadChars( record );
							destination += DIRECTORY_DELIMITER;
							destination += fileName;

							#ifdef _Windows
								if( destination.getCharSet() == STR_UTF8 && destination.testCharSet() == STR_ANSI )
									destination = destination.convertToCharset( STR_ANSI );
							#endif

							newSong.id = id;
							newSong.artist = artist;
							newSong.record = record;
							newSong.location = location;
							newSong.destination = destination;

							iTunesLibrary.addElement( newSong );
						}
					}
				}
			}
		}
	}

	std::cout << iTunesLibrary.size() << " valid songs in library" << std::endl;
}

static void readPlayList( xml::Document *theTemplate, const CI_STRING &playListName )
{
	CI_STRING	playListNameFound;

	xml::Element	*plist = theTemplate->getElement( "plist" );
	if( plist )
	{
		xml::Element *outerDict = plist->getElement( "dict" );
		if( outerDict )
		{
			xml::Element *array = outerDict->getElement( "array" );
			if( array )
			{
				size_t	numElements1 = array->getNumObjects();
				for( size_t i=0; i<numElements1; i++ )
				{
					xml::Element	*playListDict = array->getElement( i );
					if( playListDict && playListDict->getTag() == "dict" )
					{
						playListNameFound = "";
						size_t	numElements2 = playListDict->getNumObjects();
						for(
							size_t j=0;
							j<numElements2 && !playListNameFound[0U];
							j += 2
						)
						{
							xml::Element	*key = playListDict->getElement(j);
							xml::Element	*string = playListDict->getElement(j+1);
							if( key
							&& string
							&& key->getTag() == "key"
							&& string->getTag() == "string" )
							{
								if( key->getValue( xml::PLAIN_MODE ) == "Name" )
									playListNameFound = string->getValue( xml::PLAIN_MODE );
							}
						}

						std::cout << "Found " << playListNameFound.convertToTerminal() << '\n';

						if( playListNameFound == playListName )
						{
							// we got it 8-)
							xml::Element *playListArray = playListDict->getElement( "array" );
							if( playListArray )
							{
								size_t	numElements3 = playListArray->getNumObjects();
								std::cout << numElements3 << " elements found\n";
								for( size_t j=0; j<numElements3; j++ )
								{
									xml::Element	*entryDict = playListArray->getElement( j );
									if( entryDict && entryDict->getTag() == "dict" )
									{
										xml::Element	*integer = entryDict->getElement( "integer" );
										if( integer )
										{
											STRING	id = integer->getValue( xml::PLAIN_MODE );
											size_t	numElements4 = iTunesLibrary.size();
											for( size_t k=0; k<numElements4; k++ )
											{
												const iTunesSong &theSong = iTunesLibrary[k];
												if( theSong.id == id )
													playList.addElement( theSong );
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	std::cout << playList.size() << " valid songs in playlist\n";
	std::cout << iTunesLibrary.size() << " valid songs in database\n";
}

static bool cleanPlayer( const STRING &path )
{
	bool		  	deletePath = true;
	DirectoryList	fileData;
	STRING		  	pattern;
	CI_STRING	  	filePath;

	fileData.dirlist( path );
	for( 
		DirectoryList::iterator it = fileData.begin(), endIT = fileData.end();
		it != endIT;
		++it 
	)
	{
		const STRING	&fileName = it->fileName;

		if( fileName != "." && fileName != ".." )
		{
			filePath = path;
			filePath += DIRECTORY_DELIMITER;
			filePath += fileName;

			#ifdef __MACH__
				filePath = filePath.deCanonical();
			#endif
			if( isDirectory( filePath ) )
			{
				deletePath = cleanPlayer( filePath ) && deletePath;
			}
			else
			{
				bool	found = false;
				size_t	numPlaylistSongs = playList.size();
				for( size_t i=0; !found && i<numPlaylistSongs; i++ )
				{
					const iTunesSong &theSong = playList[i];

					if( filePath == theSong.destination )
						found = true;
				}
				if( !found )
				{
					std::cout << "removing " << filePath.convertToTerminal() << '\n';
					unlink( filePath );
				}
				else
					deletePath = false;
			}

		}
	}

	if( deletePath )
	{
		std::cout << "removing " << path.convertToTerminal() << '\n';
		strRmdirE( path );
	}

	return deletePath;
}

static void copySongs( void )
{
	struct stat	sourceStat, destStat;
	int			errCode;
	size_t		numElements = playList.size();
	size_t		i, numCopyErrors, numCopies;
	STRING		destination;

	i = 0;
	while( i<numElements )
	{
		const iTunesSong &theSong = playList[i];

		errCode = strStat( theSong.location, &sourceStat );
		if( !errCode )
			errCode = strStat( theSong.destination, &destStat );

		if( !errCode && (sourceStat.st_size == destStat.st_size) )
		{
			playList.removeElementAt( i );
			numElements--;
		}
		else
			i++;
	}

	numCopyErrors = numCopies = 0;
	for( i=0; i<numElements; i++ )
	{
		const iTunesSong &theSong = playList[i];

		numCopies++;

		destination = theSong.destination;
		makePath( destination );

		std::cout 
			<< "Copy\n"
			<< theSong.location.convertToTerminal() << '\n'
			<< destination.convertToTerminal() << '\n'
			<< '(' << (i+1) << '/' << numElements << ')' << std::endl
		;

		try
		{
			fcopy( theSong.location, destination );
		}
		catch( std::exception &e )
		{
			numCopyErrors++;
			std::cerr << e.what() << std::endl;
			strRemove( destination );
		}
	}

	std::cout << numCopies << " file(s) to copy, " << numCopyErrors << " error(s)\n";
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
	doEnableLogEx(gakLogging::llInfo);
#ifdef NDEBUG
	if( argc != 4 )
	{
		std::cerr << "usage: " << argv[0] << " <lib> <list> <destination>\n";
		exit( -1 );
	}
	STRING		library = argv[1];
	STRING		list = argv[2];
	STRING		destPath = argv[3];
#else
//	STRING		library = "L:\\opt\\mp3Player\\iTunes\\iTunes Library.xml";
	STRING		library = "C:\\CRESD\\sound\\XiTunes\\iTunes Library.xml";
//	STRING		list = "HTC";
//	STRING		list = "Samsung";
	STRING		list = "XXX";
//	STRING		destPath = "F:\\user\\gak\\mp3Player";
//	STRING		destPath = "C:\\TEMP\\mp3Player";
//	STRING		destPath = "C:\\CRESD\\Sound\\HTC";
//	STRING		destPath = "I:\Musik";
	STRING		destPath = getenv( "TMP" );
	destPath += "\\Musik";
#endif
	if( destPath.rightString(1)[0U] == DIRECTORY_DELIMITER )
		destPath = destPath.leftString( destPath.strlen()-1 );

	if( destPath.isEmpty() )
	{
		std::cerr << "Invalid destination path\n";
		exit( -1 );
	}
	STR_CHARSET	charset = destPath.testCharSet();
	#ifdef _Windows
		if( charset == STR_ANSI )
			charset = STR_OEM;
	#endif
	destPath.setCharSet( charset );

	xml::Parser	theParser( library );

	std::cout << "Reading file " << library << '\n';
	xml::Document *theXmlTemplate = theParser.readFile( false );
	if( !theXmlTemplate )
	{
		std::cerr << "Invalid iTunes Library\n";
		exit( -1 );
	}

	std::cout << "Reading library\n";
	readLibrary( theXmlTemplate, destPath );
	std::cout << "Reading play list\n";
	readPlayList( theXmlTemplate, list );

	std::cout << "Purging files\n";
	cleanPlayer( destPath );

	std::cout << "Copying files\n";
	copySongs();

	return 0;
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -a.
#	pragma option -p.
#endif
