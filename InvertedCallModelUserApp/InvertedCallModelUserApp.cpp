#include <Windows.h>
#include <stdio.h>


DWORD WINAPI CompletionPortThread(LPVOID PortHandle);

typedef struct _OVL_WRAPPER
{
    OVERLAPPED  Overlapped;
    LONG        ReturnedSequence;
} OVL_WRAPPER, * POVL_WRAPPER;

#define FILE_DEVICE_INVERTED                    0xCF54
#define IOCTL_REVERSE          CTL_CODE(FILE_DEVICE_INVERTED, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DIRECT      CTL_CODE(FILE_DEVICE_INVERTED, 2050, METHOD_BUFFERED, FILE_ANY_ACCESS)

int main()
{
    HANDLE driverHandle = NULL;
    HANDLE completionPortHandle = NULL;
    HANDLE threadHandle = NULL;
    DWORD  function = 0;

    driverHandle = CreateFile(
        LR"(\\.\Inverted)",
        GENERIC_READ | GENERIC_WRITE,
        0,                          
        nullptr,                     
        OPEN_EXISTING,              
        FILE_FLAG_OVERLAPPED,      
        nullptr
    );                
    if (driverHandle == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed with error 0x%lx \r\n", GetLastError());
        return -1;
    }

    completionPortHandle = CreateIoCompletionPort(
        driverHandle,
        nullptr,
        0,
        0
    );
    if (completionPortHandle == nullptr)
    {
        printf("CreateIoCompletionPort failed with error 0x%lx \r\n", GetLastError());
        return -2;
    }

    threadHandle = CreateThread(
        nullptr,             
        0,                    
        CompletionPortThread, 
        completionPortHandle,  
        0,                 
        nullptr
    );             
    if (threadHandle == nullptr)
    {
        printf("CreateThread failed with error 0x%lx \r\n", GetLastError());
        return -4;
    }

    printf("0. Exit \r\n");
    printf("1. Send IOCTL_REVERSE \r\n");
    printf("2. Simulate a driver event by sending IOCTL_DIRECT. This will reuse a previously pended IOCTL_REVERSE \r\n");

    while (TRUE)
    {
        printf("Selection: ");
        scanf_s("%lx", &function);

        switch (function)
        {
            case 0:
            {
                printf("Byeee!\r\n");
                return 0;
            }
            case 1:
            {
                POVL_WRAPPER wrapper = reinterpret_cast<POVL_WRAPPER>(malloc(sizeof(wrapper)));
                if (NULL != wrapper)
                {
                    RtlZeroMemory(wrapper, sizeof(*wrapper));
                    DeviceIoControl(
                        driverHandle,
                        static_cast<DWORD>(IOCTL_REVERSE),
                        nullptr,                    
                        0,                          
                        &wrapper->ReturnedSequence,  
                        sizeof(LONG),                
                        nullptr,                    
                        &wrapper->Overlapped
                    );
                    if (GetLastError() != ERROR_IO_PENDING)
                    {
                        printf("! Driver did not pend the IOCTL_REVERSE ! Aborting...0x%lx \r\n", GetLastError());
                        return -3;
                    }
                }
                break;
            }
            case 2:
            {
                char message[] = "Hello from user-mode!";

                POVL_WRAPPER wrapper = reinterpret_cast<POVL_WRAPPER>(malloc(sizeof(wrapper)));
                if (NULL != wrapper)
                {
                    RtlZeroMemory(wrapper, sizeof(*wrapper));
                    DeviceIoControl(
                        driverHandle,
                        static_cast<DWORD>(IOCTL_DIRECT),
                        message,
                        sizeof(message),
                        &wrapper->ReturnedSequence, 
                        sizeof(LONG),             
                        nullptr,                     
                        &wrapper->Overlapped
                    ); 
                }
                break;
            }
            default:
            {
                printf("Try again... \r\n");
            }
        }

    }
}

DWORD WINAPI CompletionPortThread(LPVOID PortHandle)
{
    OVERLAPPED* overlapped = nullptr;
    DWORD byteCount = 0;
    ULONG_PTR compKey = 0;

    while (TRUE)
    {
        overlapped = nullptr;
        BOOL worked = GetQueuedCompletionStatus(
            PortHandle,
            &byteCount,  
            &compKey,  
            &overlapped,   
            INFINITE
        );    
        if (overlapped == nullptr || byteCount == 0)
        {
            continue;
        }

        POVL_WRAPPER wrap = reinterpret_cast<POVL_WRAPPER>(overlapped);
        printf(">>> Notification received. Sequence = %ld \r\n", wrap->ReturnedSequence);
    }
}