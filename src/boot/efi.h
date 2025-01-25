#ifndef EFI_H
#define EFI_H 1

#define EFIAPI __attribute__((ms_abi))

#define IN
#define OUT
#define OPTIONAL
#define CONST const

typedef unsigned char BOOLEAN;
#define FALSE 0
#define TRUE 1

typedef signed long long INTN;
typedef unsigned long long UINTN;

typedef signed char INT8;
typedef unsigned char UINT8;

typedef signed short INT16;
typedef unsigned short UINT16;

typedef signed int INT32;
typedef unsigned int UINT32;

typedef signed long long INT64;
typedef unsigned long long UINT64;

typedef char CHAR8;

typedef unsigned short CHAR16;

typedef void VOID;

typedef struct _EFI_GUID {
    UINT32 TimeLow;
    UINT16 TimeMid;
    UINT16 TimeHighVer;
    UINT8 ClockSeqHi;
    UINT8 ClockSeqLow;
    UINT8 Node[6];
} __attribute__((packed)) EFI_GUID;

typedef UINTN EFI_STATUS;

#define EFI_SUCCESS 0ULL

typedef VOID* EFI_HANDLE;

typedef VOID* EFI_EVENT;

typedef UINT64 EFI_LBA;

typedef UINTN EFI_TPL;

typedef struct _EFI_TABLE_HEADER {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// Reset the text output device
typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This,
    IN BOOLEAN ExtendedVerification
);

// Print a string to the screen
typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This,
    IN CHAR16* String
);

// Clear the screen
typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This
);

// EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET Reset;
    EFI_TEXT_STRING OutputString;
    void* TestString;
    void* QueryMode;
    void* SetMode;
    void* SetAttribute;                 // Change colors
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    void* SetCursorPosition;
    void* EnableCursor;
    void* Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// EFI_SYSTEM_TABLE
typedef struct _EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER Hdr;
    CHAR16* FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    //EFI_SIMPLE_TEXT_INPUT_PROTOCOL* ConIn;
    void* ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;
    //EFI_RUNTIME_SERVICES* RuntimeServices;
    void* RuntimeServices;
    //EFI_BOOT_SERVICES* BootServices;
    void* BootServices;
    UINTN NumberOfTableEntries;
    //EFI_CONFIGURATION_TABLE* ConfigurationTable;
    void* ConfigurationTable;
} EFI_SYSTEM_TABLE;

typedef EFI_STATUS (EFIAPI *EFI_IMAGE_ENTRY_POINT)(
    IN EFI_HANDLE ImageHandle, 
    IN EFI_SYSTEM_TABLE* SystemTable
);

#endif
