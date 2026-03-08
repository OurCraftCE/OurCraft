#include "stdafx.h"


#include "C4JThread.h"
#include "../../Client/Common/ShutdownManager.h"

std::vector<C4JThread*> C4JThread::ms_threadList;
CRITICAL_SECTION C4JThread::ms_threadListCS;

C4JThread	C4JThread::m_mainThread("Main thread");


C4JThread::C4JThread( C4JThreadStartFunc* startFunc, void* param, const char* threadName, int stackSize/* = 0*/ )
{
	m_startFunc = startFunc;
	m_threadParam = param;
	m_stackSize = stackSize;

	// to match XBox, if the stack size is zero, use the default 64k
	if(m_stackSize == 0)
		m_stackSize = 65536 * 2;
	// make sure it's at least 16K
	if(m_stackSize < 16384)
		m_stackSize = 16384;

	sprintf_s(m_threadName,64, "(4J) %s", threadName );

	m_isRunning = false;
	m_hasStarted = false;

	m_exitCode = STILL_ACTIVE;

	m_threadID = 0;
	m_threadHandle = 0;
	m_threadHandle = CreateThread(NULL, m_stackSize, entryPoint, this, CREATE_SUSPENDED, &m_threadID);
	EnterCriticalSection(&ms_threadListCS);
	ms_threadList.push_back(this);
	LeaveCriticalSection(&ms_threadListCS);
}

// only used for the main thread
C4JThread::C4JThread( const char* mainThreadName)
{
	m_startFunc = NULL;
	m_threadParam = NULL;
	m_stackSize = 0;

	sprintf_s(m_threadName, 64, "(4J) %s", mainThreadName);
	m_isRunning = true;
	m_hasStarted = true;
	m_lastSleepTime = System::currentTimeMillis();

	// should be the first thread to be created, so init the static critical section for the threadlist here
	InitializeCriticalSection(&ms_threadListCS);

	m_threadID = GetCurrentThreadId();
	m_threadHandle = GetCurrentThread();
	EnterCriticalSection(&ms_threadListCS);
	ms_threadList.push_back(this);
	LeaveCriticalSection(&ms_threadListCS);
}

C4JThread::~C4JThread()
{
	EnterCriticalSection(&ms_threadListCS);

	for( AUTO_VAR(it,ms_threadList.begin()); it != ms_threadList.end(); it++ )
	{
		if( (*it) == this )
		{
			ms_threadList.erase(it);			
			LeaveCriticalSection(&ms_threadListCS);
			return;
		}
	}

	LeaveCriticalSection(&ms_threadListCS);
}

DWORD WINAPI	C4JThread::entryPoint(LPVOID lpParam)
{
	C4JThread* pThread = (C4JThread*)lpParam;
	SetThreadName(-1, pThread->m_threadName);
	pThread->m_exitCode = (*pThread->m_startFunc)(pThread->m_threadParam);
	pThread->m_isRunning = false;
	return pThread->m_exitCode;
}




void C4JThread::Run()
{
	ResumeThread(m_threadHandle);
	m_lastSleepTime = System::currentTimeMillis();
	m_isRunning = true;
	m_hasStarted = true;
}

void C4JThread::SetProcessor( int proc )
{
	SetThreadAffinityMask(m_threadHandle, 1 << proc );
}

void C4JThread::SetPriority( int priority )
{
	SetThreadPriority(m_threadHandle, priority);
}

DWORD C4JThread::WaitForCompletion( int timeoutMs )
{
	return WaitForSingleObject(m_threadHandle, timeoutMs);
}

int C4JThread::GetExitCode()
{
	DWORD exitcode = 0;
	GetExitCodeThread(m_threadHandle, &exitcode);

	return *((int *)&exitcode);
}

void C4JThread::Sleep( int millisecs )
{
	::Sleep(millisecs);
}

C4JThread* C4JThread::getCurrentThread()
{
	DWORD currThreadID = GetCurrentThreadId();
	EnterCriticalSection(&ms_threadListCS);

	for(int i=0;i<ms_threadList.size(); i++)
	{
		if(currThreadID == ms_threadList[i]->m_threadID)
		{
			LeaveCriticalSection(&ms_threadListCS);
			return ms_threadList[i];
		}
	}

	LeaveCriticalSection(&ms_threadListCS);

	return NULL;
}

bool C4JThread::isMainThread()
{
	return getCurrentThread() == &m_mainThread;
}

C4JThread::Event::Event(EMode mode/* = e_modeAutoClear*/)
{
	m_mode = mode;
	m_event = CreateEvent( NULL, (m_mode == e_modeManualClear), FALSE, NULL );
}


C4JThread::Event::~Event()
{
	CloseHandle( m_event );
}


void C4JThread::Event::Set()
{
	SetEvent(m_event);
}

void C4JThread::Event::Clear()
{
	ResetEvent(m_event);
}

DWORD C4JThread::Event::WaitForSignal( int timeoutMs )
{
	return WaitForSingleObject(m_event, timeoutMs);
}

C4JThread::EventArray::EventArray( int size, EMode mode/* = e_modeAutoClear*/)
{
	assert(size<32);
	m_size = size;
	m_mode = mode;
	m_events = new HANDLE[size];
	for(int i=0;i<size;i++)
	{
		m_events[i]  = CreateEvent(NULL, (m_mode == e_modeManualClear), FALSE, NULL );
	}
}


void C4JThread::EventArray::Set(int index)
{
	SetEvent(m_events[index]);
}

void C4JThread::EventArray::Clear(int index)
{
	ResetEvent(m_events[index]);
}

void C4JThread::EventArray::SetAll()
{
	for(int i=0;i<m_size;i++)
		Set(i);
}

void C4JThread::EventArray::ClearAll()
{
	for(int i=0;i<m_size;i++)
		Clear(i);
}

DWORD C4JThread::EventArray::WaitForSingle(int index, int timeoutMs )
{
	DWORD retVal;
	retVal = WaitForSingleObject(m_events[index], timeoutMs);

	return retVal;
}

DWORD C4JThread::EventArray::WaitForAll(int timeoutMs )
{
	DWORD retVal;
	retVal = WaitForMultipleObjects(m_size, m_events, true, timeoutMs);

	return retVal;
}

DWORD C4JThread::EventArray::WaitForAny(int timeoutMs )
{
	return WaitForMultipleObjects(m_size, m_events, false, timeoutMs);
}





C4JThread::EventQueue::EventQueue( UpdateFunc* updateFunc, ThreadInitFunc threadInitFunc, const char* szThreadName)
{
	m_updateFunc = updateFunc;
	m_threadInitFunc = threadInitFunc;
	strncpy(m_threadName, szThreadName, sizeof(m_threadName) - 1);
	m_threadName[sizeof(m_threadName) - 1] = '\0';
	m_thread = NULL;
	m_startEvent = NULL;
	m_finishedEvent = NULL;
	m_processor = -1;
	m_priority = THREAD_PRIORITY_HIGHEST+1;
}

void C4JThread::EventQueue::init()
{
	m_startEvent = new C4JThread::EventArray(1);
	m_finishedEvent = new C4JThread::Event();
	InitializeCriticalSection(&m_critSect);
	m_thread = new C4JThread(threadFunc, this, m_threadName);
	if(m_processor >= 0)
		m_thread->SetProcessor(m_processor);
	if(m_priority != THREAD_PRIORITY_HIGHEST+1)
		m_thread->SetPriority(m_priority);
	m_thread->Run();
}

void C4JThread::EventQueue::sendEvent( Level* pLevel )
{
	if(m_thread == NULL)
		init();
	EnterCriticalSection(&m_critSect);
	m_queue.push(pLevel);
	m_startEvent->Set(0);
	m_finishedEvent->Clear();
	LeaveCriticalSection(&m_critSect);
}

void C4JThread::EventQueue::waitForFinish()
{
	if(m_thread == NULL)
		init();
	EnterCriticalSection(&m_critSect);
	if(m_queue.empty())
	{
		LeaveCriticalSection((&m_critSect));
		return;
	}
	LeaveCriticalSection((&m_critSect));
	m_finishedEvent->WaitForSignal(INFINITE);
}

int C4JThread::EventQueue::threadFunc( void* lpParam )
{
	EventQueue* p = (EventQueue*)lpParam;
	p->threadPoll();
	return 0;
}

void C4JThread::EventQueue::threadPoll()
{
	ShutdownManager::HasStarted(ShutdownManager::eEventQueueThreads, m_startEvent);

	if(m_threadInitFunc)
		m_threadInitFunc();

	while(ShutdownManager::ShouldRun(ShutdownManager::eEventQueueThreads))
	{

		DWORD err = m_startEvent->WaitForAny(INFINITE);
		if(err == WAIT_OBJECT_0)
		{
			bool bListEmpty = true;
			do 
			{
				EnterCriticalSection(&m_critSect);
				void* updateParam = m_queue.front();
				LeaveCriticalSection(&m_critSect);

				m_updateFunc(updateParam);

				EnterCriticalSection(&m_critSect);
				m_queue.pop();
				bListEmpty = m_queue.empty();
				if(bListEmpty)
				{
					m_finishedEvent->Set();
				}
				LeaveCriticalSection(&m_critSect);

			} while(!bListEmpty);
		}
	};

	ShutdownManager::HasFinished(ShutdownManager::eEventQueueThreads);
}


