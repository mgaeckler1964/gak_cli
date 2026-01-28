/*
		Project:		GAK_CLI
		Module:			season.cpp
		Description:	Show next summer, winter, moon etc.
		Author:			Martin Gäckler
		Address:		Hofmannsthalweg 14, A-4030 Linz
		Web:			https://www.gaeckler.at/

		Copyright:		(c) 1988-2025 Martin Gäckler

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

#include <iostream>
#include <iomanip>

#include <gak/datetime.h>
#include <gak/stringStream.h>
#include <gak/cmdlineParser.h>
#include <gak/numericString.h>

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

const int OPT_LOCATION	= 0x010;
const int CHAR_LOCATION	= 'L';


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

static const std::time_t SECONDS_PER_MINUTE = 60;
static const std::time_t SECONDS_PER_HOUR = SECONDS_PER_MINUTE*60;
static const std::time_t SECONDS_PER_DAY = SECONDS_PER_HOUR*24;
static const std::time_t SECONDS_PER_WEEK = SECONDS_PER_DAY*7;

const char *ptimeUnits[] =
{
	" Wochen ", " Tage ", " Stunden ", " Minuten ", " Sekunden "
};

const char *stimeUnits[] =
{
	" Woche ", " Tag ", " Stunde ", " Minute ", " Sekunde "
};

const char *seasons[] =
{
	"", "Fr\x81hling", "Sommer", "Herbst", "Winter"
};

static gak::CommandLine::Options options[] =
{
	{ CHAR_LOCATION,	"location",	0, 1, OPT_LOCATION|gak::CommandLine::needArg,	"<your location>" },
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

template <std::time_t UNIT_SECCONDS, int UNIT_IDX> 
std::time_t showUnit( std::ostream &out, std::time_t secondsLeft )
{
	if( secondsLeft >= UNIT_SECCONDS )
	{
		std::time_t unit = secondsLeft / UNIT_SECCONDS;
		secondsLeft -= unit*UNIT_SECCONDS;
		out << unit << (unit==1?stimeUnits:ptimeUnits)[UNIT_IDX];
	}

	return secondsLeft;
}

static void showTimeLeft( gak::DateTime now, gak::DateTime event, const char *eventName, std::size_t *previousLen )
{
	doEnterFunctionEx(gakLogging::llInfo, "showTimeLeft");
	doLogValueEx(gakLogging::llInfo, eventName );

	gak::STRING outStr;
	gak::oSTRINGstream	out(outStr);

	std::time_t secondsEvent = event.getUtcUnixSeconds();
	std::time_t secondsNow = now.getUtcUnixSeconds();
	std::time_t secondsLeft;
	bool future;
	if( secondsEvent >= secondsNow )
	{
		secondsLeft = secondsEvent - secondsNow;
		future = true;
	}
	else
	{
		secondsLeft = secondsNow - secondsEvent;
		future = false;
	}

	secondsLeft = showUnit<SECONDS_PER_WEEK, 0>(out, secondsLeft);
	secondsLeft = showUnit<SECONDS_PER_DAY, 1>(out, secondsLeft);
	secondsLeft = showUnit<SECONDS_PER_HOUR, 2>(out, secondsLeft);
	secondsLeft = showUnit<SECONDS_PER_MINUTE, 3>(out, secondsLeft);
	secondsLeft = showUnit<1, 4>(out, secondsLeft);

	out << (future ? "bis zum n" OEM_ae "chsten "	// nächsten
						 : "seit letzten ")
		<< eventName;
	out.flush();
	std::cout << outStr;
	size_t oldLen = *previousLen;
	size_t lineLen = outStr.size();

	doLogValueEx(gakLogging::llInfo, outStr );
	doLogValueEx(gakLogging::llInfo, oldLen );
	doLogValueEx(gakLogging::llInfo, lineLen );

	*previousLen = lineLen;
	for( size_t i=lineLen; i<oldLen; ++i )
	{
		std::cout << ' ';
	}
	std::cout << std::endl;
}

static void season(double longitude, double latitude )
{
	doEnterFunctionEx(gakLogging::llInfo, "season");
	static gak::PODarray<std::size_t>	s_lineLens;

	size_t line=0;
	gak::DateTime now;
#if NO_SEASON
	std::cout << "Jetzt ist " << now.weekDayName() << ' ' << now <<  "     " << std::endl;
#else
	gak::DateTime::Season	season = now.getSeason();
	std::cout << "Jetzt ist " << now.weekDayName() << ' ' << now << ' ' << seasons[season] << "     " << std::endl;
#endif

	if( longitude && latitude )
	{
		gak::DateTime	sunrise, sunset;
		now.sunriseEquation( longitude, latitude, &sunrise, &sunset );
		std::cout << "Sonnenaufgang: " << sunrise << " -untergang: " << sunset << std::endl;
	}

	showTimeLeft( now, gak::DateTime::uptime(), "Rechnerstart", &s_lineLens[line++] );

#if !NO_SEASON
	gak::DateTime	nextSpring = now.nextSpring();
	gak::DateTime	nextSummer = now.nextSummer();
	if (nextSpring < nextSummer )
	{
		showTimeLeft( now, nextSpring, "Fr\x81hling", &s_lineLens[line++] );		// Frühling
		showTimeLeft( now, nextSummer, "Sommer", &s_lineLens[line++] );
	}
	else
	{
		showTimeLeft( now, nextSummer, "Sommer", &s_lineLens[line++] );
		showTimeLeft( now, nextSpring, "Fr\x81hling", &s_lineLens[line++] );		// Frühling
	}

	if( season == gak::DateTime::S_SUMMER )
	{
		showTimeLeft( now, now.nextAutumn(), "Herbst", &s_lineLens[line++] );
	}
	else if( season == gak::DateTime::S_AUTUMN )
	{
		showTimeLeft( now, now.nextWinter(), "Winter", &s_lineLens[line++] );
	}

	gak::DateTime	nextFullMoon = now.nextFullMoon();
	gak::DateTime	nextNewMoon = now.nextNewMoon();
	std::time_t		next, last, moonLevelTime;
	if (nextFullMoon < nextNewMoon )
	{
		gak::DateTime	lastNewMoon = now.lastNewMoon();
		showTimeLeft( now, nextFullMoon, "Vollmond", &s_lineLens[line++] );
		showTimeLeft( now, nextNewMoon, "Neumond", &s_lineLens[line++] );
		showTimeLeft( now, lastNewMoon, "Neumond", &s_lineLens[line++] );

		next = nextFullMoon.getUtcUnixSeconds();
		last = lastNewMoon.getUtcUnixSeconds();

		moonLevelTime = now.getUtcUnixSeconds() - last;
	}
	else
	{
		gak::DateTime	lastFullMoon = now.lastFullMoon();
		showTimeLeft( now, nextNewMoon, "Neumond", &s_lineLens[line++] );
		showTimeLeft( now, nextFullMoon, "Vollmond", &s_lineLens[line++] );
		showTimeLeft( now, lastFullMoon, "Vollmond", &s_lineLens[line++] );

		next = nextNewMoon.getUtcUnixSeconds();
		last = lastFullMoon.getUtcUnixSeconds();

		moonLevelTime = next - now.getUtcUnixSeconds();
	}
	time_t	fullPhase = next-last;
	int		moonPercent = int(100.0*moonLevelTime/fullPhase +0.5);
	std::cout << moonPercent << "% Mondphase   " << std::endl;
#endif
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

int main( int, const char *argv[] )
{
	doEnableLogEx( gakLogging::llInfo );
	//doDisableLog();
	doEnterFunctionEx(gakLogging::llInfo, "main");

	try
	{
		gak::CommandLine cmdLine( options, argv );
		double longitude = 0, latitude = 0;

		if( cmdLine.flags & OPT_LOCATION )
		{
			gak::ArrayOfStrings	location;
			gak::STRING			locStr = cmdLine.parameter[CHAR_LOCATION][0];
			location.createElements( locStr, "," );
			longitude = location[0].getValueE<double>();
			latitude = location[1].getValueE<double>();
		}

#ifdef __WINDOWS__
		system("cls");
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD coord = { 0,0 };
#	ifndef NDEBUG
		for( int i=0; i<7; ++i )
#	else
		while( 1 )
#	endif
		{
			SetConsoleCursorPosition( hConsole, coord );
			season(longitude, latitude);
#	if 0
			gak::DateTime now;
			unsigned sec = now.getSecond();
			Sleep( (10 - sec%10) *1000 );
#	else
			Sleep( 10000 );
#endif
		}
#else
		season(longitude, latitude);
#endif
	}
	catch( gak::CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>\n" << options;
	}
	catch( std::exception &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << argv[0] << ": Unkown error" << std::endl;
	}


}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -b.
#	pragma option -a.
#	pragma option -p.
#endif

