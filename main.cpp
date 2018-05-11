// This is a really simple tutorial on how to make multiple kernel-level threads in c++
// I put everything in the one file for simplicity.

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <strsafe.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

// Macros
#define BUFF_SIZE 256
#define ARRAY_SIZE	100000
#define NUM_THREADS 4

// Function Declarations
DWORD WINAPI myThreadFunction(LPVOID lpParam);
void ErrorHandler(LPTSTR lpszFunction);

// Globals
HANDLE printRubberChicken; // We will need a mutex for our thread prints
std::vector <int> myArray (ARRAY_SIZE); // as an example, let's just create and sort a huge random array


int main(int argc, char* argv)
{
	// Fill our array with some random ints
	srand(time(NULL)); // seed a random generator with the current system time
	for (auto i = 0; i < ARRAY_SIZE; i++)
	{
		auto num = rand() % ARRAY_SIZE + 1;
		myArray[i] = num;
	}

	// Arrays to store our thread details
	DWORD   threadId[NUM_THREADS];
	HANDLE  threadHandle[NUM_THREADS];


	// Create some threads
	for (int i = 0; i < NUM_THREADS; i++)
	{
		
		threadHandle[i] = CreateThread
		(
			NULL,                   // default security attributes
			0,                      // stack size, usually 0 (default)
			&myThreadFunction,      // the function to pass to each thread
			LPVOID(i),				// arguments to the thread function
			0,                      // thread creation flags, usually 0 (default) 
			&threadId[i]			// after the thread is created, store the thread ID in this variable 
		);


		// Check for errors from CreateThread
		if (threadHandle[i] == NULL)
		{
			ErrorHandler(LPTSTR("Error in CreateThread"));
			ExitProcess(1);
		}

	}

	// Make our rubber chicken
	printRubberChicken = CreateMutex
	(
		NULL,			// default security attributes
		FALSE,			// initially not owned by any thread
		NULL			// unnamed mutex
	);             
	// bwak bwak bwak
	if (printRubberChicken == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		return 1;
	}
	
	// wait on your thread (or threads, if you have multiple) to finish.
	WaitForMultipleObjects(NUM_THREADS, threadHandle, TRUE, INFINITE);

	// Close thread handles
	for (int i = 0; i < NUM_THREADS; i++)
	{
		// Close each thread handle
		CloseHandle(threadHandle[i]);
	}

	CloseHandle(printRubberChicken);

	return 0;
}


DWORD WINAPI myThreadFunction(LPVOID lpParam)
{
	TCHAR msgBuf[256];
	size_t cchStringSize;
	DWORD dwChars;

	int threadNumber = int(lpParam); // a really dirty way to get the thread number.

	// Make sure there is a console to print to.
	HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (stdoutHandle == INVALID_HANDLE_VALUE)
		return 1;

	// Sort our array via brute forcing it
	for (auto i = (ARRAY_SIZE / NUM_THREADS * threadNumber); i < (ARRAY_SIZE / NUM_THREADS * (threadNumber + 1)); i++)
	{
		for (auto j = 0; j < ARRAY_SIZE - 1; j++)
		{
			if (myArray[j] > myArray[j + 1])
			{
				auto temp = myArray[j];
				myArray[j] = myArray[j + 1];
				myArray[j + 1] = temp;
			}
		}
	}

	// Each thread print the results
	// claim the mutex
	auto waitForMutex = WaitForSingleObject
	(
		printRubberChicken,    // handle to mutex
		INFINITE				// no time-out interval
	);
	if (waitForMutex == WAIT_OBJECT_0)
	{
		for (auto i = 0; i < 10; i++)
		{
			// Print the parameter values using Windows "thread-safe functions". Wonderful. Thanks, Microsoft. 
			StringCchPrintf(msgBuf, BUFF_SIZE, TEXT("Thread %d: Array[%d] = %d\n"), threadNumber, i, myArray[i]);
			StringCchLength(msgBuf, BUFF_SIZE, &cchStringSize);
			WriteConsole(stdoutHandle, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
		}
		// release that mutex
		if (!ReleaseMutex(printRubberChicken))
		{
			// Error handling
			StringCchPrintf(msgBuf, BUFF_SIZE, TEXT("Failed to release mutex"));
			StringCchLength(msgBuf, BUFF_SIZE, &cchStringSize);
			WriteConsole(stdoutHandle, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
		}
		else
		{
			StringCchPrintf(msgBuf, BUFF_SIZE, TEXT("Mutex Released \n"));
			StringCchLength(msgBuf, BUFF_SIZE, &cchStringSize);
			WriteConsole(stdoutHandle, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
		}
	}
	else
	{
		// Error handling
		StringCchPrintf(msgBuf, BUFF_SIZE, TEXT("Failed to claim mutex"));
		StringCchLength(msgBuf, BUFF_SIZE, &cchStringSize);
		WriteConsole(stdoutHandle, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
	}

	return 0;
}


void ErrorHandler(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code.

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage
	(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL
	);

	// Display the error message.

	lpDisplayBuf = (LPVOID)LocalAlloc
	(	LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
		StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	// Free error-handling buffer allocations.

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}
