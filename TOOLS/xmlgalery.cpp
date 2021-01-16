/*
		Project:		GAK_CLI
		Module:			xmlgalery.cpp
		Description:	Scan a directory and generate an XML file with all images that can be parsed with an XSL processor
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

#include <io.h>
#include <windows.h>

#include <gak/sortedArray.h>
#include <gak/string.h>
#include <gak/fieldSet.h>
#include <gak/xmlParser.h>
#include <gak/numericString.h>

// --------------------------------------------------------------------- //
// ----- imported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module switches ----------------------------------------------- //
// --------------------------------------------------------------------- //

#ifdef __BORLANDC__
#	pragma option -RT-
#	pragma option -a4
#	pragma option -pc
#endif

using namespace gak;

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

// --------------------------------------------------------------------- //
// ----- class static data --------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- prototypes ---------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module functions ---------------------------------------------- //
// --------------------------------------------------------------------- //

static void makeImageTable( xml::Element *imgTable )
{
	int						i, colCount;
	STRING					fileName, destFileName, href;
	xml::Any				*table, *tr, *img;
	WIN32_FIND_DATA			findData;
	SortedArray<CI_STRING>	fileNames;

	STRING imgPath = imgTable->getAttribute( "imgPath" );
	if( !imgPath[0U] )
	{
		std::cerr << "no image path defined\n";
		exit( -1 );
	}
	imgPath.replaceChar( '/', '\\' );
	if( !imgPath.endsWith( "\\" ) )
		imgPath += '\\';

	STRING destPath = imgTable->getAttribute( "destPath" );
	destPath.replaceChar( '/', '\\' );
	if( !destPath.endsWith( "\\" ) )
		destPath += '\\';

	STRING cellClass = imgTable->getAttribute( "cellClass" );
	STRING javaScriptFunc = imgTable->getAttribute( "javaScriptFunc" );
	STRING registrationFunc = imgTable->getAttribute( "registrationFunc" );

	STRING colCountStr = imgTable->getAttribute( "colCount" );
	if( colCountStr[0U] )
	{
		colCount = colCountStr.getValueE<unsigned>();
		if( colCount < 1 )
			colCount = 1;
	}
	else
		colCount = 1;

	STRING	searchPattern = imgPath;
	searchPattern += "*.*";
	HANDLE hnd = FindFirstFile( searchPattern, &findData );
	if( hnd != INVALID_HANDLE_VALUE )
	{
		do
		{

			fileName = findData.cFileName;
			fileName.lowerCase();
			if( fileName.endsWith( ".jpg" ) || fileName.endsWith( ".gif" ) )
			{
				fileName = findData.cFileName;
				fileNames.addElement( fileName );
			}
		} while( FindNextFile( hnd, &findData ) );
		FindClose( hnd );
	}


	if( fileNames.size() > 0 )
	{
		if( imgTable->getParent()->getTag() == "table" )
		{
			table = static_cast<xml::Any*>(imgTable->getParent());
			table->removeObject( imgTable );
		}
		else
		{
			table = new xml::Any( "table" );
			imgTable->getParent()->replaceObject( imgTable, table );
		}
		for( i=0; i<(int)fileNames.size(); i++ )
		{

			if( i%colCount == 0 )
				tr = static_cast<xml::Any *>(table->addObject( new xml::Any( "tr" ) ));

			xml::Any *td = static_cast<xml::Any *>(tr->addObject( new xml::Any( "td" ) ));

			if( cellClass[0U] )
				td->setStringAttribute( "class", cellClass );

			fileName = imgPath;
			fileName += fileNames[i];
			fileName.replaceChar( '\\', '/' );

			img = new xml::Any( "img" );
			img->setStringAttribute( "src", fileName );

			destFileName = destPath;
			destFileName += fileNames[i];
			if( !access( destFileName, 04 ) )
			{
				destFileName.replaceChar( '\\', '/' );
				xml::Any *anchor = static_cast<xml::Any*>(td->addObject( new xml::Any( "a" ) ));
				if( javaScriptFunc.isEmpty() )
					href = destFileName;
				else
				{
					href  = "javascript: ";
					href += javaScriptFunc;
					href += "('";
					href += destFileName;
					href += "')";
				}
				anchor->setStringAttribute( "href", href );
				anchor->addObject( img );

				if( !registrationFunc.isEmpty() )
				{
					xml::Any *script = static_cast<xml::Any*>(td->addObject( new xml::Any( "script" ) ));
					STRING javaScript = registrationFunc;
					javaScript += "('";
					javaScript += destFileName;
					javaScript += "')";
					script->addObject( new xml::CData( javaScript ) );
				}
			}
			else
				td->addObject( img );
		}
		FindClose( hnd );
	}

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
	int				i;
	FieldSet		configuration;
	STRING			configFile = "xmlgalery.cfg";
	xml::XmlArray	imgTables;

	for( i=1; i<argc-1; i++ )
	{
		if( !strcmpi( argv[i], "-cfg" ) )
			configFile = argv[i+1];
	}

	configuration.loadConfigFile( configFile );

	i = 1;
	while( i<argc-1 )
	{
		if( argv[i][0] == '-' )
		{
			configuration[argv[i]+1] = argv[i+1];
			i += 2;
		}
		else
			i++;
	}

	STRING xmlFile = configuration["template"];
	if( !xmlFile[0U] )
	{
		std::cerr << "no template defined\n";
		exit( -1 );
	}

	STRING xmlOutput = configuration["output"];
	if( !xmlOutput[0U] )
	{
		std::cerr << "no xml output defined\n";
		exit( -1 );
	}

	xml::Parser		theParser( xmlFile );
	xml::Document	*theXmlTemplate = theParser.readFile( false );
	if( !theXmlTemplate )
	{
		std::cerr << "Invalid XML Input\n";
		exit( -1 );
	}

	theXmlTemplate->getAllElements( &imgTables, "imgTable" );

	for( i=0; i<(int)imgTables.size(); i++ )
	{
		makeImageTable( imgTables[i] );
	}

	STRING xmlResult = theXmlTemplate->generateDoc();

	FILE *fp = fopen( xmlOutput, "w" );
	if( fp )
	{
		fputs( (const char*)xmlResult, fp );
		fclose( fp );
	}


	return 0;
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -a.
#	pragma option -p.
#endif
