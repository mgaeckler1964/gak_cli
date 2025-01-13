/*
		Project:		GAK_CLI
		Module:			dupMails.cpp
		Description:	search for duplicate mails in mbox files
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

#include <gak/dirScanner.h>
#include <gak/mboxParser.h>
#include <gak/md5.h>
#include <gak/map.h>
#include <gak/cmdlineParser.h>

// --------------------------------------------------------------------- //
// ----- includes ------------------------------------------------------ //
// --------------------------------------------------------------------- //

#include <iostream>

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

const int FLAG_CROSS_TREE	= 0x01;
const int FLAG_USE_META		= 0x02;

static gak::CommandLine::Options options[] =
{
	{ 'X', "tree",	0, 1, FLAG_CROSS_TREE, "Find duplicate mails over mbox folder" },
	{ 'M', "meta",	0, 1, FLAG_USE_META, "include meta data in hash calculation" },
	{ 0 }
};

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

typedef gak::Array<gak::mail::MAIL>	Mails;
struct MD5
{
	unsigned char output[16];
	int compare( const MD5 &other ) const
	{
		return memcmp(output, other.output, 16);
	}
};
typedef gak::PairMap<MD5,Mails>	MapMails;

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

class DupMailProcessor : public gak::FileProcessor
{
public:
	MapMails				m_theMap;
	size_t					m_mailCount;

	DupMailProcessor(const gak::CommandLine	&cmdLine)  : gak::FileProcessor(cmdLine), m_mailCount(0)
	{
	}
	void start( const gak::STRING &path )
	{
		std::cout << "start: " << path << std::endl;
	}
	void process( const gak::STRING &file )
	{
		Mails	theMails;
		std::cout << "process: " << file << std::endl;

		gak::mail::loadMboxFile( file, theMails );
		m_mailCount += theMails.size();

		for( 
			Mails::iterator it = theMails.begin(), endIT = theMails.end();
			it != endIT;
			++it
		)
		{
			MD5	hash;
			gak::STRING &text = it->body;

			if( m_cmdLine.flags & FLAG_USE_META )
			{
				text += it->from;
				text += it->to;
				text += it->subject;
				text += it->date.getOriginalTime();
			}

			md5( (unsigned char *)((const char *)text), int(text.strlen()), hash.output );
			text = "";
			m_theMap[hash].addElement(*it);
		}

			if(!(m_cmdLine.flags & FLAG_CROSS_TREE) )
			{
				showResult();
			}

		std::cout << "found: " << theMails.size() << '/' 
				<< m_theMap.size() << '/' << m_mailCount << std::endl;
	}
	void end( const gak::STRING &path )
	{
		std::cout << "end: " << path << std::endl;
	}


	void showResult()
	{
		for( 
			MapMails::const_iterator it = m_theMap.cbegin(), 
			endIT = m_theMap.cend();
			it != endIT;
			++it
		)
		{
			const Mails &theMails = it->getValue();
			if(theMails.size() >1)
			{
				std::cout << "DUP found:\n";
				for(
					Mails::const_iterator it = theMails.cbegin(), endIT = theMails.cend();
					it != endIT;
					++it
				)
				{
					std::cout << it->mboxFile << ": " << it->date << "; " << it->from << "; " << it->subject << std::endl;
				}
			}
		}
		m_theMap.clear();
	}
};

static void dupMails( const gak::CommandLine &cmdLine )
{
	gak::DirectoryScanner<DupMailProcessor> theScanner(cmdLine);

	for( int i=1; i<cmdLine.argc; ++i )
	{
		theScanner(cmdLine.argv[i]);
	}
	if( cmdLine.flags & FLAG_CROSS_TREE )
	{
		theScanner.processor().showResult();
	}

}

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

int main( int , const char *argv[] )
{
	int result = EXIT_FAILURE;

	try
	{
		gak::CommandLine cmdLine( options, argv );
		dupMails( cmdLine );
		result = EXIT_SUCCESS;
	}
	catch( gak::CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... {<Source Path>|<mbox file>} ...\n" << options;
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

