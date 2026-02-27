/*
		Project:		GAK_CLI
		Module:			setBdeConfig.cpp
		Description:	Searches for configuration files of the BDE and updates the registry
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

/* --------------------------------------------------------------------- */
/* ----- includes ------------------------------------------------------ */
/* --------------------------------------------------------------------- */

#include <io.h>

#include <iostream>
#include <fstream>

#include <windows.h>

#include <gak/gaklib.h>
#include <gak/stdlib.h>

#include <winlib/registry.h>

/* --------------------------------------------------------------------- */
/* ----- module switches ----------------------------------------------- */
/* --------------------------------------------------------------------- */

#ifdef __BORLANDC__
#	pragma option -RT-
#	pragma option -a4
#	pragma option -pc
#endif

using namespace winlib;

/* --------------------------------------------------------------------- */
/* ----- constants ----------------------------------------------------- */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- type definitions ---------------------------------------------- */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- macros -------------------------------------------------------- */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- prototypes ---------------------------------------------------- */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- imported datas ------------------------------------------------ */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- module statics ------------------------------------------------ */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- exported datas ------------------------------------------------ */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- module functions ---------------------------------------------- */
/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */
/* ----- entry points -------------------------------------------------- */
/* --------------------------------------------------------------------- */

int main( int argc, const char *argv[] )
{
	Registry	theKey;
	bool		success = false;
	bool		found = false;

	if( argc < 2 )
	{
		std::cerr << "usage: " << argv[0] << " <bdeCfgFile> ..." << std::endl;
		exit( -1 );
	}

	for( int i=1; i<argc; i++ )
	{
		if( !access( argv[i], 0 ) )
		{
			found = true;

			std::cout << "Found " << argv[i] << std::endl;

			if( theKey.openPublic( "Software\\Borland\\Database Engine", KEY_SET_VALUE|KEY_WOW64_32KEY ) == ERROR_SUCCESS )
			{
				if( theKey.setValueEx( "CONFIGFILE01", rtSTRING, LPBYTE(argv[i]), DWORD(strlen(argv[i])+1) ) == ERROR_SUCCESS )
				{
					success = true;
				}
				else
				{
					std::cerr << argv[0] << ": Cannot update registry" << std::endl;
				}
			}
			else
			{
				std::cerr << argv[0] << ": Cannot open registry" << std::endl;
			}

			break;

		}
	}

	if( !found )
	{
		std::cout << "No configurations available -> nothing changed" << std::endl;
	}
	{
		/*
			BDE Error workarround
		*/
		DWORD	sectorsPerCluster = 0;
		DWORD	bytesPerSector = 0;
		DWORD	numberOfFreeClusters = 0;
		DWORD	totalNumberOfClusters = 0;

		GetDiskFreeSpaceA("C:\\",
			&sectorsPerCluster,
			&bytesPerSector,
			&numberOfFreeClusters,
			&totalNumberOfClusters
		);

		std::cout << "sectorsPerCluster " <<sectorsPerCluster << "\n"
			"bytesPerSectors " << bytesPerSector << "\n"
			"numberOfFreeClusters " << numberOfFreeClusters << "\n"
			"totalNumberOfCluster " << numberOfFreeClusters << std::endl;

		INT64	freeDiskSize = INT64(sectorsPerCluster) * INT64(bytesPerSector) * INT64(numberOfFreeClusters);
		freeDiskSize /= 1024LL * 1024LL;
		long bdeFree = long(freeDiskSize%(4*1024));

		std::cout << "Real free: " << freeDiskSize << " MB\nStupid BDE free: " << bdeFree << " MB" << std::endl;

		if( bdeFree < 512 )
		{
			size_t safety = size_t((freeDiskSize + 32)*1024*1024);
			gak::Buffer<char> bdeWasteBuffer( safety );
			if( bdeWasteBuffer )
			{
				char	*tmp = getenv("TMP");
				if( tmp )
				{
					char		tmpfile[1024];

					tmpnam(tmpfile);

					gak::STRING	path = gak::STRING(tmp) + tmpfile;
					{
						std::cout << "create file " << path << ' ' << safety << std::endl;
						std::ofstream fp(path, std::ios_base::binary);
						if( fp.is_open() )
						{
							fp.write(bdeWasteBuffer, safety );
							if( fp.bad() )
							{
								std::cerr << "Cannot write" << std::endl;
								success = false;
							}
						}
						else
						{
							std::cerr << "Cannot create file " << path << std::endl;
							success = false;
						}
					}
				}
				else
				{
					std::cerr << "Cannot find tmp" << std::endl;
					success = false;
				}
			}
			else
			{
				std::cerr << "Cannot allocate file buffer" << std::endl;
				success = false;
			}
		}
	}
	return success ? 0 : -1;
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -a.
#	pragma option -p.
#endif

