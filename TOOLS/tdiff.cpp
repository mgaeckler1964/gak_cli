/*
		Project:		GAK_CLI
		Module:			tdiff.cpp
		Description:	show difference of two files
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

/* --------------------------------------------------------------------- */
/* ----- includes ------------------------------------------------------ */
/* --------------------------------------------------------------------- */

#include <stdio.h>

#include <gak/gaklib.h>
#include <gak/io.h>

#ifdef _Windows
#	include <windows.h>
#	include <shellapi.h>
#endif

/* --------------------------------------------------------------------- */
/* ----- entry points -------------------------------------------------- */
/* --------------------------------------------------------------------- */

int main( int argc, const char *argv[] )
{
	gak::STRING	result;
	FILE		*fp;

#ifndef NDEBUG
	const char *_argv[] = {
		"tdiff",
//		"f:\\source\\gaklib\\ctools\\diff_test1.txt",
//		"f:\\source\\gaklib\\ctools\\diff_test2.txt",
		"f:\\source\\gak_cli\\Alt.txt",
		"f:\\source\\gak_cli\\Neu.txt",
		"g:\\temp\\gak\\diff.txt",
		NULL
	};
	argc = 4;
	argv = _argv;
#endif
	if( argc < 3 || argc > 4 )
		puts( "Bad number of arguments" );
	else
	{
		result = gak::diff( argv[1], argv[2] );
		fp = argv[3] && *argv[3] ? fopen( argv[3], "w" ) : stdout;
		if( fp )
		{
			fputs( result, fp );

			if( argv[3] && *argv[3] )
				fclose( fp );
		}
	}

#ifdef _Windows
	if( argc == 4 )
	{
		puts( "Executing diff file" );
		ShellExecute( NULL, "open", argv[3], "", ".", SW_SHOW );
	}
#endif

	return 0;
}

