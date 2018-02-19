#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#define THREAD_COUNT 4
int readAndCount(const char *product,const char *day);




int readAndCount(const char *product,const char *day) {
	FILE *stream;
	errno_t err;
	char line[150];
	size_t len = 100;
	char *token = NULL;
	char *nextToken = NULL;
	int count = 0;
	char start[15] = "#START DAY";
	char end[15] = "#END DAY";
	err = fopen_s(&stream, TEXT("../market.txt"), "r");

	if (err != 0)
	{
		printf("error when opening file\n");
		system("pause");
		return 0;

	}
	strcat_s(start, strlen(start)+strlen(day)+1,day);
	strcat_s(end, strlen(end)+strlen(day)+1, day);
	
	while (fgets(line, len, stream) != NULL) {
		if (strstr(line, start) != NULL)
		{
			while (fgets(line, len, stream) != NULL) {
				token = strtok_s(line, ",", &nextToken);
				while ((token != NULL))
				{
					if (strstr(token, product) != NULL) count++;
					token = strtok_s(NULL, ",", &nextToken);
				}
				if (strstr(line, end) != NULL) break;
			}
		}
	}
	return count;
}

typedef struct
{
	const char *day;
	const char *productName; 
	int threadNo;
	int nmbOfProduct;
}THREAD_PARAMETERS;

DWORD WINAPI threadWork(LPVOID parameters);

int main(int argc, char* argv[])
{
	const char* products[] = { "MILK", "BISCUIT", "CHIPS", "COKE" };
	int pid = 0;
	int productArray[4];
	int* threadID;
	int i = 0;
	HANDLE* handles;
	HANDLE pipe;
	THREAD_PARAMETERS* lpParameter;
	BOOL fSuccess = FALSE;
	DWORD dwMode, BytesToRead , BytesToWrite;
	TCHAR buffer[64];
	
	//allocate memory for every parameters needed
	handles = malloc(sizeof(HANDLE)* THREAD_COUNT);
	lpParameter = malloc(sizeof(THREAD_PARAMETERS)* THREAD_COUNT);
	threadID = malloc(sizeof(int)* THREAD_COUNT);

	//Open the named pipe
	pipe = CreateFile("\\\\.\\Pipe\\DataBus",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	
	if (pipe == INVALID_HANDLE_VALUE)

	{
		printf(TEXT("Cannot open the name pipe. Error Code : %d\n"), GetLastError());
		system("pause");
		return -1;
	}

	dwMode = PIPE_READMODE_BYTE;
	fSuccess = SetNamedPipeHandleState(
		pipe,
		&dwMode,
		NULL,
		NULL);

	if (!fSuccess)
	{
		printf(TEXT("SetNamedPipeHandleState failed. Error Code : %d\n"), GetLastError());
		system("pause");
		return -1;
	}

	fSuccess = ReadFile(pipe,
		buffer,
		128*sizeof(TCHAR),
		&BytesToRead,
		NULL);
	
	if (!fSuccess)
	{
		printf(TEXT("Reading file failed. Error Code : %d\n"), GetLastError());
		system("pause");
		return -1;
	}

	//for each thread
	for (i = 0; i < THREAD_COUNT; i++)
	{
		//initialize parameters
		lpParameter[i].nmbOfProduct = 0;
		lpParameter[i].productName = products[i];
		lpParameter[i].threadNo = i + 1;
		lpParameter[i].day = buffer;
		
		//create thread
		handles[i] = CreateThread(NULL, 0, threadWork, &lpParameter[i], 0, &threadID[i]);
		
		//check errors in creation
		if (handles[i] == INVALID_HANDLE_VALUE)
		{
			printf("error when creating thread\n");
			system("pause");
			exit(0);
		}
		printf("thread %d has started working with id: %d\n", i + 1, threadID[i]);
	}

	WaitForMultipleObjects(THREAD_COUNT, handles, TRUE,INFINITE); // wait for all threads to do their works
	
	productArray[0] = lpParameter[0].nmbOfProduct;
	productArray[1] = lpParameter[1].nmbOfProduct;
	productArray[2] = lpParameter[2].nmbOfProduct;
	productArray[3] = lpParameter[3].nmbOfProduct;

	//Write data to parent
	if (!WriteFile(pipe,productArray, sizeof(productArray) * sizeof(int), &BytesToWrite, NULL)); 
	{
		if (GetLastError() != ERROR_SUCCESS)
		{
			printf("Cannot send a message.Error Code: %d \n", GetLastError());
			system("pause");
			return 0;
		}
	}

	free(handles);
	free(lpParameter);
	free(threadID);
	CloseHandle(pipe);

	
	return 1;
}


DWORD WINAPI threadWork(LPVOID parameters)
{
	THREAD_PARAMETERS* param = (THREAD_PARAMETERS*)parameters;

	int i = 0;
	int count = 0;

	count = readAndCount(param->productName, param ->day);
	param ->nmbOfProduct = count;
	return 1;
}