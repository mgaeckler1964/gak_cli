/*
		Project:		GAK_CLI
		Module:			minMax.cpp
		Description:	search for smallest/bigest, newest/oldest files
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

#include <gak/gaklib.h>
#include <gak/exception.h>
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

// --------------------------------------------------------------------- //
// ----- constants ----------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

#undef max
#undef min

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

struct MinMaxData
{
	gak::F_STRING	m_minSizeName, m_maxSizeName,
					m_minDateName, m_maxDateName;

	gak::uint64		m_minSize, m_maxSize;
	time_t			m_minDate, m_maxDate;

	MinMaxData()
	{
		m_minSize = std::numeric_limits<gak::uint64>::max();
		m_maxSize = std::numeric_limits<gak::uint64>::min();
		m_minDate = std::numeric_limits<time_t>::max();
		m_maxDate = std::numeric_limits<time_t>::min();
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

// --------------------------------------------------------------------- //
// ----- class inlines ------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class constructors/destructors -------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class static functions ---------------------------------------- //
// --------------------------------------------------------------------- //

void minMaxFile( const gak::DirectoryEntry &file, MinMaxData &minMaxData )
{
	if( !file.directory )
	{
		if( minMaxData.m_minSize > file.fileSize )
		{
			minMaxData.m_minSizeName = file.fileName;
			minMaxData.m_minSize = file.fileSize;
		}
		if( minMaxData.m_maxSize < file.fileSize )
		{
			minMaxData.m_maxSizeName = file.fileName;
			minMaxData.m_maxSize = file.fileSize;
		}
		if( minMaxData.m_minDate > file.modifiedDate.getUnixSeconds() )
		{
			minMaxData.m_minDateName = file.fileName;
			minMaxData.m_minDate = file.modifiedDate.getUnixSeconds();
		}
		if( minMaxData.m_maxDate < file.modifiedDate.getUnixSeconds() )
		{
			minMaxData.m_maxDateName = file.fileName;
			minMaxData.m_maxDate = file.modifiedDate.getUnixSeconds();
		}
	}
}

inline void minMaxFile( const gak::F_STRING &fName, MinMaxData &minMaxData )
{
	gak::DirectoryEntry file(fName);
	minMaxFile( file, minMaxData );
}

void minMaxDirectory( const gak::F_STRING &directoryName, MinMaxData &minMaxData )
{
	gak::DirectoryList	dirList;

	dirList.dirlist( directoryName );
	for(
		gak::DirectoryList::iterator it = dirList.begin(), endIT = dirList.end();
		it != endIT;
		++it
	)
	{
		if( it->fileName != "." && it->fileName != ".." )
		{
			gak::F_STRING	fName = directoryName + DIRECTORY_DELIMITER + it->fileName;
			it->fileName = fName;
			minMaxFile( *it, minMaxData );
			if( gak::isDirectory( fName ) )
			{
				minMaxDirectory( fName, minMaxData );
			}
		}
	}
}

int minMax( const char *argv[] )
{
	MinMaxData	minMaxData;
	while( const char *arg = *++argv )
	{
		gak::F_STRING fName = arg;
		minMaxFile( fName, minMaxData );
		if( gak::isDirectory( fName ) )
		{
			minMaxDirectory( fName, minMaxData );
		}
	}

	std::cout << "Oldest Entry:\t" << minMaxData.m_minDateName
			  << "\n\t\t" << gak::DateTime(minMaxData.m_minDate) 
			  << "\nNewest Entry:\t" << minMaxData.m_maxDateName
			  << "\n\t\t" << gak::DateTime(minMaxData.m_maxDate) 
			  << "\nSmallest Entry:\t" << minMaxData.m_minSizeName
			  << "\n\t\t" << minMaxData.m_minSize
			  << "\nBiggest Entry:\t" << minMaxData.m_maxSizeName
			  << "\n\t\t" << minMaxData.m_maxSize << std::endl;
			  
	return EXIT_SUCCESS;
}

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

int main( int /*argc*/, const char *argv[] )
{
	int result = EXIT_FAILURE;

	try
	{
		result = minMax( argv );
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

