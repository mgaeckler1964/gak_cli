/*
		Project:		
		Module:			
		Description:	
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

#ifndef MBOX_INDEX_H
#define MBOX_INDEX_H

// --------------------------------------------------------------------- //
// ----- switches ------------------------------------------------------ //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- includes ------------------------------------------------------ //
// --------------------------------------------------------------------- //

#include <gak/types.h>
#include <gak/string.h>
#include <gak/compare.h>
#include <gak/iostream.h>
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

// --------------------------------------------------------------------- //
// ----- constants ----------------------------------------------------- //
// --------------------------------------------------------------------- //

/// Mailbox index file
static const gak::uint32 MBOX_INDEX_MAGIC	= 0x19901993;
static const gak::uint16 MBOX_INDEX_VERSION	= 0x1;
static const char MBOX_INDEX_EXT[] = ".mboxIdx";

/// Mailbox position file
static const gak::uint32 MBOX_POS_MAGIC		= 0x19931990;
static const gak::uint16 MBOX_POS_VERSION	= 0x1;
static const char MBOX_POS_EXT[] = ".mboxPos";

/// Mail server index file
static const gak::uint32 MAIL_INDEX_MAGIC	= 0x19641964;
static const gak::uint16 MAIL_INDEX_VERSION	= 0x1;
static const char MAIL_INDEX_FILE[] = ".mailIndex";

// AI mail brain
static const gak::uint32 BRAIN_MAGIC		= 0x19701974;
static const gak::uint16 BRAIN_VERSION		= 0x1;
static const char BRAIN_FILE[] = "mail.BrainIndex";

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //

struct MailAddress
{
	gak::STRING		mboxFile;
	size_t			mailIndex;

	MailAddress(const gak::STRING &mboxFile="", size_t mailIndex=0) : mboxFile(mboxFile), mailIndex(mailIndex) {}

	int compare( const MailAddress &other ) const
	{
		int result = gak::compare( mboxFile, other.mboxFile );
		return result ? result : gak::compare( mailIndex, other.mailIndex );
	}
	void toBinaryStream ( std::ostream &stream ) const
	{
		gak::toBinaryStream( stream, mboxFile );
		gak::toBinaryStream( stream, gak::uint64(mailIndex) );
	}
	void fromBinaryStream ( std::istream &stream )
	{
		gak::fromBinaryStream( stream, &mboxFile );
		gak::uint64 mailIndex;
		gak::fromBinaryStream( stream, &mailIndex );
		this->mailIndex = size_t(mailIndex);
	}
};

typedef gak::Index<size_t>			MboxIndex;
typedef gak::Index<MailAddress>		MailIndex;
typedef MboxIndex::SearchResult		MboxSearchResult;
typedef MailIndex::SearchResult		MailSearchResult;
typedef MailIndex::RelevantHits		MailRelevantHits;

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

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -b.
#	pragma option -a.
#	pragma option -p.
#endif

#endif //  MBOX_INDEX_H