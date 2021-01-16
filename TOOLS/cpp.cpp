/*
		Project:		GAK_CLI
		Module:			cpp.cpp
		Description:	A C/C++ preprocessor
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

#include <gak/cmdlineParser.h>
#include <gak/nullStream.h>
#include <gak/cppParser.h>
#include <gak/cppPreprocessor.h>
#include <gak/directory.h>
#include <gak/directoryEntry.h>
#include <gak/set.h>

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

using namespace gak;

// --------------------------------------------------------------------- //
// ----- constants ----------------------------------------------------- //
// --------------------------------------------------------------------- //

const int OPT_INCLUDE		= 0x0010;
const int OPT_MACRO			= 0x0020;
const int OPT_IGNORE		= 0x0040;
const int OPT_SHOW			= 0x0080;
const int OPT_PRINT			= 0x0100;
const int OPT_SHOW_MACROS	= 0x0200;
const int OPT_SHOW_EXTENDED = 0x0400;
const int OPT_CROSS_REF_DB	= 0x0800;
const int OPT_FILTER		= 0x1000;

static const uint32 magic = ('X'<<24) | ('R'<<16) | ('d'<<8) | 'b';

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

typedef	Array< KeyValuePair<STRING, STRING> >	ArrayOfMacros;
typedef TreeMap< F_STRING, Includes >			IncludeMap;

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

static const STRING	cplusplusMacro = "__cplusplus";
static const STRING	cplusplusValue = "1";

static gak::CommandLine::Options options[] =
{
	{ 'S', "showIncludes",			0,  1, OPT_SHOW },
	{ 'M', "showMacros",			0,  1, OPT_SHOW_MACROS },
	{ 'X', "showExtended",			0,  1, OPT_SHOW_EXTENDED },
	{ 'E', "ignoreIncludeError",	0,  1, OPT_IGNORE },
	{ 'P', "printCppResult",		0,  1, OPT_PRINT },
	{ 'F', "filter",				0,  1, OPT_FILTER|CommandLine::needArg,							"<macro name>" },
	{ 'D', "macro",					0, -1, OPT_MACRO|CommandLine::needArg|CommandLine::noAssignOp,	"<macro definition>" },
	{ 'I', "includePath",			0, -1, OPT_INCLUDE|CommandLine::needArg,						"<include path>" },
	{ 'C', "crossRefDB",			0,  1, OPT_CROSS_REF_DB|CommandLine::needArg,					"<cross reference database>" },
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

static void precompileFile( 
	const F_STRING &fName, 
	const ArrayOfStrings &includes, const ArrayOfMacros &macros,
	int flags, 
	IncludeMap &includeMap, CrossReference &crossReference
)
{
	F_STRING		extension = getExtension( fName );
	CPPparser		theFile( fName );
	CPreprocessor	myProcessor(CPreprocessor::omBinary);

	if( extension == "cpp" || extension == "cxx" || extension == "hxx" )
	{
		myProcessor.addMacro( cplusplusMacro, cplusplusValue );
	}

	for(
		ArrayOfStrings::const_iterator it = includes.cbegin(), endIT = includes.cend();
		it != endIT;
		++it
	)
	{
		myProcessor.addIncludePath( *it );
	}
	for(
		ArrayOfMacros::const_iterator it = macros.cbegin(), endIT = macros.cend();
		it != endIT;
		++it
	)
	{
		myProcessor.addMacro( it->getKey(), it->getValue() );
	}

	if( flags & OPT_IGNORE )
	{
		myProcessor.ignoreIncludeErrors();
	}

	if( flags & OPT_PRINT )
	{
		myProcessor.setOutputMode( CPreprocessor::omText );
		myProcessor.precompile( &theFile, std::cout );
	}
	else
	{
		myProcessor.precompile( &theFile, oNullStream() );
	}
	STRING error = theFile.getErrors();
	if( !error.isEmpty() )
	{
		std::cerr << error << std::endl;
	}
	
	if( flags & (OPT_SHOW_MACROS|OPT_CROSS_REF_DB) )
	{
		const CrossReference &curCrossReference = myProcessor.getCrossReference();
		MergeCrossReference( crossReference, curCrossReference );
	}
	if( flags & OPT_SHOW )
	{
		Includes &curIncludes = myProcessor.getIncludeFiles();
		Includes &newIncludes = includeMap[fName];

		newIncludes.moveFrom( curIncludes );
	}
}

static size_t precompileFiles( 
	const F_STRING &fName, 
	const ArrayOfStrings &includes, const ArrayOfMacros &macros,
	int flags, 
	IncludeMap &includeMap, CrossReference &crossReference
)
{
	size_t			count = 0;
	DirectoryList	dList;

	F_STRING	fullPathName = fullPath( fName );
	dList.findFiles( fName );
	for(
		DirectoryList::const_iterator it = dList.cbegin(), endIT = dList.cend();
		it != endIT;
		++it
	)
	{
		precompileFile( 
			makeFullPath( fullPathName, it->fileName ), 
			includes, macros, flags, 
			includeMap, crossReference 
		);
		++count;
	}

	return count;
}

static int cpp( const CommandLine &cmdLine )
{
	IncludeMap				includeMap;
	CrossReference			crossReference;
	bool					filesSpecified = false;
	size_t					count = 0;
	const ArrayOfStrings	&includes = cmdLine.parameter['I'];
	const ArrayOfStrings	&macrosParams = cmdLine.parameter['D'];
	ArrayOfMacros			macros;
	const char				**argv	= cmdLine.argv + 1;
	const char				*arg;
	STRING					filter = cmdLine.flags & OPT_FILTER ? cmdLine.parameter['F'][0] : STRING();

	for(
		ArrayOfStrings::const_iterator	it = macrosParams.cbegin(), endIT = macrosParams.cend();
		it != endIT;
		++it
	)
	{
		STRING macro = *it;

		size_t	assignPos = macro.searchChar( '=' );
		STRING value = assignPos != STRING::no_index ? macro.subString(assignPos+1) : STRING("0");
		if( assignPos != STRING::no_index )
		{
			macro.cut( assignPos );
		}
		if( !macro.isEmpty() )
		{
			KeyValuePair<STRING, STRING>	&macroDef = macros.createElement();
			macroDef.setKey( macro );
			macroDef.setValue( value );
		}
	}

	F_STRING	crossRefDB = cmdLine.flags & OPT_CROSS_REF_DB ? cmdLine.parameter['C'][0] : STRING();
	if( !crossRefDB.isEmpty() && isFile( crossRefDB ) )
	{
		readFromFile( crossRefDB, &crossReference, magic );
	}

	while( (arg = *argv++) != NULL )
	{
		F_STRING	fName = arg;
		count += precompileFiles( fName, includes, macros, cmdLine.flags, includeMap, crossReference );
		filesSpecified = true;
	}

	if( cmdLine.flags & OPT_SHOW )
	{
		for(
			IncludeMap::const_iterator	it = includeMap.cbegin(), endIT = includeMap.cend();
				it != endIT;
				++it 
		)
		{
			const Includes &includeFiles = it->getValue();
			std::cout << it->getKey() << ':' << std::endl;
			for(
				Includes::const_iterator it = includeFiles.cbegin(), endIT = includeFiles.cend();
				it != endIT;
				++it 
			)
			{
				const Set<F_STRING>	&sourceFiles = it->getValue();

				std::cout << it->getKey() << " (" << sourceFiles.size() << ')' << std::endl;
				if( cmdLine.flags & OPT_SHOW_EXTENDED )
				{
					for(
						Set<F_STRING>::const_iterator it = sourceFiles.cbegin(), endIT = sourceFiles.cend();
						it != endIT;
						++it 
					)
					{
						std::cout << '\t' << *it << std::endl;
					}
				}
			}
		}
	}

	if( cmdLine.flags & OPT_SHOW_MACROS )
	{
		for(
			CrossReference::const_iterator it = crossReference.cbegin(), endIT = crossReference.cend();
			it != endIT;
			++it
		)
		{
			const Declaration	&decl = it->getKey();

			if( filter.isEmpty() || filter == decl.identifier )
			{
				const DeclUsages	&usages = it->getValue();
				std::cout << decl.sourceFile << ' ' << decl.lineNo << ": " << decl.identifier << " (" << usages.size() << ')' << std::endl;
				if( cmdLine.flags & OPT_SHOW_EXTENDED )
				{
					for(
						DeclUsages::const_iterator it = usages.cbegin(), endIT = usages.cend();
						it != endIT;
						++it
					)
					{
						std::cout << '\t' << it->m_fileName << ' ' << it->m_lineNo << ": " << it->m_column << std::endl;
					}
				}
			}
		}
	}

	if( !crossRefDB.isEmpty() && crossReference.size() )
	{
		writeToFile( crossRefDB, crossReference, magic );
	}

	if( filesSpecified && !count )
	{
		throw gak::CmdlineError( "no file found" );
	}

	return EXIT_SUCCESS;
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
	int result = EXIT_FAILURE;

	try
	{
		CommandLine cmdLine( options, argv );
		result = cpp( cmdLine );
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... <Source Files> ...\n" << options;
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

