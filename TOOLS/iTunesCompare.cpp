/*
		Project:		GAK_CLI
		Module:			iTunesCompare.cpp
		Description:	compare two itunes libraries
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

#include <gak/set.h>
#include <gak/string.h>
#include <gak/fieldSet.h>
#include <gak/xmlParser.h>
#include <gak/numericString.h>

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

struct iTunesSong
{
	STRING	genre, artist, album, title;

	size_t			size;
	unsigned short	CDno, cdCount;
	unsigned short	trackNo, trackCount;
	unsigned short	coverCount;
	unsigned short	year;


	public:
	int compare ( const iTunesSong &other ) const
	{
		int result = gak::compare( artist, other.artist );
		if( !result )
			result = gak::compare( album, other.album );
		if( !result )
			result = gak::compare( title, other.title );

		if( !result )
		{
			if( trackNo < other.trackNo )
				result = -1;
			else if( trackNo > other.trackNo )
				result = 1;
		}

		if( !result )
		{
			if( CDno < other.CDno )
				result = -1;
			else if( CDno > other.CDno )
				result = 1;
		}

		return result;
	}
	int compareMeta( const iTunesSong &other ) const
	{
		int result = compare( other );

		if( !result )
			result = genre != other.genre;

		if( !result )
			result = this->size != other.size;
		if( !result )
			result = this->CDno != other.CDno;
		if( !result )
			result = this->cdCount != other.cdCount;
		if( !result )
			result = this->trackNo != other.trackNo;
		if( !result )
			result = this->trackCount != other.trackCount;
		if( !result )
			result = this->coverCount != other.coverCount;
		if( !result )
			result = this->year != other.year;

		return result;
	}
	void clear( void )
	{
		genre = artist = album = title = "";
		size = 0;
		CDno = cdCount = trackNo = trackCount = coverCount = year = 0;
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

// --------------------------------------------------------------------- //
// ----- class static data --------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- prototypes ---------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module functions ---------------------------------------------- //
// --------------------------------------------------------------------- //

static void readGenreList(
	xml::Document		*theTemplate,
	const Set<STRING>	&ignoreGenres,
	Set<STRING>			&genresList
)
{
	doEnterFunction("readGenreList");

	STRING	genre;

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
					genre = "";

					xml::Element	*recordDict = innerDict->getElement( i );
					if( recordDict && recordDict->getTag() == "dict" )
					{
						size_t	numElements2 = recordDict->getNumObjects();
						for( size_t j=0; j<numElements2 && !genre[0U]; j += 2 )
						{
							xml::Element	*key = recordDict->getElement(j);
							xml::Element	*string = recordDict->getElement(j+1);
							if( key
							&& string
							&& key->getTag() == "key"
							&& string->getTag() == "string" )
							{
								if( key->getValue( xml::PLAIN_MODE ) == "Genre" )
									genre = string->getValue( xml::PLAIN_MODE );
							}
						}

						if( !genre.isEmpty() && !ignoreGenres.hasElement( genre ) )
						{
							genresList += genre;
						}
					}
				}
			}
		}
	}
}

static void readArtistsList(
	xml::Document		*theTemplate,
	const Set<STRING>	&ignoreGenres,
	const Set<STRING>	&ignoreArtists,
	Set<STRING>			&artistsList
)
{
	doEnterFunction("readArtistsList");

	STRING	artist, genre;
	bool	compilation;
	size_t	pos;

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
					artist = "";
					genre = "";
					compilation = false;

					xml::Element	*recordDict = innerDict->getElement( i );
					if( recordDict && recordDict->getTag() == "dict" )
					{
						size_t	numElements2 = recordDict->getNumObjects();
						for( size_t j=0; j<numElements2 && (!artist[0U] || !genre[0U] || !compilation); j += 2 )
						{
							xml::Element	*key = recordDict->getElement(j);
							xml::Element	*string = recordDict->getElement(j+1);
							if( key
							&& string
							&& key->getTag() == "key" )
							{
								if( string->getTag() == "string" )
								{
									if( key->getValue( xml::PLAIN_MODE ) == "Artist" )
										artist = string->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Genre" )
										genre = string->getValue( xml::PLAIN_MODE );
								}
								else if( key->getValue( xml::PLAIN_MODE ) == "Compilation" )
								{
									if( string->getTag() == "true" )
										compilation = true;
								}
							}
						}

						if( artist[0U] && genre[0U] && !compilation )
						{
							pos = ignoreGenres.findElement( genre );
							if( pos == ignoreGenres.no_index )
							{
								pos = ignoreArtists.findElement( artist );
								if( pos == ignoreArtists.no_index )
								{
									artistsList += artist;
								}
							}
						}
					}
				}
			}
		}
	}
}

static void readAlbumList(
	xml::Document		*theTemplate,
	const Set<STRING>	&ignoreGenres,
	const Set<STRING>	&ignoreArtists,
	const Set<STRING>	&ignoreRecords,
	Set<STRING>			&recordsList
)
{
	doEnterFunction("readAlbumList");

	STRING	record, genre, artist, title;
	bool	compilation;
	size_t	pos;

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
					record = "";
					artist = "";
					genre = "";
					compilation = false;

					xml::Element	*recordDict = innerDict->getElement( i );
					if( recordDict && recordDict->getTag() == "dict" )
					{
						size_t	numElements2 = recordDict->getNumObjects();
						for( size_t j=0; j<numElements2 && (!genre[0U] || !artist[0U] || !record[0U] || !compilation); j += 2 )
						{
							xml::Element	*key = recordDict->getElement(j);
							xml::Element	*string = recordDict->getElement(j+1);

							if( key
							&& string
							&& key->getTag() == "key" )
							{
								if( string->getTag() == "string" )
								{
									if( key->getValue( xml::PLAIN_MODE ) == "Album" )
										record = string->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Artist" )
										artist = string->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Genre" )
										genre = string->getValue( xml::PLAIN_MODE );
								}
								else if( key->getValue( xml::PLAIN_MODE ) == "Compilation" )
								{
									if( string->getTag() == "true" )
										compilation = true;
								}
							}
						}

						if( record[0U] && artist[0U] && genre[0U] && !compilation )
						{
							pos = ignoreGenres.findElement( genre );
							if( pos == (size_t)-1 )
							{
								pos = ignoreArtists.findElement( artist );
								if( pos == ignoreArtists.no_index )
								{
									title = artist;
									title += ':';
									title+= record;

									pos = ignoreRecords.findElement( title );
									if( pos == ignoreRecords.no_index )
									{
										recordsList += title;
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

static void readSongList(
	xml::Document			*theTemplate,
	const Set<STRING>		&ignoreGenres,
	const Set<STRING>		&ignoreArtists,
	const Set<STRING>		&ignoreRecords,
	const Set<iTunesSong>	&ignoreSongs,
	Set<iTunesSong>			&songsList,
	Set<iTunesSong>			&changedSongs

)
{
	doEnterFunction( "readSongList" );

	STRING			artistRecord, tmp;
	iTunesSong		title;
	size_t			pos;

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
					title.clear();

					xml::Element	*recordDict = innerDict->getElement( i );
					if( recordDict && recordDict->getTag() == "dict" )
					{
						size_t	numElements2 = recordDict->getNumObjects();
						for( size_t j=0; j<numElements2; j += 2 )
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
										title.album = value->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Artist" )
										title.artist = value->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Genre" )
										title.genre = value->getValue( xml::PLAIN_MODE );
									else if( key->getValue( xml::PLAIN_MODE ) == "Name" )
										title.title = value->getValue( xml::PLAIN_MODE );
								}
								else if( value->getTag() == "integer" )
								{
									if( key->getValue( xml::PLAIN_MODE ) == "Size" )
									{
										tmp = value->getValue( xml::PLAIN_MODE );
										title.size = tmp.getValueE<size_t>();
									}
									else if( key->getValue( xml::PLAIN_MODE ) == "Disc Number" )
									{
										tmp = value->getValue( xml::PLAIN_MODE );
										title.CDno = tmp.getValueE<unsigned short>();
									}
									else if( key->getValue( xml::PLAIN_MODE ) == "Disc Count" )
									{
										tmp = value->getValue( xml::PLAIN_MODE );
										title.cdCount = tmp.getValueE<unsigned short>();
									}
									else if( key->getValue( xml::PLAIN_MODE ) == "Track Number" )
									{
										tmp = value->getValue( xml::PLAIN_MODE );
										title.trackNo = tmp.getValueE<unsigned short>();
									}
									else if( key->getValue( xml::PLAIN_MODE ) == "Track Count" )
									{
										tmp = value->getValue( xml::PLAIN_MODE );
										title.trackCount = tmp.getValueE<unsigned short>();
									}
									else if( key->getValue( xml::PLAIN_MODE ) == "Artwork Count" )
									{
										tmp = value->getValue( xml::PLAIN_MODE );
										title.coverCount = tmp.getValueE<unsigned short>();
									}
									else if( key->getValue( xml::PLAIN_MODE ) == "Year" )
									{
										tmp = value->getValue( xml::PLAIN_MODE );
										title.year = tmp.getValueE<unsigned short>();
									}
								}
							}
						}

						if( !title.album.isEmpty()
						&&  !title.artist.isEmpty()
						&&  !title.genre.isEmpty()
						&&  !title.title.isEmpty() )
						{
							pos = ignoreGenres.findElement( title.genre );
							if( pos == ignoreGenres.no_index )
							{
								pos = ignoreArtists.findElement( title.artist );
								if( pos == ignoreArtists.no_index )
								{

									artistRecord = title.artist + ':' + title.album;
									pos = ignoreRecords.findElement( artistRecord );
									if( pos == ignoreRecords.no_index )
									{
										pos = ignoreSongs.findElement( title );
										if( pos == ignoreSongs.no_index )
										{
											songsList += title;
										}
										else
										{
											iTunesSong	secondBaseSong = ignoreSongs[pos];

											if( secondBaseSong.compareMeta( title ) )
											{
												changedSongs.addElement( title );
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
//	doDisableLog();
	doEnterFunction("main");

	Set<STRING>		knownGenres, missingGenres;
	Set<STRING>		knownArtists, missingArtists;
	Set<STRING>		knownRecords, missingRecords;
	Set<iTunesSong>	knownSongs, missingSongs, changedSongs;

	if( argc != 3 )
	{
		std::cerr << "usage: " << argv[0] << " lib#1 lib#2\n";
		exit( -1 );
	}
	STRING		library1 = argv[1];
	STRING		library2 = argv[2];

	doDisableLog();

	xml::Parser	theParser1( library1 );
	xml::Parser	theParser2( library2 );

	xml::Document *theXmlTemplate1 = theParser1.readFile( false );
	if( !theXmlTemplate1 )
	{
		std::cerr << "Invalid XML lib#1 " << library1 << std::endl;
		exit( -1 );
	}
	xml::Document *theXmlTemplate2 = theParser2.readFile( false );
	if( !theXmlTemplate2 )
	{
		std::cerr << "Invalid XML lib#2 " << library2 << std::endl;
		exit( -1 );
	}

	readGenreList( theXmlTemplate2, missingGenres, knownGenres );
	readGenreList( theXmlTemplate1, knownGenres, missingGenres );

	std::cout << "These items are missing in " << library2 << std::endl;
	std::cout << "Genre\n-----\n";

	for( 
		Set<STRING>::const_iterator it = missingGenres.cbegin(), endIT = missingGenres.cend();
		it != endIT;
		++it
	)
		std::cout << '>' << it->convertToTerminal() << "<\n";
	std::cout << missingGenres.size() << " missing genre(s)" << std::endl;

	readArtistsList( theXmlTemplate2, missingGenres, missingArtists, knownArtists );
	readArtistsList( theXmlTemplate1, missingGenres, knownArtists, missingArtists );

	std::cout << "Artist\n------\n";
	for( 
		Set<STRING>::const_iterator it = missingArtists.cbegin(), endIT = missingArtists.cend();
		it != endIT;
		++it
	)
		std::cout << '>' << it->convertToTerminal() << "<\n";
	std::cout << missingArtists.size() << " missing artist(s)" << std::endl;

	readAlbumList( theXmlTemplate2, missingGenres, missingArtists, missingRecords, knownRecords );
	readAlbumList( theXmlTemplate1, missingGenres, missingArtists, knownRecords, missingRecords );

	std::cout << "Artist:Record\n-------------\n";
	for( 
		Set<STRING>::const_iterator it = missingRecords.cbegin(), endIT = missingRecords.cend();
		it != endIT;
		++it
	)
		std::cout << '>' << it->convertToTerminal() << "<\n";
	std::cout << missingRecords.size() << " missing record(s)" << std::endl;

	readSongList( theXmlTemplate2, missingGenres, missingArtists, missingRecords, missingSongs, knownSongs, changedSongs );
	readSongList( theXmlTemplate1, missingGenres, missingArtists, missingRecords, knownSongs, missingSongs, changedSongs );

	std::cout << "Artist:Record:Title:Track#:CD#\n------------------------------\n";
	for( 
		Set<iTunesSong>::const_iterator it = missingSongs.cbegin(), endIT = missingSongs.cend();
		it != endIT;
		++it
	)
	{
		const iTunesSong	&missingSong = *it;
		std::cout 
			<< missingSong.artist.convertToTerminal() << ':' 
			<< missingSong.album.convertToTerminal() << ':'
			<< missingSong.title.convertToTerminal() << ':'
			<< missingSong.trackNo << ':'
			<< missingSong.CDno << '\n'
		;
	}
	std::cout << missingSongs.size() << " missing titles(s)" << std::endl;

	std::cout << "Changed Titles#\n--------------\n";
	for( 
		Set<iTunesSong>::const_iterator it = changedSongs.cbegin(), endIT = changedSongs.cend();
		it != endIT;
		++it
	)
	{
		const iTunesSong	&changedSong = *it;
		const iTunesSong	&knownSong = knownSongs[knownSongs.findElement( changedSong )];
		std::cout 
			<< changedSong.artist.convertToTerminal() << ':' 
			<< changedSong.album.convertToTerminal() << ':'
			<< changedSong.title.convertToTerminal() << ':'
			<< changedSong.trackNo << ':'
			<< changedSong.CDno << '\n'
		;
		if( changedSong.size != knownSong.size )
			std::cout << "Size expected " << changedSong.size << " found " << knownSong.size << '\n';
		if( changedSong.cdCount != knownSong.cdCount )
			std::cout << "CD Count expected " << changedSong.cdCount << " found " << knownSong.cdCount << '\n';
		if( changedSong.trackNo != knownSong.trackNo )
			std::cout << "Title# expected " << changedSong.trackNo << " found " << knownSong.trackNo << '\n';
		if( changedSong.coverCount != knownSong.coverCount )
			std::cout << "Cover expected " << changedSong.coverCount << " found " << knownSong.coverCount << '\n';
		if( changedSong.year != knownSong.year )
			std::cout << "Year expected " << changedSong.year << " found " << knownSong.year << '\n';
		if( changedSong.genre != knownSong.genre )
			std::cout << "Genre expected " << changedSong.genre << " found " << knownSong.genre << '\n';
	}
	std::cout << changedSongs.size() << " changed titles(s)" << std::endl;

	return 0;
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -a.
#	pragma option -p.
#endif