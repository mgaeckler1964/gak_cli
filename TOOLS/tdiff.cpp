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

#include <fstream>

#include <gak/gaklib.h>
#include <gak/string.h>
#include <gak/exception.h>

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
#ifndef NDEBUG
	const char *_argv[] = {
		"tdiff",
//		"f:\\source\\gaklib\\ctools\\diff_test1.txt",
//		"f:\\source\\gaklib\\ctools\\diff_test2.txt",
		"f:\\source\\gak_cli\\Alt.txt",
		"f:\\source\\gak_cli\\Neu.txt",
		"c:\\temp\\gak\\diff.txt",
		NULL
	};
	argc = 4;
	argv = _argv;
#endif
	int result = 0;
	if( argc < 3 || argc > 4 )
	{
		std::cerr << "Bad number of arguments" << std::endl;
		result = -1;
	}
	else
	{
		gak::STRING		fileName	= argv[3] && *argv[3] ? argv[3] : nullptr;
		gak::STRING		result		= gak::diff( argv[1], argv[2] );
		std::ofstream	fout;
		std::ostream	&out		= !fileName.isEmpty() ? (fout.open(fileName), fout) : std::cout;
		if( out.good() )
		{
			out << result;
		}
		else
		{
			result = -1;
			if( fileName.isEmpty() )
				fileName = "<Console>";
			std::cerr << gak::OpenReadError(fileName).what() << std::endl;
		}
	}

#ifdef _Windows
	if( argc == 4 && !result )
	{
		puts( "Executing diff file" );
		ShellExecute( NULL, "open", argv[3], "", ".", SW_SHOW );
	}
#endif

	return result;
}

