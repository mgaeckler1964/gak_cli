/*
		Project:		GAK_CLI
		Module:			clip.cpp
		Description:	check a WAV file for clipping
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

#include <gak/cmdlineParser.h>
#include <gak/directoryEntry.h>
#include <gak/map.h>
#include <gak/directory.h>
#include <gak/wavefile.h>

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

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

struct ChannelInfo
{
	struct ClipInfo
	{
		size_t	clipCount;
		size_t	firstOccurence;

		ClipInfo() : clipCount(0), firstOccurence(0)
		{
		}
	};
	typedef gak::PairMap<size_t, ClipInfo>	ClipInfos;
	typedef ClipInfos::const_iterator		const_iterator;

	long	localMaxSample, maxSample, minSample, lastSample;
	size_t	firstMaxOccurence, firstMinOccurence;

	gak::int64	totalSum;
	gak::uint64	absSum, localMaxSum;
	
	size_t	currentClipLength, localMaxCount;

	ChannelInfo() 
		: localMaxSample(0), maxSample(0), minSample(0), lastSample(0), 
		  totalSum(0), 
		  absSum(0), localMaxSum(0), 
		  currentClipLength(0), localMaxCount(0)
	{
	}

	ClipInfos	clipCounts;
};

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

const int OPT_SHOW_CLIP_ONLY		= 0x0010;

static gak::CommandLine::Options options[] =
{
	{ 'C', "clipOnly",			0,  1, OPT_SHOW_CLIP_ONLY },
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

static void findMaxVolume( const gak::F_STRING &fName, gak::WaveFile &waveFile, bool clipsOnly )
{
	bool	clippingFound = false;
	gak::Array<ChannelInfo>	info;

	gak::int32	posClip = waveFile.getPosClip();
	gak::int32	negClip = waveFile.getNegClip();

	const size_t numSamples = waveFile.getNumSamplesPerChannel();
	gak::uint16 numChannels = waveFile.getNumChannels();

	for( size_t t=0; t<numSamples; ++t )
	{
		for( unsigned c=0; c<numChannels; ++c )
		{
			ChannelInfo &channel = info[c];
			gak::int32 sample=waveFile.readSample();

			channel.totalSum += sample;
			channel.absSum += std::abs(sample);

			if( channel.maxSample < sample )
			{
				channel.maxSample = sample;
				channel.firstMaxOccurence = t;
			}
			if( channel.minSample > sample )
			{
				channel.minSample = sample;
				channel.firstMinOccurence = t;
			}

			if( sample == posClip || sample == negClip )
			{
				if( channel.lastSample == sample )
				{
					channel.currentClipLength++;
				}
				else
				{
					channel.currentClipLength = 1;
				}
			}
			else if( channel.currentClipLength )
			{
				if( channel.currentClipLength > 1 )
				{
					clippingFound = true;
					if( channel.clipCounts.hasElement( channel.currentClipLength ) )
					{
						channel.clipCounts[channel.currentClipLength].clipCount++;
					}
					else
					{
						channel.clipCounts[channel.currentClipLength].firstOccurence = t-channel.currentClipLength;
						channel.clipCounts[channel.currentClipLength].clipCount = 1;
					}
				}
				channel.currentClipLength = 0;
			}

			if( sample > 0 )
			{
				if( channel.lastSample < sample )
				{
					channel.localMaxSample = sample;
				}
				else if( channel.localMaxSample > sample )
				{
					channel.localMaxCount++;
					channel.localMaxSum += channel.lastSample;
					channel.localMaxSample = 0;
				}
			}
			channel.lastSample = sample;
		}
	}

	if( !clipsOnly || clippingFound )
	{
		std::cout << fName << ":\n";

		for( unsigned c=0; c<numChannels; ++c )
		{
			if( numChannels == 1 )
			{
				std::cout << "Mono:\n";
			}
			else if( numChannels == 2 )
			{
				std::cout << (c ? "Right:\n" : "Left:\n");
			}
			else
			{
				std::cout << "Channel " << c << ":\n";
			}
			ChannelInfo &channel = info[c];
			std::cout << "min: " << channel.minSample << " Time: " << waveFile.getFormatedTimeCode(channel.firstMinOccurence) << '\n';
			std::cout << "max: " << channel.maxSample << " Time: " << waveFile.getFormatedTimeCode(channel.firstMaxOccurence) << '\n';

			std::cout << "dc: " << double(channel.totalSum)/double(numSamples) << " avg: " << (double(channel.absSum)/double(numSamples)) << '\n';
			std::cout << "avg max: " << double(channel.localMaxSum)/double(channel.localMaxCount) << '\n';

			if( channel.clipCounts.size() )
			{
				std::cout << "\tClipping <Strength>: <Count> <first occurence>\n";

				for( 
					ChannelInfo::const_iterator it = channel.clipCounts.begin(), endIT = channel.clipCounts.end();
					it != endIT;
					++it 
				)
				{
					std::cout << '\t' << it->getKey() << ": " << it->getValue().clipCount << ' ' << waveFile.getFormatedTimeCode(it->getValue().firstOccurence) << '\n';
				}
			}
		}
		std::cout << "neg: " << negClip << " pos: " << posClip << std::endl;
	}
}

static void clip( const gak::F_STRING &fName, bool clipsOnly )
{
	gak::WaveFile	waveFile;

	waveFile.openWaveFile( fName );

	std::cout << fName.rightString( 40 ) << "\r";
	findMaxVolume( fName, waveFile, clipsOnly );
}

static size_t clipFiles( const gak::F_STRING &fName, bool clipsOnly )
{
	size_t				count = 0;
	gak::DirectoryList	dList;

	gak::F_STRING	fullPathName = fullPath( fName );
	dList.findFiles( fName );
	for(
		gak::DirectoryList::const_iterator it = dList.cbegin(), endIT = dList.cend();
		it != endIT;
		++it
	)
	{
		try
		{
			clip( 
				makeFullPath( fullPathName, it->fileName ), clipsOnly
			);
			++count;
		}
		catch( std::exception &e )
		{
			std::cerr << it->fileName << ": " << e.what() << std::endl;
		}
	}

	return count;
}

static int clip( const gak::CommandLine &cmdLine )
{
	bool			filesSpecified = false;
	size_t			count = 0;
	const char		*arg;
	const char		**argv	= cmdLine.argv + 1;
	bool			clipsOnly = cmdLine.flags & OPT_SHOW_CLIP_ONLY;

	while( (arg = *argv++) != NULL )
	{
		gak::F_STRING	fName = arg;
		count += clipFiles( fName, clipsOnly );
		filesSpecified = true;
	}

	if( !filesSpecified  )
	{
		throw gak::CmdlineError( "no files specified" );
	}
	else if( filesSpecified && !count )
	{
		throw gak::CmdlineError( "no file found" );
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

int main( int argc, const char *argv[] )
{	
	int result = EXIT_FAILURE;

	try
	{
		gak::CommandLine cmdLine( options, argv );
		result = clip( cmdLine );
	}
	catch( gak::CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... <Wave Files> ...\n" << options;
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

