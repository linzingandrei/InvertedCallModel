#include <ntddk.h> 
#include <wdf.h>


DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD InvertedEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL InvertedEvtIoDeviceControl;

typedef struct _INVERTED_DEVICE_CONTEXT {
    WDFQUEUE    NotificationQueue;
} INVERTED_DEVICE_CONTEXT, * PINVERTED_DEVICE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INVERTED_DEVICE_CONTEXT, InvertedGetContextFromDevice)

typedef struct _REQUEST_CONTEXT {
    CHAR        Buffer[1005];
} REQUEST_CONTEXT, * PREQUEST_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, GetRequestContext)

#define FILE_DEVICE_INVERTED                    0xCF54
#define IOCTL_REVERSE     CTL_CODE(FILE_DEVICE_INVERTED, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DIRECT      CTL_CODE(FILE_DEVICE_INVERTED, 2050, METHOD_BUFFERED, FILE_ANY_ACCESS)

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    WDF_DRIVER_CONFIG config = { 0 };
    WDF_DRIVER_CONFIG_INIT(&config, InvertedEvtDeviceAdd);

    NTSTATUS status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );
    if (!NT_SUCCESS(status))
    {
        __debugbreak();
        return status;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
InvertedEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    WDF_OBJECT_ATTRIBUTES objAttributes = { 0 };
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    WDFDEVICE device = { 0 };
    PINVERTED_DEVICE_CONTEXT devContext = NULL;
    WDF_IO_QUEUE_CONFIG queueConfig = { 0 };

    DECLARE_CONST_UNICODE_STRING(userDeviceName, L"\\Global??\\Inverted");
    UNREFERENCED_PARAMETER(Driver);

    __debugbreak();

    WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objAttributes, INVERTED_DEVICE_CONTEXT);

    status = WdfDeviceCreate(
        &DeviceInit,
        &objAttributes,
        &device
    );
    if (!NT_SUCCESS(status))
    {
        __debugbreak();
        return status;
    }

    devContext = InvertedGetContextFromDevice(device);

    status = WdfDeviceCreateSymbolicLink(
        device,
        &userDeviceName
    );
    if (!NT_SUCCESS(status))
    {
        __debugbreak();
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoDeviceControl = InvertedEvtIoDeviceControl;
    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(
        device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE
    );
    if (!NT_SUCCESS(status))
    {
        __debugbreak();
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(
        device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->NotificationQueue
    );
    if (!NT_SUCCESS(status))
    {
        __debugbreak();
        return status;
    }

    return STATUS_SUCCESS;
}



VOID
InvertedEvtNotify(
    _In_ PINVERTED_DEVICE_CONTEXT Context
)
{
    __debugbreak();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    WDFREQUEST notifyRequest = NULL;

    PCHAR bufferPointer = NULL;
    size_t bufferLength = 0;
    ULONG_PTR info = 0;

    status = WdfIoQueueRetrieveNextRequest(
        Context->NotificationQueue,
        &notifyRequest
    );
    if (!NT_SUCCESS(status))
    {
        DbgPrintEx(0, 0, "No pending requests!");

        return;
    }

    PREQUEST_CONTEXT requestContext = NULL;
    requestContext = GetRequestContext(notifyRequest);

    status = WdfRequestRetrieveOutputBuffer(
        notifyRequest,
        sizeof(LONG),
        (PVOID*)&bufferPointer,
        &bufferLength
    );
    if ((!NT_SUCCESS(status)) || (bufferLength < sizeof(requestContext->Buffer)))
    {
        status = STATUS_SUCCESS;
        info = 0;
    }
    else
    {
        requestContext->Buffer[1000] = 'A';
        requestContext->Buffer[1001] = 'C';
        requestContext->Buffer[1002] = 'K';
        requestContext->Buffer[1003] = '\0';

        RtlCopyMemory(bufferPointer, requestContext->Buffer, sizeof(requestContext->Buffer));

        status = STATUS_SUCCESS;
        info = sizeof(requestContext->Buffer);
    }

    WdfRequestCompleteWithInformation(notifyRequest, status, info);
}

_Use_decl_annotations_
VOID
InvertedEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    PINVERTED_DEVICE_CONTEXT devContext = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    devContext = InvertedGetContextFromDevice(WdfIoQueueGetDevice(Queue));
    __debugbreak();

    switch (IoControlCode)
    {
        case IOCTL_REVERSE:
        {
            PCHAR inputBuffer = NULL;
            size_t inputBufferLength = 0;
            WDF_OBJECT_ATTRIBUTES requestAttributes;
            PREQUEST_CONTEXT requestContext = NULL;

            status = WdfRequestRetrieveInputBuffer(
                Request,
                1,
                (PVOID*)&inputBuffer,
                &inputBufferLength
            );
            if (!NT_SUCCESS(status))
            {
                WdfRequestCompleteWithInformation(
                    Request,
                    status,
                    0
                );
                break;
            }

            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, REQUEST_CONTEXT);
            status = WdfObjectAllocateContext(
                Request,
                &requestAttributes,
                (PVOID*)&requestContext
            );
            if (!NT_SUCCESS(status))
            {
                WdfRequestCompleteWithInformation(
                    Request,
                    status,
                    0
                );
                break;
            }

            RtlZeroMemory(requestContext->Buffer, sizeof(requestContext->Buffer));
            RtlCopyMemory(requestContext->Buffer, inputBuffer, inputBufferLength);

            status = WdfRequestForwardToIoQueue(
                Request,
                devContext->NotificationQueue
            );
            if (!NT_SUCCESS(status))
            {
                WdfRequestCompleteWithInformation(
                    Request,
                    status,
                    0
                );
            }
            else
            {

                return;
            }
        }
        case IOCTL_DIRECT:
        {
            PCHAR inputBuffer = NULL;
            size_t inputLength = 0;

            status = WdfRequestRetrieveInputBuffer(
                Request,
                1,
                (PVOID*)&inputBuffer,
                &inputLength
            );
            if (!NT_SUCCESS(status))
            {
                WdfRequestCompleteWithInformation(
                    Request,
                    status,
                    0
                );
                break;
            }

            DbgPrintEx(0, 0, "[DIRECT] Received: %s\n", inputBuffer);

            InvertedEvtNotify(devContext);
            WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
            break;
        }
        default:
        {
            WdfRequestCompleteWithInformation(Request, STATUS_NOT_SUPPORTED, 0);
            NT_ASSERT(FALSE);
        }
    }
}