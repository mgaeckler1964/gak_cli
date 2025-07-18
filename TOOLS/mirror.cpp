/*
		Project:		GAK_CLI
		Module:			mirror.cpp
		Description:	backup (create a mirror) of a directoy
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

"L:\Archiv\CD Images" "d:\CD Images" 
"F:\Source" "F:\SourceMIRROR"
*/

// --------------------------------------------------------------------- //
// ----- switches ------------------------------------------------------ //
// --------------------------------------------------------------------- //

#ifndef NDEBUG
#define NOTHREADS
#endif

// --------------------------------------------------------------------- //
// ----- includes ------------------------------------------------------ //
// --------------------------------------------------------------------- //

#include <memory>
#include <fstream>
#include <iomanip>

#include <sys/stat.h>

#include <gak/condQueue.h>
#include <gak/cmdlineParser.h>
#include <gak/thread.h>
#include <gak/directory.h>
#include <gak/fcopy.h>
#include <gak/stack.h>
#include <gak/hash.h>
#include <gak/fmtNumber.h>
#include <gak/numericString.h>
#include <gak/acls.h>
#include <gak/eta.h>

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

const int FLAG_DO_COMPARE	= 0x010;
const int FLAG_DO_LOG		= 0x020;
const int FLAG_CREATE_TREE	= 0x040;
const int OPT_MAX_AGE		= 0x080;
const int OPT_MAX_QUEUE		= 0x100;

const int CHAR_DO_COMPARE	= 'C';
const int CHAR_DO_LOG		= 'L';
const int CHAR_CREATE_TREE	= 'T';
const int CHAR_MAX_AGE		= 'A';
const int CHAR_MAX_QUEUE	= 'Q';

const int COUNT_WIDTH		= 6;
const int LOCK_WIDTH		= 2;

// --------------------------------------------------------------------- //
// ----- macros -------------------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- module static data -------------------------------------------- //
// --------------------------------------------------------------------- //

static LockQueue<STRING>	s_logStrings;
static CommandLine::Options options[] =
{
	{ CHAR_DO_COMPARE,	"compare",		0, 1, FLAG_DO_COMPARE },
	{ CHAR_DO_LOG,		"log",			0, 1, FLAG_DO_LOG },
	{ CHAR_CREATE_TREE,	"createTree",	0, 1, FLAG_CREATE_TREE },
	{ CHAR_MAX_AGE,		"maxAge",		0, 1, OPT_MAX_AGE|CommandLine::needArg,	"<max age in day of backup files>" },
	{ CHAR_MAX_QUEUE,	"maxQueueLen",	0, 1, OPT_MAX_QUEUE|CommandLine::needArg,	"<max length of process queues>" },
	{ 0 }
};

// --------------------------------------------------------------------- //
// ----- type definitions ---------------------------------------------- //
// --------------------------------------------------------------------- //
typedef CondQueue<DirectoryEntry> DirectoryQueue;
// --------------------------------------------------------------------- //
// ----- class definitions --------------------------------------------- //
// --------------------------------------------------------------------- //

class TreeCreator
{
	bool	m_error, m_running, m_performed;
	STRING	m_destination;
	Locker	m_theLock;

	public:
	TreeCreator( const STRING &destination ) : m_destination( destination )
	{
		m_error = m_running = m_performed = false;
	}
	bool hasPerformed( void ) const
	{
		return m_performed;
	}
	bool hasError( void ) const
	{
		return m_error;
	}

	void perform( const STRING &backupPath );
};

class CollectorBase : public Thread
{
	protected:
	DirectoryQueue		m_fileQueue;
	std::size_t			m_count, m_errorCount, m_maxQueueLen;

	public:
	CollectorBase( std::size_t maxQueueLen )
	{
		m_maxQueueLen = maxQueueLen;
		m_count = m_errorCount = 0;
	}
	const DirectoryQueue &getQueue( void ) const
	{
		return m_fileQueue;
	}
	DirectoryQueue &getQueue( void )
	{
		return m_fileQueue;
	}
	Locker &getLocker( void )
	{
		return m_fileQueue.getLocker();
	}
	const Locker &getLocker( void ) const
	{
		return m_fileQueue.getLocker();
	}
	std::size_t getCount( void ) const
	{
		return m_count;
	}
	std::size_t getErrorCount( void ) const
	{
		return m_errorCount;
	}
	int getLockCount( void ) const
	{
		return getLocker().getLockCount();
	}
	ThreadID getLockedBy( void ) const
	{
		return getLocker().getLockedBy();
	}
};

class CollectorThread : public CollectorBase
{
	F_STRING		m_sourcePath,
					m_excludes,
					m_backupPath;
	DirectoryList	m_completeList;
	DateTime	  	m_latestDate;
	F_STRING		m_latestFile;

	void scanDirectory( const STRING &dir, const F_STRING &excludes );

	public:
	CollectorThread( const STRING &sourcePath, const STRING &excludes, std::size_t maxQueueLen )
	: CollectorBase( maxQueueLen ), m_sourcePath( sourcePath ), m_excludes(excludes), m_latestDate( time_t(0) )
	{
		doEnterFunctionEx(gakLogging::llDetail,"CollectorThread::CollectorThread");
		StartThread("CollectorThread");
	}

	virtual void ExecuteThread( void );

	const STRING &getSource( void ) const
	{
		return m_sourcePath;
	}
	const DirectoryEntry *findElement( const DirectoryEntry *other )
	{
		return m_completeList.findElement( *other );
	}
	const DirectoryEntry *findElement( const STRING &fileName )
	{
		return m_completeList.findElement( fileName );
	}

	const STRING &getBackupPath( void ) const
	{
		if( m_backupPath.isEmpty() )
		{
			DateTime	latestDate = m_latestDate.calcOriginalTime();
			char	 	nowC[64];

			sprintf(
				nowC,
				"%04d_%02d_%02d_%02d_%02d_%02d",
				latestDate.getYear(), (int)latestDate.getMonth(), latestDate.getDay(),
				latestDate.getHour(), latestDate.getMinute(), latestDate.getSecond()
			);
			const_cast<CollectorThread*>(this)->m_backupPath = m_sourcePath + nowC;
			s_logStrings.push( STRING("Latest File: ") + m_latestFile );
		}
		return m_backupPath;
	}

};

class CopyFilterThread : public CollectorBase
{
	SharedObjectPointer<CollectorThread>	m_theSrcCollector;
	SharedObjectPointer<CollectorThread>	m_theDstCollector;

	STRING						m_destinationPath;
	bool						m_archiveMode;
	bool						m_compareMode;
	std::size_t					m_checkCount;

	public:
	CopyFilterThread(
		SharedObjectPointer<CollectorThread> srcCollector,
		SharedObjectPointer<CollectorThread> dstCollector,
		const STRING &dest, bool archiveMode, bool compareMode,
		std::size_t maxQueueLen
	)
	: CollectorBase( maxQueueLen ),
	m_theSrcCollector(srcCollector),
	m_theDstCollector(dstCollector),
	m_destinationPath(dest),
	m_archiveMode(archiveMode),
	m_compareMode(compareMode),
	m_checkCount(0)
	{
		StartThread("CopyFilterThread");
	}
	virtual void ExecuteThread( void );
	const STRING &getSource( void ) const
	{
		return m_theSrcCollector->getSource();
	}
	const STRING &getDestination( void ) const
	{
		return m_destinationPath;
	}
	const STRING &getBackupPath( void ) const
	{
		Thread::joinOtherThread( m_theDstCollector );
		return m_theDstCollector->getBackupPath();
	}
	std::size_t getCheckCount() const
	{
		return m_checkCount;
	}
};

class DeleteFilterThread : public CollectorBase
{
	SharedObjectPointer<CollectorThread>	m_theSrcCollector;
	SharedObjectPointer<CollectorThread>	m_theDstCollector;

	STRING		m_sourcePath;
	bool		m_compareMode;

	bool exists( const STRING &sourceFile )
	{
		bool	result;
		if( m_theSrcCollector )
			result = m_theSrcCollector->findElement( sourceFile );
		else
			result = ::exists( sourceFile );

		return result;
	}
	public:
	DeleteFilterThread(
		SharedObjectPointer<CollectorThread> srcCollector,
		SharedObjectPointer<CollectorThread> dstCollector,
		const STRING &src, bool compareMode,
		std::size_t maxQueueLen
	)
	: CollectorBase( maxQueueLen ),
	m_theSrcCollector(srcCollector),
	m_theDstCollector(dstCollector),
	m_sourcePath( src ),
	m_compareMode(compareMode)
	{
		StartThread("DeleteFilterThread");
	}
	virtual void ExecuteThread( void );
	const STRING &getDestination( void ) const
	{
		return m_theDstCollector->getSource();
	}
	const STRING &getSource( void ) const
	{
		return m_sourcePath;
	}
	const STRING &getBackupPath( void ) const
	{
		Thread::joinOtherThread( m_theDstCollector );
		return m_theDstCollector->getBackupPath();
	}
};

class CopyThread : public Thread
{
	std::clock_t								m_startTick;
	std::size_t	  								m_count, m_errorCount, m_aclErrorCount;
	SharedObjectPointer<CopyFilterThread>		m_filter;
	bool										m_archiveMode;
	TreeCreator									*m_theTreeCreator;
	TreeMap<FileID,STRING>						m_copiedFiles;
	unsigned									m_permille;
	uint64										m_totalBytes;

	public:
	bool operator () ( unsigned permille, std::size_t bytesProcessed)
	{
		m_totalBytes += bytesProcessed;
		m_permille = permille;
		return false;
	}
	private:
	void fcopy( const STRING &src, const STRING &dest )
	{
		FileID	srcID = getFileID( src );
		if( !srcID )
		{
			::fcopy( src, dest, *this );
		}
		else
		{
			const STRING	*copiedFile = m_copiedFiles.findValueByKey( srcID );
			if( copiedFile )
			{
				strRemove( dest );
				flink( *copiedFile, dest );
			}
			else
			{
				m_copiedFiles[srcID] = dest;
				::fcopy( src, dest, *this );
			}
		}
	}
	public:
	CopyThread(
		SharedObjectPointer<CopyFilterThread> theFilter,
		bool archiveMode,
		TreeCreator *theTreeCreator
	) : m_filter(theFilter)
	{
		m_startTick = 0;
		m_totalBytes = 0;
		m_aclErrorCount = m_errorCount = m_count = 0;
		m_permille = 0;
		m_archiveMode = archiveMode;
		m_theTreeCreator = theTreeCreator;
		StartThread("CopyThread");
	}
	virtual void ExecuteThread( void );

	std::size_t getCount( void ) const
	{
		return m_count;
	}
	std::size_t getErrorCount( void ) const
	{
		return m_errorCount;
	}
	std::size_t getAclErrorCount( void ) const
	{
		return m_aclErrorCount;
	}
	unsigned getPermille() const
	{
		return m_permille;
	}
	void logDiskSpeed() const
	{
		if( m_startTick )
		{
			const uint64 bytesPerSecond = m_totalBytes * CLOCKS_PER_SEC / (clock()-m_startTick);

			const char *unit;
			uint64 divisor;
			if( bytesPerSecond > 10L*1024L*1024L )
			{
				divisor = (1024L*1024L);
				unit = "M";
			}
			else if( bytesPerSecond > 10L*1024L )
			{
				divisor = 1024L;
				unit = "K";
			}
			else
			{
				divisor = 1L;
				unit = "";
			}
			const uint64 totalBytes = divisor != 1 ? bytesPerSecond / divisor : bytesPerSecond;
			std::cout << ' ' << formatNumber( totalBytes, 0, 0, '_' ) << ' ' << unit << "B/s";
		}
	}
};

class DeleteThread : public Thread
{
	int										m_maxAge;
	std::size_t	  							m_count;
	SharedObjectPointer<DeleteFilterThread>	m_filter;
	TreeCreator								*m_theTreeCreator;
	Stack<STRING>							m_directories;

	public:
	DeleteThread(
		SharedObjectPointer<DeleteFilterThread> filter,
		int maxAge,
		TreeCreator *theTreeCreator
	)
	: m_filter(filter), m_maxAge(maxAge), m_theTreeCreator(theTreeCreator)
	{
		StartThread("DeleteThread");
	}
	virtual void ExecuteThread( void );

	std::size_t getCount( void ) const
	{
		return m_count;
	}
	const Stack<STRING> &getDirectoryStack( void ) const
	{
		return m_directories;
	}
};

class CheckSumThread : public Thread
{
	STRING		m_fileName;

	public:
	MD5Hash		m_hash;

	CheckSumThread( const STRING &fileName ) : m_fileName( fileName )
	{
		StartThread("CheckSumThread");
	};
	virtual void ExecuteThread( void );
};

// --------------------------------------------------------------------- //
// ----- exported datas ------------------------------------------------ //
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

inline STRING getDestFilePath(
	const STRING &sourceFilePath,
	const STRING &sourcePath,
	const STRING &destPath )
{
	doEnterFunctionEx(gakLogging::llDetail,"getDestFilePath");
	STRING	tmp = sourceFilePath;
	tmp += sourcePath.strlen();
	STRING	destFilePath = destPath + tmp;

	return destFilePath;
}

static void removeTree( const STRING &tree )
{
	doEnterFunctionEx(gakLogging::llInfo,"removeTree");
	DirectoryList	backups;
	backups.dirlist( tree );
	for( 
		DirectoryList::iterator it = backups.begin(), endIT = backups.end();
		it != endIT;
		++it
	)
	{
		const STRING &name=it->fileName;
		if( name != "." && name != ".." )
		{
			STRING subTree = tree + DIRECTORY_DELIMITER + name;
			if( it->directory )
			{
				removeTree( subTree );
				strRmdir( subTree );
			}
			else
				strRemove( subTree );
		}
	}

	strRmdir( tree );
	strRemove( tree );
}

static void mergeBackups( const STRING &oldBackup, const STRING &newBackup, bool createTree )
{
	doEnterFunctionEx(gakLogging::llInfo,"mergeBackups");
	DirectoryList	backups;

	if( !createTree )
	{
		std::cout << "Merge " << oldBackup << " to " << newBackup << std::endl;

		backups.dirtree( oldBackup );
		for( 
			DirectoryList::iterator it = backups.begin(), endIT = backups.end();
			it != endIT;
			++it
		)
		{
			const STRING	&oldName = it->fileName;

			if( !createTree )
			{
				STRING newName = getDestFilePath( oldName, oldBackup, newBackup );

				makePath( newName );
				strRename( oldName, newName );
			}

			strRmdir( oldName );
			strRemove( oldName );
		}
	}
	else
	{
		std::cout << "Removing " << oldBackup << std::endl;
	}

	removeTree( oldBackup );

}

static void deleteOldBackups( const STRING &destination, int maxAge, bool createTree )
{
	doEnterFunctionEx(gakLogging::llInfo,"deleteOldBackups");

	Date	  		now, lastBackupDate;
	DirectoryList	backups;
	STRING		  	backupPattern = destination + "*.*";
	STRING		  	path, dirName, mergeSrc;

	std::size_t	slashPos = destination.searchRChar( DIRECTORY_DELIMITER );
	if( slashPos != destination.no_index )
	{
		dirName = (const char *)destination + (slashPos +1);
		path = destination.leftString( slashPos );
	}
	else
	{
		dirName = destination;
		path = "";
	}

	backups.findFiles( backupPattern );
	for( 
		DirectoryList::iterator it = backups.begin(), endIT = backups.end();
		it != endIT;
		++it
	)
	{
		CI_STRING name = it->fileName;
		if( name.beginsWith( dirName ) )
		{
			int year, month, day, dummy;
			year = month = day = 0;
			sscanf(
				((const char *)name)+strlen(dirName),
				"%d_%d_%d_%d_%d_%d",
				&year, &month, &day, &dummy, &dummy, &dummy
			);
			if( year && month && day )
			{
				STRING	tree = path;
				if( !tree.isEmpty() )
					tree += DIRECTORY_DELIMITER;
				tree += name;

				Date	backupDate( static_cast<unsigned char>(day), Date::Month(month), static_cast<unsigned short>(year) );
				int 	age = now - backupDate;
				if( age > maxAge )
				{
					std::cout << "Removing " << tree << std::endl;
					try
					{
						removeTree( tree );
					}
					catch( std::exception &e )
					{
						std::cerr << e.what() << std::endl;
					}
				}
				else
				{
					if( !mergeSrc.isEmpty() )
					{
						if( year < now.getYear()-1 && year == lastBackupDate.getYear() )
						{
							mergeBackups( mergeSrc, tree, createTree );
						}
						else if( ( month < now.getMonth()-1 || year < now.getYear() )
						&& month == lastBackupDate.getMonth()
						&& year == lastBackupDate.getYear() )
						{
							mergeBackups( mergeSrc, tree, createTree );
						}
						else if( age > 2 && lastBackupDate - backupDate == 0 )
						{
							mergeBackups( mergeSrc, tree, createTree );
						}
					}
					lastBackupDate = backupDate;
					mergeSrc = tree;	// save path for later merge
				}
			}
		}
	}
}

static void mirror(
	const STRING &source, const STRING &destination,
	int maxAge, bool createTree, bool doLog, bool compareMode,
	std::size_t maxQueueLen
)
{
	doEnterFunctionEx(gakLogging::llInfo, "mirror" );
	StopWatch	sw(true);
	clock_t deleteTime = 0;
	clock_t copyTime = 0;

	std::auto_ptr<TreeCreator>	theTreeCreator;

	if( createTree && maxAge && exists( destination ) )
	{
		theTreeCreator = std::auto_ptr<TreeCreator>( new TreeCreator( destination ) );
	}

	std::ofstream	log;
	if( doLog )
	{
		STRING	mirrorLog = getTempPath() + DIRECTORY_DELIMITER_STRING "mirror.log";
		log.open( mirrorLog, std::ios_base::app );
	}


	SharedObjectPointer<CollectorThread>	theSourceCollector = new CollectorThread(
		source, ".mirrorExcludes", maxQueueLen
	);
	SharedObjectPointer<CollectorThread>	theDestCollector = new CollectorThread(
		destination, NULL, maxQueueLen
	);


	SharedObjectPointer<DeleteFilterThread>	theDeleteFilter = new DeleteFilterThread(
		maxQueueLen ? SharedObjectPointer<CollectorThread>() : theSourceCollector,
		theDestCollector,
		source, compareMode, maxQueueLen
	);
	SharedObjectPointer<CopyFilterThread>		theCopyFilter = new CopyFilterThread(
		theSourceCollector,
		maxQueueLen ? SharedObjectPointer<CollectorThread>() : theDestCollector,
		destination, maxAge > 0, compareMode, maxQueueLen
	);

	SharedObjectPointer<DeleteThread>			theDeleteConsumer = new DeleteThread(
		theDeleteFilter, maxAge, theTreeCreator.get()
	);

	SharedObjectPointer<CopyThread>			theCopyConsumer = new CopyThread(
		theCopyFilter, maxAge > 0, theTreeCreator.get()
	);

	const DirectoryQueue	&destQueue = theDestCollector->getQueue();
	const DirectoryQueue	&deleteQueue = theDeleteFilter->getQueue();
	const Stack<STRING>		&directoryList = theDeleteConsumer->getDirectoryStack();

	const DirectoryQueue	&sourceQueue = theSourceCollector->getQueue();
	const DirectoryQueue	&copyQueue = theCopyFilter->getQueue();

	if( compareMode )
		std::cout << "check  " << source << std::endl;
	else
		std::cout << "mirror " << source << std::endl;
    std::cout << "to     " << destination << std::endl;
    std::cout << "id     " << GetCurrentProcessId() << std::endl;

	if( maxAge > 0 )
	{
		std::cout << "Removing old backups" << std::endl;
		deleteOldBackups( destination, maxAge, createTree );
	}

	Eta<>	delEtaCalculator;
	Eta<>	checkEtaCalculator;
	Eta<>	copyEtaCalculator;
	while( theCopyConsumer->isRunning || theDeleteConsumer->isRunning )
	{
		if( !deleteTime && !theDeleteConsumer->isRunning )
		{
			deleteTime = sw.get<Seconds<>>().asSeconds();
		}
		if( !copyTime && !theCopyConsumer->isRunning )
		{
			copyTime = sw.get<Seconds<>>().asSeconds();
		}
		Sleep( 1000 );

		static std::size_t	lastCopySize = 0;
		const std::size_t	copySize = copyQueue.size();
		delEtaCalculator.addValue(destQueue.size() + deleteQueue.size()+directoryList.size());
		checkEtaCalculator.addValue(sourceQueue.size());
		copyEtaCalculator.addValue(copySize);
		std::cout << std::setfill( '0' ) << "Mirror " << 
			(theDestCollector->isRunning ? "DC" : "dc") <<
			(theDestCollector->isWaiting ? 'W' : '_') <<
			'/' <<
			std::setw( COUNT_WIDTH ) << destQueue.size() <<
			'/' <<
			std::setw( LOCK_WIDTH ) << theDestCollector->getLockCount() <<
			'/' <<
			(theDeleteFilter->isRunning ? "DF" : "df") <<
			(theDeleteFilter->isWaiting ? 'W' : '_') <<
			'/' <<
			std::setw( COUNT_WIDTH ) << (deleteQueue.size()+directoryList.size()) <<
			'/' <<
			std::setw( LOCK_WIDTH ) << theDeleteFilter->getLockCount() <<
			'/' <<
			(theDeleteConsumer->isRunning ? "DT" : "dt") <<
			(theDeleteConsumer->isWaiting ? 'W' : '_') <<
			" - " <<
			(theSourceCollector->isRunning ? "CC" : "cc") <<
			(theSourceCollector->isWaiting ? 'W' : '_') <<
			'/' <<
			std::setw( COUNT_WIDTH ) << sourceQueue.size() <<
			'/' <<
			std::setw( LOCK_WIDTH ) << theSourceCollector->getLockCount() <<
			'/' <<
			(theCopyFilter->isRunning ? "CF" : "cf") <<
			(theCopyFilter->isWaiting ? 'W' : '_') <<
			'/' <<
			std::setw( COUNT_WIDTH ) << copySize <<
			'/' <<
			std::setw( LOCK_WIDTH ) << theCopyFilter->getLockCount() <<
			'/' <<
			(theCopyConsumer->isRunning ? "CT" : "ct") <<
			(theCopyConsumer->isWaiting ? 'W' : '_')
		;
		if( lastCopySize == copySize && (copySize || theCopyConsumer->getPermille()) )
		{
			std::cout << std::setw( 3 ) << std::setfill( ' ' ) << (theCopyConsumer->getPermille()/10) << 
				'.' << (theCopyConsumer->getPermille()%10) << "% "
			;
			lastCopySize = copySize;
		}
		Eta<>::ClockTicks	copyTicks = copyEtaCalculator.getETA(501,1499);
		Eta<>::ClockTicks	checkTicks = checkEtaCalculator.getETA(501,1499);
		Eta<>::ClockTicks	delTicks = delEtaCalculator.getETA(501,1499);

		if( compareMode )
		{
			std::cout << " m5 " << checkEtaCalculator;
		}
		else if( copyTicks >= checkTicks &&  copyTicks >= delTicks && copyTicks > 0 )
		{
			std::cout << " co " << copyEtaCalculator;
		}
		else if( checkTicks >=  delTicks && checkTicks > 0 )
		{
			std::cout << " ch " << checkEtaCalculator;
		}
		else if( delTicks > 0 && theDeleteConsumer->isRunning )
		{
			std::cout << " de " << delEtaCalculator;
		}
		else
		{
			std::cout << "                         ";
		}
		theCopyConsumer->logDiskSpeed();
		std::cout << " \r" << std::flush;

		if( doLog && s_logStrings.size() )
		{
			STRING	logEntry;
			std::cout << std::endl;
			do
			{
				logEntry = s_logStrings.pop();

				std::cout << logEntry << std::endl;
				if( log.rdbuf()->is_open() )
				{
					log << logEntry << '\n';
				}
			}
			while( s_logStrings.size() );
		}
		else
		{
			s_logStrings.clear();
		}
	}
	sw.stop();
	if( !deleteTime )
	{
		deleteTime = sw.get<Seconds<>>().asSeconds();
	}
	if( !copyTime )
	{
		copyTime = sw.get<Seconds<>>().asSeconds();
	}
	if( !deleteTime )
	{
		deleteTime = 1;
	}
	if( !copyTime )
	{
		copyTime = 1;
	}


	if( compareMode )
	{
		clock_t checkTime = sw.get<Seconds<>>().asSeconds();;
		if( !checkTime )
		{
			checkTime = 1;
		}

		std::cout <<
			"\nChecked    : " << theCopyFilter->getCheckCount() <<
			"\nPer Sec    : " << theCopyFilter->getCheckCount()/checkTime <<
			"\nNot Deleted: " << theDeleteFilter->getCount() <<
			"\nNot Copied : " << theCopyFilter->getCount() <<
			"\nErrors     : " << theCopyFilter->getErrorCount() <<
			std::endl
		;
	}
	else
	{
		std::cout << 
		    "\nProcessed : " << theDestCollector->getCount() << '/' << theSourceCollector->getCount() <<
			"\nPer sec   : " << theDestCollector->getCount() / deleteTime << '/' << theSourceCollector->getCount() / copyTime <<
			"\nDeleted   : " << theDeleteConsumer->getCount() <<
			"\nCopied    : " << theCopyConsumer->getCount() <<
			"\nErrors    : " << theCopyConsumer->getErrorCount() <<
			"\nACL Errors: " << theCopyConsumer->getAclErrorCount() <<
			std::endl
		;
	}
}

static int mirror( const CommandLine &cmdLine )
{
	doEnterFunctionEx(gakLogging::llInfo, "mirror( const CommandLine &cmdLine )" );

	int			maxAge = 0;
	std::size_t	maxQueueLen = 0;
	bool		createTree;
	bool		doLog;
	bool		doCompare;

	if( cmdLine.flags & OPT_MAX_AGE )
	{
		maxAge = cmdLine.parameter[CHAR_MAX_AGE][0].getValueE<unsigned>();
	}
	if( cmdLine.flags & OPT_MAX_QUEUE )
	{
		maxQueueLen = cmdLine.parameter[CHAR_MAX_QUEUE][0].getValueE<std::size_t>();
	}
	doLog = cmdLine.flags & FLAG_DO_LOG;
	doCompare = cmdLine.flags & FLAG_DO_COMPARE;
	createTree = cmdLine.flags & FLAG_CREATE_TREE;

	if( cmdLine.argc != 3 )
		throw CmdlineError();

	STRING	source = cmdLine.argv[1];
	STRING	destination = cmdLine.argv[2];

	if( doCompare )
	{
		doLog = true;
		maxAge = 0;
		createTree = false;
	}
	else if( !maxAge )
		createTree = false;
	else
		maxQueueLen = 0;

	if( source[source.strlen()-1] == DIRECTORY_DELIMITER )
		source.cut( source.strlen() -1 );

	if( destination[destination.strlen()-1] == DIRECTORY_DELIMITER )
		destination.cut( destination.strlen() -1 );

	mirror(
		source, destination,
		maxAge, createTree, doLog, doCompare, maxQueueLen
	);

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

void CollectorThread::scanDirectory( const STRING &dir, const F_STRING &excludes )
{
	doEnterFunctionEx(gakLogging::llDetail,"CollectorThread::scanDirectory");

	ArrayOfStrings	excludeList;
	DirectoryList	dirList;

	try
	{
		dirList.dirlist( dir );
	}
	catch( std::exception &e )
	{
		s_logStrings.push( e.what() );
	}

	if( !m_maxQueueLen )
	{
		while( !waitForLocker( getLocker(), randomNumber(10000) ) )
		{
			Sleep( randomNumber(1000) );
		}
	}
	if( excludes.strlen() )
	{
		STRING excludesPath = dir + DIRECTORY_DELIMITER + excludes;
		excludeList.readFromFile(excludesPath);
		if( excludeList.size() )
		{
			s_logStrings.push( "Read " + formatNumber( excludeList.size() ) + " exclusions for " + dir );
		}
	}

	for( 
		DirectoryList::iterator it = dirList.begin(), endIT = dirList.end();
		it != endIT;
		++it
	)
	{
		DirectoryEntry	fileEntry	= *it;
		const STRING	&file		= fileEntry.fileName;

		if( file != "." && file != ".." && (excludeList.size()  == 0 || excludeList.findElement(file) == excludeList.no_index))
		{

			STRING	newDir = dir;
			newDir += DIRECTORY_DELIMITER;
			newDir += file;
			fileEntry.fileName = newDir;

			if( !fileEntry.directory && fileEntry.modifiedDate > m_latestDate )
			{
				m_latestDate = fileEntry.modifiedDate;
				m_latestFile = fileEntry.fileName;
			}

			m_fileQueue.push( fileEntry );
			m_completeList.addElement( fileEntry );

			while( m_maxQueueLen && m_fileQueue.size() >= m_maxQueueLen )
			{
				Sleep( 1000 );
			}

			m_count++;

			if( fileEntry.directory )
			{
				scanDirectory( newDir, excludes );
			}
		}
	}

	if( !m_maxQueueLen )
	{
		getLocker().unlock();
	}
}

// --------------------------------------------------------------------- //
// ----- class protected ----------------------------------------------- //
// --------------------------------------------------------------------- //

// --------------------------------------------------------------------- //
// ----- class virtuals ------------------------------------------------ //
// --------------------------------------------------------------------- //

void CollectorThread::ExecuteThread( void )
{
	doEnterFunctionEx(gakLogging::llInfo,"CollectorThread::ExecuteThread");

	m_count = 0;
	scanDirectory( m_sourcePath, m_excludes );
	doLogValueEx( gakLogging::llInfo, m_count );
}


void CopyFilterThread::ExecuteThread( void )
{
	doEnterFunctionEx(gakLogging::llInfo,"CopyFilterThread::ExecuteThread");
	bool			inputLocked = false;
	std::ofstream	errFile;

	if( m_compareMode )
	{
		STRING	tmp = getTempPath();
		STRING	errorLog = tmp + DIRECTORY_DELIMITER + "mirror_";

		errorLog += formatNumber( GetCurrentProcessId() );
		errorLog += "_cf_error.log";

		errFile.open( errorLog );
	}

	bool		addFile;
	STRING		reason;

	if( m_theDstCollector )
	{
		join( m_theDstCollector );
	}

	DirectoryEntry	theDestEntry;
	DirectoryQueue	&inputQueue = m_theSrcCollector->getQueue();

	const STRING	&source = m_theSrcCollector->getSource();

	while( m_theSrcCollector->isRunning || inputQueue.size() )
	{
		if( !m_theSrcCollector->isRunning && !inputLocked )
		{
			inputQueue.getLocker().lock();
			inputLocked = true;
		}
		if( inputQueue.wait(2000) )
		{
			addFile = false;
			reason = (const char *)NULL;
			DirectoryEntry theSourceEntry = inputQueue.pop();
			const STRING &theSourceFile = theSourceEntry.fileName;
			inputQueue.unlock();

			STRING theDestFile = getDestFilePath(
				theSourceFile, source, m_destinationPath
			);

			if( m_theDstCollector )
			{
				const DirectoryEntry *tmp = m_theDstCollector->findElement( theDestFile );
				if( tmp )
				{
					theDestEntry = *tmp;
				}
				else
				{
					addFile = true;
				}
			}
			else
			{
				theDestEntry.findFile( theDestFile );
			}

			if( theSourceEntry.directory )
			{
				if( !theDestEntry.directory || addFile )
				{
					addFile = true;
					if( m_compareMode )
					{
						reason = "Directory missing ";
					}
				}
			}
			else
			{
				if( addFile
				|| theSourceEntry.fileSize != theDestEntry.fileSize
				|| abs(
					theSourceEntry.modifiedDate.getUtcUnixSeconds() -
					theDestEntry.modifiedDate.getUtcUnixSeconds()
				) >2 )
				{
					addFile = true;
					if( m_compareMode )
					{
						reason = "Size or Date changed ";
					}
				}
			}

			if( m_compareMode )
			{
				bool checkError = false;
				if( !addFile && !theSourceEntry.directory )
				{
					++m_checkCount;
					SharedObjectPointer<CheckSumThread> md5Source = new CheckSumThread( theSourceFile );
					SharedObjectPointer<CheckSumThread> md5Dest = new CheckSumThread( theDestFile );

					md5Source->join();
					md5Dest->join();

					if( waitForThreads() )
					{
						if( md5Source->m_hash.getDigest() != md5Dest->m_hash.getDigest() )
						{
							addFile = true;
							reason = "Checksum Failure\n" + digestStr( md5Source->m_hash.getDigest() ) + '\n' + digestStr( md5Dest->m_hash.getDigest() ) + '\n';
							if( md5Source->m_hash.m_streamData.size() != md5Dest->m_hash.m_streamData.size() )
							{
								reason += "Size: " + gak::formatNumber( md5Source->m_hash.m_streamData.size() ) + ' ' + gak::formatNumber( md5Dest->m_hash.m_streamData.size() ) + '\n';
							}
							else
							{
								size_t maxErrorDisplay = 5;
								for( size_t j=0, i=0; j<maxErrorDisplay && i<md5Source->m_hash.m_streamData.size(); ++i )
								{
									if( md5Source->m_hash.m_streamData[i] != md5Dest->m_hash.m_streamData[i] )
									{
										reason += "Offset: " + gak::formatNumber( i ) + '/' + gak::formatNumber( md5Dest->m_hash.m_streamData.size() ) + 
											"\nChar: " + md5Source->m_hash.m_streamData[i] + ' ' + gak::formatNumber(int(md5Source->m_hash.m_streamData[i])) + '/' + md5Dest->m_hash.m_streamData[i] + ' ' + gak::formatNumber(int(md5Dest->m_hash.m_streamData[i])) +'\n';
										j++;
									}
								}
							}
							m_errorCount++;
							checkError = true;
						}
					}
					else
					{
						addFile = true;
						reason = "Thread Failure ";
						m_errorCount++;
						checkError = true;
					}

				}
				if( addFile )
				{
					m_count++;
					reason += theSourceFile;
					s_logStrings.push( reason );
					if( checkError )
					{
						errFile << reason << std::endl;
					}
				}
			}
			else
			{
				if( addFile )
				{
					m_fileQueue.push( theSourceEntry );

					m_count++;
					while( m_maxQueueLen && m_fileQueue.size() >= m_maxQueueLen )
					{
						Sleep( 10000 );
					}
				}
#ifdef _Windows
				else if( m_archiveMode )
				{
					unsigned long attr = GetFileAttributes( theSourceFile );
					if( attr & FILE_ATTRIBUTE_ARCHIVE )
					{
						attr &= ~FILE_ATTRIBUTE_ARCHIVE;
						SetFileAttributes( theSourceFile, attr );
					}
				}
#endif
			}
		}
	}
}

void DeleteFilterThread::ExecuteThread( void )
{
	doEnterFunctionEx(gakLogging::llInfo,"DeleteFilterThread::ExecuteThread");

	bool inputLocked = false;
	DirectoryQueue	&inputQueue = m_theDstCollector->getQueue();
	const STRING	&destination = m_theDstCollector->getSource();
	STRING			logEntry;

	if( m_theSrcCollector )
	{
		join( m_theSrcCollector );
	}

	while( m_theDstCollector->isRunning || inputQueue.size() )
	{
		if( !m_theDstCollector->isRunning && !inputLocked )
		{
			inputQueue.getLocker().lock();
			inputLocked = true;
		}
		if( inputQueue.wait(2000) )
		{
			DirectoryEntry theDestFile = inputQueue.pop();
			inputQueue.unlock();

			STRING theSourceFile = getDestFilePath(
				theDestFile.fileName, destination, m_sourcePath
			);
			if( !exists( theSourceFile ) )
			{
				m_count++;
				if( m_compareMode )
				{
					logEntry = "Missing ";
					logEntry += theSourceFile;
					s_logStrings.push( logEntry );

				}
				else
				{
					m_fileQueue.push( theDestFile );

					while( m_maxQueueLen && m_fileQueue.size() >= m_maxQueueLen )
					{
						Sleep( 1000 );
					}
				}
			}
		}
	}
}

void CopyThread::ExecuteThread()
{
	doEnterFunctionEx(gakLogging::llInfo,"CopyThread::ExecuteThread");

	bool inputLocked = false;
	STRING			logEntry;
	DirectoryQueue	&copyQueue = m_filter->getQueue();

	const STRING	&source = m_filter->getSource();
	const STRING	&destination = m_filter->getDestination();

	STRING			backupPath = m_archiveMode ? m_filter->getBackupPath() : STRING("");

	STRING			tmp = getTempPath();

	STRING			errorLog = tmp + DIRECTORY_DELIMITER + "mirror_";
	STRING			copyLog = tmp + DIRECTORY_DELIMITER +  "mirror_";

	errorLog += formatNumber( GetCurrentProcessId() );
	errorLog += "_error.log";
	copyLog += formatNumber( GetCurrentProcessId() );
	copyLog += "_copied.log";

	std::ofstream errFile( errorLog );
	std::ofstream logFile( copyLog );

	logFile << "Copy from " << source << " to " << destination << '\n';

	while( m_filter->isRunning || copyQueue.size() )
	{
		if( !m_filter->isRunning && !inputLocked )
		{
			copyQueue.getLocker().lock();
			inputLocked = true;
		}
		if( copyQueue.wait(randomNumber(1000)) )
		{
			if( !m_startTick )
			{
				m_startTick = std::clock();
			}

			DirectoryEntry	theSourceFile = copyQueue.pop();
			copyQueue.unlock();

			STRING theDestFile = getDestFilePath(
				theSourceFile.fileName, source, destination
			);

			if( isDirectory( theSourceFile.fileName ) )
			{
				if( isFile( theDestFile ) )
				{
					if( m_archiveMode )
					{
						if( m_theTreeCreator )
						{
							m_theTreeCreator->perform( backupPath );
							if( m_theTreeCreator->hasError() )
							{
								errFile << "Error creating backup directory\n";
								m_theTreeCreator = NULL;
							}
							else
							{
								remove( theDestFile );
							}
						}
						if( !m_theTreeCreator )
						{
							STRING theBackupFile = getDestFilePath(
								theSourceFile.fileName, source, backupPath
							);
							makePath( theBackupFile );
							strRename( theDestFile, theBackupFile );
						}
					}
					else
					{
						remove( theDestFile );
					}
				}

				if( !exists( theDestFile ) )
				{
					makePath( theDestFile );
					try
					{
						makeDirectory( theDestFile );
					}
					catch( std::exception &e )
					{
						m_errorCount++;
						errFile << "mkdir " << e.what() << std::endl;
					}
						
					try
					{
						copyACLs( theSourceFile.fileName, theDestFile );
					}
					catch( std::exception &e )
					{
						m_aclErrorCount++;
						errFile << "ACLs " << e.what() << std::endl;
					}
				}
			}
			else	// if( isDirectory( theSourceFile.fileName ) )
			{
				if( m_archiveMode )
				{
					if( m_theTreeCreator )
					{
						m_theTreeCreator->perform( backupPath );
						if( m_theTreeCreator->hasError() )
						{
							errFile << "Error creating backup directory\n";
							m_theTreeCreator = NULL;
						}
					}
					if( !m_theTreeCreator )
					{
						STRING theBackupFile = getDestFilePath(
							theSourceFile.fileName, source, backupPath
						);
						makePath( theBackupFile );
						strRename( theDestFile, theBackupFile );
					}
				}

				if( !exists( theDestFile ) )
				{
					makePath( theDestFile );
				}

				strRemove( theDestFile );
				logFile << theSourceFile.fileName << '\n';

				const unsigned consoleWidth = getConsoleWidth();
				const unsigned padWidth = (consoleWidth - 10)/2;
				logEntry = "Copy ";
				logEntry += theSourceFile.fileName.padCopy( padWidth, STR_P_LEFT );
				logEntry += " to ";
				logEntry += theDestFile.padCopy( padWidth, STR_P_LEFT );
				s_logStrings.push( logEntry );

				try
				{
					fcopy( theSourceFile.fileName, theDestFile );
#ifdef _Windows
					if( m_archiveMode )
					{
						unsigned long attr = GetFileAttributes( theSourceFile.fileName );
						attr &= ~FILE_ATTRIBUTE_ARCHIVE;
						SetFileAttributes( theSourceFile.fileName, attr );
					}
#endif
				}
				catch( std::exception &e )
				{
					m_errorCount++;
					errFile << "Copy " << theSourceFile.fileName << " to " << theDestFile << ": " << e.what() << std::endl;
					logEntry = "Error copy ";
					logEntry += theSourceFile.fileName;
					logEntry += " to ";
					logEntry += theDestFile;
					s_logStrings.push( logEntry );
				}

				try
				{
					copyACLs( theSourceFile.fileName, theDestFile );
				}
				catch( std::exception &e )
				{
					m_aclErrorCount++;
					errFile << "ACLs " << e.what() << std::endl;
				}
			}	// if( isDirectory( theSourceFile.fileName ) )

			m_count++;
		}
	}
	doLogValueEx(gakLogging::llInfo, m_filter->isRunning);
	doLogValueEx(gakLogging::llInfo, copyQueue.size());
	logFile << "Finished copy from " << source << " to " << destination << '\n';
}

void DeleteThread::ExecuteThread()
{
	doEnterFunctionEx(gakLogging::llInfo,"DeleteThread::ExecuteThread");

	bool			inputLocked = false;
	DirectoryQueue	&deleteQueue = m_filter->getQueue();
	STRING			backupPath = m_maxAge ? m_filter->getBackupPath() : NULL_STRING;
	const STRING	&destination = m_filter->getDestination();
	STRING			tmp = getTempPath();
	STRING			deleteLog =  tmp + DIRECTORY_DELIMITER + "mirror_";
	deleteLog += formatNumber( GetCurrentProcessId() );

	deleteLog += "_deleted.log";

	std::ofstream	logFile( deleteLog );

	logFile << "Delete from " << destination << '\n';

	m_count = 0;
	while( m_filter->isRunning || deleteQueue.size() )
	{
		if( !m_filter->isRunning && !inputLocked )
		{
			deleteQueue.getLocker().lock();
			inputLocked = true;
		}
		if( deleteQueue.wait(randomNumber(1000)) )
		{
			DirectoryEntry theDestFile = deleteQueue.pop();
			deleteQueue.unlock();

			if( isDirectory( theDestFile.fileName ) )
			{
				m_directories.push( theDestFile.fileName );
			}
			else
			{
				if( m_maxAge )
				{
					if( m_theTreeCreator )
					{
						m_theTreeCreator->perform( backupPath );
						if( m_theTreeCreator->hasError() )
							m_theTreeCreator = NULL;
						else
							strRemove( theDestFile.fileName );
					}
					if( !m_theTreeCreator )
					{
						STRING	backupFile = getDestFilePath( theDestFile.fileName, destination, backupPath );
						makePath( backupFile );
						strRename( theDestFile.fileName, backupFile );
					}
				}
				else
				{
					strRemove( theDestFile.fileName );
				}
				logFile << theDestFile.fileName << '\n';

				m_count++;
			}
		}
	}
	while( m_directories.size() )
	{
		STRING directory = m_directories.pop();
		strRmdir( directory );
		logFile << directory << '\n';

		m_count++;
	}
	logFile << "Finished deletion from " << destination << '\n';
}

void CheckSumThread::ExecuteThread( void )
{
	doEnterFunctionEx(gakLogging::llDetail,"CheckSumThread::ExecuteThread");
	try
	{
		m_hash.m_storeStream = true;
		m_hash.hash_file( m_fileName );
	}
	catch( std::exception &e )
	{
		std::cerr << "Error (md5) " << m_fileName << ' ' << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << "Error (md5) " << m_fileName << std::endl;
	}
}


// --------------------------------------------------------------------- //
// ----- class publics ------------------------------------------------- //
// --------------------------------------------------------------------- //

void TreeCreator::perform( const STRING &backupPath )
{
	doEnterFunctionEx(gakLogging::llInfo,"TreeCreator::perform");
	LockGuard	lock( m_theLock, 100000 );

	if( lock )
	{
		if( !hasPerformed() )
		{
			m_running = true;
			try
			{
				STRING	logEntry = STRING("Link ") + m_destination +" to " + backupPath;
				s_logStrings.push( logEntry );
				if( dlink( m_destination, backupPath ) )
				{
					m_error = true;
				}
			}
			catch( ... )
			{
				m_error = true;
			}
			m_performed = true;
			m_running = false;
		}
	}
	else
	{
		while( m_running )
		{
			Sleep( 1000 );
		}
	}
}

// --------------------------------------------------------------------- //
// ----- entry points -------------------------------------------------- //
// --------------------------------------------------------------------- //

int main( int , const char *argv[] )
{
	int result = EXIT_FAILURE;

	doEnableLogEx(gakLogging::llInfo);
	doImmediateLog();
	doEnterFunctionEx(gakLogging::llInfo,"main");

	try
	{
		CommandLine cmdLine( options, argv );
		result = mirror( cmdLine );
	}
	catch( CmdlineError &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <options>... <Source Path> <Destination Path>\n" << options;
	}
	catch( std::exception &e )
	{
		std::cerr << argv[0] << ": " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << argv[0] << ": Unkown error" << std::endl;
	}

	doFlushLogs();
	doDisableLog();
	return result;
}

#ifdef __BORLANDC__
#	pragma option -RT.
#	pragma option -a.
#	pragma option -p.
#endif

