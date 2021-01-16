/*
		Project:		GAK_CLI
		Module:			pathEnv.c
		Description:	Generate environment variables for some special windows directories
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

#include <windows.h>
#include <gak/gaklib.h>

static void setenv( const char *env, HKEY environmentKey )
{
	const char *var	= getenv( env );

	if( var && *var )
	{
		RegSetValueEx(
			environmentKey,
			env, 0, REG_EXPAND_SZ,
			(unsigned char *)var, (DWORD)(strlen(var))
		);

		printf( "%s=%s", env, var );
	}
}

int main( void )
{
	HKEY	shellFolderKey,
			environmentKey;
	int		i;
	char	valueName[2560],
			value[10240];
	DWORD	nameSize,
			valueSize;
	DWORD	type;
	LONG	errCode;

	char	*temp = getenv( "TEMP" );
	char	*username = getenv( "USERNAME" );

	RegOpenKey(
		HKEY_CURRENT_USER,
		"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
		&shellFolderKey
	);
	RegOpenKey( HKEY_CURRENT_USER, "Environment", &environmentKey );

	i=0;
	while( 1 )
	{
		nameSize = sizeof( valueName );
		valueSize = sizeof( value );

		errCode = RegEnumValue( shellFolderKey, i++,
			valueName, &nameSize,
			NULL,
			&type,
			value,
			&valueSize );
		if( errCode != ERROR_SUCCESS )
			break;

		if( type == REG_SZ || type == REG_EXPAND_SZ )
		{
			char	*cpSource, *cpDest;
			cpSource = cpDest = valueName;
			while( *cpSource )
			{
				if( !isspace( *cpSource ) )
				{
					*cpDest++ = (char)toupper( *cpSource++ );
				}
				else
					cpSource++;
			}
			*cpDest=0;

			RegSetValueEx(
				environmentKey, valueName, 0, REG_SZ, value, valueSize
			);
		}
	}

	if( temp && *temp && username && *username )
	{
		size_t	tempLen = strlen( temp );
		size_t	userLen = strlen( username );
		if( tempLen <= userLen || strcmp( username, temp + tempLen-userLen ) )
		{
			if( tempLen + userLen + 3 < sizeof( value ) )
			{
				strcpy( value, temp );
				strcat( value, DIRECTORY_DELIMITER_STRING );
				strcat( value, username );
				valueSize = (DWORD)(strlen( value ));
				RegSetValueEx(
					environmentKey, "TEMP", 0, REG_SZ, value, valueSize
				);
				RegSetValueEx(
					environmentKey, "TMP", 0, REG_SZ, value, valueSize
				);
			}
		}

		mkdir( temp );
	}

	setenv( "HOMEDRIVE", environmentKey );
	setenv( "HOMEPATH", environmentKey );
	setenv( "HOMESHARE", environmentKey );

	RegCloseKey( environmentKey );
	RegCloseKey( shellFolderKey );

	return 0;
}
