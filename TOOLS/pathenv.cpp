/*
		Project:		GAK_CLI
		Module:			pathEnv.cpp
		Description:	Generate environment variables for some special windows directories
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

#include <iostream>

#include <windows.h>
#include <gak/gaklib.h>
#include <gak/array.h>
#include <winlib/registry.h>

using namespace gak;
using namespace winlib;

static void setenv( const char *env, const Registry &environmentKey )
{
	const char *var	= getenv( env );

	if( var && *var )
	{
		environmentKey.setValueEx( env, rtENV, var, strlen(var)+1 );
		std::cout << env << '=' << var << std::endl;
	}
}

int main()
{
	Registry	shellFolderKey,
				environmentKey;

	shellFolderKey.openPrivate(
		"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"
	);
	environmentKey.openPrivate( "Environment", KEY_ALL_ACCESS );

	RegValuePairs values;
	shellFolderKey.getValuePairs( &values );
	for( 
		RegValuePairs::const_iterator it = values.cbegin(), endIT = values.cend();
		it != endIT;
		++it
	)
	{
		if( it->type == rtSTRING || it->type == rtENV )
		{
			char	*cpSource, *cpDest;
			/*
				Remove all spaces and transform to uppercase from the shell folder
				OK: we are modifying that string buffer. Since the new value will be shorter
				and we do not use strlen it is OK
			*/
			cpSource = cpDest = const_cast<char*>(it->valueName.c_str());
			while( *cpSource )
			{
				if( !isspace( *cpSource ) )
				{
					*cpDest++ = char(toupper( *cpSource++ ));
				}
				else
					cpSource++;
			}
			*cpDest=0;

			environmentKey.setValueEx(it->valueName, it->type, it->valueBuffer, it->valueBuffer.size()+1 );
		}
	}

	char	*temp = getenv( "TEMP" );
	char	*username = getenv( "USERNAME" );
	if( temp && *temp && username && *username )
	{
		size_t	tempLen = strlen( temp );
		size_t	userLen = strlen( username );
		if( tempLen <= userLen || strcmp( username, temp + tempLen-userLen ) )
		{
			STRING value = STRING(temp)+DIRECTORY_DELIMITER_STRING+username;

			environmentKey.writeValue("TEMP", value );
			environmentKey.writeValue("TMP", value );
		}

		mkdir( temp );
	}

	setenv( "HOMEDRIVE", environmentKey );
	setenv( "HOMEPATH", environmentKey );
	setenv( "HOMESHARE", environmentKey );

	return 0;
}
