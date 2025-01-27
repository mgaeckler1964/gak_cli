/*
		Project:		GAK_CLI
		Module:			mboxIndexer.cpp
		Description:	index mbox files for AI and search
		Author:			Martin Gäckler
		Address:		Hofmannsthalweg 14, A-4030 Linz
		Web:			https://www.gaeckler.at/

		Copyright:		(c) 1988-2025 Martin Gäckler

		This program is free software: you can redistribute it and/or modify  
		it under the terms of the GNU General Public License as published by  
		the Free Software Foundation, version 3.

		You should have received a copy of the GNU General Public License 
		along with this program. If not, see <http://www.gnu.org/licenses/>.

		THIS SOFTWARE IS PROVIDED BY Martin Gäckler, Austria, Linz ``AS IS''
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

#include <gak/dirScanner.h>
#include <gak/cmdlineParser.h>
#include <gak/mboxParser.h>
#include <gak/indexer.h>

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
using gak::mail::Mails;

// --------------------------------------------------------------------- //
// ----- constants ----------------------------------------------------- //
// --------------------------------------------------------------------- //

const int FLAG_USE_META		= 0x10;
const int FLAG_STOP_WORDS	= 0x20;

const uint32 INDEX_MAGIC	= 0x19901993;
const uint16 INDEX_VERSION	= 0x1;

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

struct MailAddress
{
	// STRING		mboxFile;
	size_t		mailIndex;

	MailAddress() : mailIndex(0) {}

	int compare( const MailAddress &other ) const
	{
		return gak::compare( mailIndex, other.mailIndex );
	}
};

typedef gak::Index<size_t>			MailIndex;
typedef MailIndex::RelevantHits		MailSearchResult;

// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

class MailIndexer : public gak::FileProcessor
{
public:
	size_t				m_mailCount;
	gak::ArrayOfStrings	m_stopWords;

	MailIndexer(const gak::CommandLine	&cmdLine)  : gak::FileProcessor(cmdLine), m_mailCount(0)
	{
		if( cmdLine.flags && FLAG_STOP_WORDS )
		{
			STRING stopWordsFile = cmdLine.parameter['S'][0];
			std::cout << "Reading " << stopWordsFile << '\n';
			m_stopWords.readFromFile(stopWordsFile);
		}
	}
	void start( const gak::STRING &path )
	{
		std::cout << "start: " << path << std::endl;
	}
	void process( const gak::STRING &file )
	{
		STRING				indexFile = file + ".mboxIdx";
		size_t				idx=0;
		MailIndex			mboxIndex;
		gak::mail::Mails	theMails;
		std::cout << "process: " << file << std::endl;

		gak::mail::loadMboxFile( file, theMails );
		m_mailCount += theMails.size();
		std::cout << "read " << theMails.size() << "Mails" << std::endl;

		for( 
			Mails::iterator it = theMails.begin(), endIT = theMails.end();
			it != endIT;
			++it
		)
		{
			const gak::mail::MAIL &theMail = *it;
			STRING text = theMail.extractPlainText();

			std::cout << idx << "S\r";

			if( m_cmdLine.flags & FLAG_USE_META )
			{
				std::cout << idx << "M\r";
				text += it->from;
				text += it->to;
				text += it->subject;
				text += it->date.getOriginalTime();
				std::cout << it->subject << "\n";
			}

			std::cout << idx << "I\r";
			gak::StringIndex index = gak::indexString(text, m_stopWords);
			std::cout << idx << "m\r";
			mboxIndex.mergeIndexPositions( idx, index );

			idx++;
			text = "";
//			std::cout << idx << '\r';
		}


		writeToBinaryFile( indexFile, mboxIndex, INDEX_MAGIC, INDEX_VERSION, ovmShortDown );
		std::cout << "found: " << theMails.size() << '/' << m_mailCount << std::endl;
	}
	void end( const gak::STRING &path )
	{
		std::cout << "end: " << path << std::endl;
	}
};

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

static gak::CommandLine::Options options[] =
{
	{ 'M', "meta",	0, 1, FLAG_USE_META, "include meta data in index calculation" },
	{ 'S', "stopWords",	0, 1, FLAG_STOP_WORDS|gak::CommandLine::needArg, "file with stop words" },
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

static int mboxIndexer( const gak::CommandLine &cmdLine )
{
	int result = EXIT_FAILURE;

	gak::DirectoryScanner<MailIndexer> theScanner(cmdLine);

	for( int i=1; i<cmdLine.argc; ++i )
	{
		theScanner(cmdLine.argv[i]);
	}

	return result;
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

int main( int , const char *argv[] )
{
	int result = EXIT_FAILURE;

	try
	{
		gak::CommandLine cmdLine( options, argv );
		result = mboxIndexer( cmdLine );
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options> <mboxfile> ...\n" << options;
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

