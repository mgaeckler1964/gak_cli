/*
		Project:		GAK_CLI
		Module:			setBdeConfig.c
		Description:	Searches for configuration files of the BDE and updates the registry
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

/* --------------------------------------------------------------------- */
/* ----- includes ------------------------------------------------------ */
/* --------------------------------------------------------------------- */

#include <io.h>
#include <windows.h>

#include <gak/gaklib.h>

/* --------------------------------------------------------------------- */
/* ----- module switches ----------------------------------------------- */
/* --------------------------------------------------------------------- */

#ifdef __BORLANDC__
#	pragma option -RT-
#	pragma option -a4
#	pragma option -pc
#endif

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
	HKEY	theKey;
	cBool	success = false;
	cBool	found = false;
	int		i;

	if( argc < 2 )
	{
		fprintf( stderr, "usage: %s <bdeCfgFile> ...", argv[0] );
		exit( -1 );
	}

	for( i=1; i<argc; i++ )
	{
		if( !access( argv[i], 0 ) )
		{
			found = true;

			printf( "Found %s\n", argv[i] );

			if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				"Software\\Borland\\Database Engine",
				0, KEY_SET_VALUE,
				&theKey ) == ERROR_SUCCESS
			)
			{
				if( RegSetValueEx(
					theKey,
					"CONFIGFILE01",
					0,
					REG_SZ,
					(LPBYTE)argv[i], (DWORD)(strlen(argv[i]))
				) == ERROR_SUCCESS )
				{
					success = true;
				}
				else
				{
					puts( "Cannot update registry" );
				}

				RegCloseKey( theKey );
			}
			else
			{
				puts( "Cannot open registry" );
			}

			break;

		}
	}

	if( !found )
	{
		puts( "No configurations available -> nothing changed" );
	}
	{
		/*
			BDE Error workarround
		*/
		DWORD	sectorsPerCluster = 0;
		DWORD	bytesPerSector = 0;
		DWORD	numberOfFreeClusters = 0;
		DWORD	totalNumberOfClusters = 0;
		INT64	freeDiskSize;
		long	bdeFree;

		GetDiskFreeSpaceA("C:\\",
			&sectorsPerCluster,
			&bytesPerSector,
			&numberOfFreeClusters,
			&totalNumberOfClusters
		);

		printf( "sectorsPerCluster %ld\n"
			"sytesPerSectors %ld\n"
			"numberOfFreeClusters %ld\n"
			"totalNumberOfCluster %ld\n",
			sectorsPerCluster,
			bytesPerSector,
			numberOfFreeClusters,
			totalNumberOfClusters
		);

		freeDiskSize = (INT64)sectorsPerCluster *  (INT64)bytesPerSector * (INT64)numberOfFreeClusters;
		freeDiskSize /= (INT64)1024 * (INT64)1024;
		bdeFree = (long)(freeDiskSize%(4*1024));

		printf( "Real free: %ld MB\nStupid BDE free: %ld MB\n", (long)freeDiskSize, bdeFree );
		if( bdeFree < 512 )
		{
			long safety = (long)((freeDiskSize + 32)*1024*1024);
			void *bdeWasteBuffer = malloc(safety);
			if( bdeWasteBuffer )
			{
				char tmpfile[1024];
				char path[1024];
				char	*tmp = getenv("TMP");
				if( tmp )
				{
					tmpnam(tmpfile);
					sprintf( path, "%s%s", tmp, tmpfile );
					{
						FILE *fp = fopen(path, "wb" );
						printf("create file %s %ld\n", path, safety);
						if( fp )
						{
							int count = fwrite(bdeWasteBuffer, 1, safety, fp );
							if( count != safety )
							{
								puts("Cannot write");
								success = false;
							}
							fclose( fp );
						}
						else
						{
							printf("Cannot create file %s\n", path);
							success = false;
						}
					}
				}
				else
				{
					puts("Cannot find tmp");
					success = false;
				}
				free( bdeWasteBuffer );
			}
			else
			{
				puts("Cannot allocate file buffer");
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

