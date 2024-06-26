/*
		Project:		
		Module:			
		Description:	
		Author:			Martin G�ckler
		Address:		Hopfengasse 15, A-4020 Linz
		Web:			https://www.gaeckler.at/

		Copyright:		(c) 1988-2023 Martin G�ckler

		This program is free software: you can redistribute it and/or modify  
		it under the terms of the GNU General Public License as published by  
		the Free Software Foundation, version 3.

		You should have received a copy of the GNU General Public License 
		along with this program. If not, see <http://www.gnu.org/licenses/>.

		THIS SOFTWARE IS PROVIDED BY Martin G�ckler, Austria, Linz ``AS IS''
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

#include <gak/datetime.h>

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
	"", "Fr�hling", "Sommer", "Herbst", "Winter"
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
std::time_t showUnit( std::time_t secondsLeft )
{
	if( secondsLeft >= UNIT_SECCONDS )
	{
		std::time_t unit = secondsLeft / UNIT_SECCONDS;
		secondsLeft -= unit*UNIT_SECCONDS;
		std::cout << unit << (unit==1?stimeUnits:ptimeUnits)[UNIT_IDX];
	}

	return secondsLeft;
}

static void showTimeLeft( gak::DateTime now, gak::DateTime event, const char *eventName )
{
	std::time_t secondsLeft = event.getUtcUnixSeconds() - now.getUtcUnixSeconds();

	secondsLeft = showUnit<SECONDS_PER_WEEK, 0>(secondsLeft);
	secondsLeft = showUnit<SECONDS_PER_DAY, 1>(secondsLeft);
	secondsLeft = showUnit<SECONDS_PER_HOUR, 2>(secondsLeft);
	secondsLeft = showUnit<SECONDS_PER_MINUTE, 3>(secondsLeft);
	secondsLeft = showUnit<1, 4>(secondsLeft);
	std::cout << "bis zum n\x84" "chsten " << eventName << std::endl;	// n�chsten
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

int main( void )
{
	gak::DateTime now;
	std::cout << "Jetzt ist " << now << ' ' << seasons[now.getSeason()] << std::endl;
	gak::DateTime	nextSpring = now.nextSpring();
	showTimeLeft( now, nextSpring, "Fr\x81hling" );		// Fr�hling
	gak::DateTime	nextSummer = now.nextSummer();
	showTimeLeft( now, nextSummer, "Sommer" );

	gak::DateTime	nextFullMoon = now.nextFullMoon();
	showTimeLeft( now, nextFullMoon, "Vollmond" );

	gak::DateTime	nextNewMoon = now.nextNewMoon();
	showTimeLeft( now, nextNewMoon, "Neumond" );
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -b.
#	pragma option -a.
#	pragma option -p.
#endif

