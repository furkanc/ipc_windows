#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>


#define NO_OF_PROCESS 7
#define PRODUCTS 4
#define PIPE_TIMEOUT 1000
#define _CRT_SECURE_NO_WARNINGS
#define BUFFSIZE 128

void printMostSoldItem(int dayAndProduct[][PRODUCTS], const char* products[]);
void printMostSoldEachDay(int dayAndProduct[][PRODUCTS], const char* products[]);
void printTotalSoldSevenDays(int dayAndProduct[][PRODUCTS], const char* products[]);
void printItemSoldEachDay(int dayAndProduct[][PRODUCTS], const char* products[]);
int comp(const void * elem1, const void * elem2);


int main(int argc, char* argv[])
{
	STARTUPINFO si[NO_OF_PROCESS];
	PROCESS_INFORMATION pi[NO_OF_PROCESS];
	HANDLE processHandles[NO_OF_PROCESS];
	HANDLE pipes[NO_OF_PROCESS];
	HANDLE event[NO_OF_PROCESS];
	OVERLAPPED ovLap[NO_OF_PROCESS];
	LPDWORD numBytesWritten = 0;
	LPDWORD numBytesRead = 0;
	const char* products[] = { "MILK", "BISCUIT", "CHIPS", "COKE" };
	int buffer[BUFFSIZE];
	int dayAndProducts[NO_OF_PROCESS][PRODUCTS];
	char* lpCommandLine[NO_OF_PROCESS] = { "Child.exe", "Child.exe", "Child.exe", "Child.exe", "Child.exe", "Child.exe", "Child.exe" };
	int i = 0;
	char sendingData[20];



	for (i = 0; i < NO_OF_PROCESS; i++)
	{

		SecureZeroMemory(&si[i], sizeof(STARTUPINFO));
		si[i].cb = sizeof(STARTUPINFO);
		SecureZeroMemory(&pi[i], sizeof(PROCESS_INFORMATION));
		SecureZeroMemory(&ovLap[i], sizeof(OVERLAPPED));


		pipes[i] = CreateNamedPipe("\\\\.\\Pipe\\DataBus",
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, // read/write access and overlapped mode
			PIPE_TYPE_BYTE, // byte-type pipe
			NO_OF_PROCESS, // number of instances
			BUFFSIZE * sizeof(int), // output buffer size
			BUFFSIZE * sizeof(int), // input buffer size
			NMPWAIT_USE_DEFAULT_WAIT, // client time-out
			NULL); //default security attributes


		//check error while creating pipe
		if (pipes[i] == INVALID_HANDLE_VALUE || pipes[i] == NULL)
		{
			printf("Cannot Create pipe. Error code : %d \n", GetLastError());
			system("pause");
			return 0;
		}

		// Create an event object for this instance
		if ((event[i] = CreateEvent(NULL, TRUE, TRUE, NULL)) == NULL)

		{

			printf("CreateEvent() for pipe %d failed with error %d\n", i, GetLastError());
			system("pause");
			return 0;
		}

		ovLap[i].hEvent = event[i];


		if (!CreateProcess(NULL,
			lpCommandLine[i],
			NULL,
			NULL,
			TRUE,
			CREATE_NEW_CONSOLE,
			NULL,
			NULL,
			&si[i],
			&pi[i]))
		{
			printf("unable to create process: %d\n", i);
			system("pause");
			ExitProcess(0);
		}
		else{	processHandles[i] = pi[i].hProcess; }

		// check another pipe connected at that time.And connect when the other I/O operation 
		while (1) {
			if (ConnectNamedPipe(pipes[i], NULL) != FALSE)
			{
				sprintf_s(sendingData, sizeof(sendingData), " %d#", i + 1);
				if (!WriteFile(pipes[i], sendingData, sizeof(sendingData) * sizeof(char), &numBytesWritten, &ovLap[i]))
				{
					printf("Cannot send a message.Error Code: %d \n", GetLastError());
				}
				else { break; }
			}
			else
			{
				printf("Cannot connect pipe. Error No: %d", GetLastError());
			}
		}
		while (1) {
			if (!ReadFile(pipes[i], &buffer, 128 * sizeof(TCHAR), &numBytesRead, &ovLap[i]))
			{
				//since working with asynchronous file handles, ReadFile function will return immediately even if operation has not been completed.
				//However, its not a failure. It labels that the read operation is pending comletion asynchronously.
				//Because Readfile may return before the system-level read operation is complete.
				//So ReadFile has to wait until system signals that I/O operation is complete
				//Check more on:
				//
				if (GetLastError() == ERROR_IO_PENDING)	continue;
				else
				{
					for (size_t j = 0; j < PRODUCTS; j++) { dayAndProducts[i][j] = buffer[j]; }
					break;
				}
			}
		}
	}

	WaitForMultipleObjects(NO_OF_PROCESS, processHandles, TRUE, INFINITE);
	
	for (i = 0; i < NO_OF_PROCESS; i++)
	{
		CloseHandle(pi[i].hThread);
		CloseHandle(pi[i].hProcess);
	}
	
	printMostSoldItem(dayAndProducts, products);
	printMostSoldEachDay(dayAndProducts,products);
	printTotalSoldSevenDays(dayAndProducts, products);
	printItemSoldEachDay(dayAndProducts, products);
	
	system("pause");
	return 1;
}

void printMostSoldItem(int dayAndProduct[][PRODUCTS], const char* products[])
{
	int totalSold[4] = { 0,0,0,0 };
	int mostSoldindex = 0;
	for (size_t i = 0; i <NO_OF_PROCESS; i++)
	{
		totalSold[0] += dayAndProduct[i][0];
		totalSold[1] += dayAndProduct[i][1];
		totalSold[2] += dayAndProduct[i][2];
		totalSold[3] += dayAndProduct[i][3];
	}

	for (size_t i = 1; i < PRODUCTS; i++)
	{
		if (totalSold[mostSoldindex] < totalSold[i])
		{
			mostSoldindex = i;
		}

		
	}

	printf("************** ");
	printf("THE MOST SOLD ITEM IN 7 DAYS : %s SOLD %d TIMES",products[mostSoldindex],totalSold[mostSoldindex]);
	printf("**************\n");
}

void printMostSoldEachDay(int dayAndProduct[][PRODUCTS], const char* products[])
{

	printf("************** ");
	printf("THE MOST SOLD ITEM IN EACH DAY");
	printf("**************\n");
	int indexOfMostSold = 0;
	for (size_t i = 0; i < NO_OF_PROCESS; i++)
	{

		for (size_t j = 1; j < PRODUCTS; j++)
		{
			if (dayAndProduct[i][indexOfMostSold] < dayAndProduct[i][j])
			{
				indexOfMostSold = j;
			}
		}


		printf("DAY %d : ", i + 1);
		for (size_t j = 0; j < PRODUCTS; j++)
		{
			if (dayAndProduct[i][indexOfMostSold] == dayAndProduct[i][j])
			{
				printf("%s, ", products[j]);
			}
		}
		if (dayAndProduct[i][indexOfMostSold] <2)
		{
			printf("SOLD %d TIME\n", dayAndProduct[i][indexOfMostSold]);
		}
		else { printf("SOLD %d TIMES\n", dayAndProduct[i][indexOfMostSold]); }
		indexOfMostSold = 0;
}
	


}

void printTotalSoldSevenDays(int dayAndProduct[][PRODUCTS], const char* products[])
{
	int totalSold[4] = { 0,0,0,0 };
	int mostSoldindex = 0;
	for (size_t i = 0; i < NO_OF_PROCESS; i++)
	{
		totalSold[0] += dayAndProduct[i][0];
		totalSold[1] += dayAndProduct[i][1];
		totalSold[2] += dayAndProduct[i][2];
		totalSold[3] += dayAndProduct[i][3];
	}
	printf("************** ");
	printf("THE TOTAL NUMBER OF ITEM SOLD IN SEVEN DAYS ");
	printf("**************\n");
	for (size_t i = 0; i < PRODUCTS; i++)
	{
		if (totalSold[i] < 2) {
			printf("%s SOLD %d TIME \n", products[i], totalSold[i]);
		}
		else
		{
			printf("%s SOLD %d TIMES \n", products[i], totalSold[i]);
		}
	}
}

void printItemSoldEachDay(int dayAndProduct[][PRODUCTS], const char* products[])
{

	printf("************** ");
	printf("THE TOTAL NUMBER OF ITEM SOLD IN EACH DAY ");
	printf("**************\n");
	for (size_t i = 0; i < NO_OF_PROCESS; i++)
	{
		printf(" -----  DAY %d ----- \n", i + 1);
		for (size_t j = 0; j < PRODUCTS; j++)
		{
			if (dayAndProduct[i][j] < 2) {
				if (dayAndProduct[i][j] == 1)
				{
					printf("%s SOLD %d TIME \n", products[j], dayAndProduct[i][j]);
				}
				else{ printf("%s NEVER SOLD \n", products[j]); }
			}
			else
			{
				printf("%s SOLD %d TIMES \n", products[j], dayAndProduct[i][j]);
			}
		}
	}
}