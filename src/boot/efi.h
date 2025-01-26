/********************************************************************************************************************
 * File: efi.h
 * Author: xWatexx (aka alobley)
 * Created: 01/25/2025
 * Last Modified: 01/25/2025
 * Version: 0.1 Alpha
 * 
 * Description:
 *    This file contains the EFI structures and definitions required to interact with the UEFI firmware. 
 *    It's a compact and efficient library.
 * 
 * 
 * Dependencies: UEFI-supported compiler (Recommended: clang)
*********************************************************************************************************************/
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

typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;

typedef void VOID;

typedef struct _EFI_GUID {
    UINT32 TimeLow;
    UINT16 TimeMid;
    UINT16 TimeHighVer;
    UINT8 ClockSeqHi;
    UINT8 ClockSeqLow;
    UINT8 Node[6];
} __attribute__((packed)) EFI_GUID;

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042a9de, 0x23dc, 0x4a38, 0x96, 0xfb, {0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}}

typedef UINTN EFI_STATUS;

#define EFI_SUCCESS 0ULL

#define HIBIT_SET 0x8000000000000000ULL
#define ENCODE_ERROR(a) (HIBIT_SET | (a))
#define EFI_ERROR(a) (((INTN)((UINTN)(a))) < 0)         // Simple and easy way to check if an error occurred, just check the sign bit

#define EFI_UNSUPPORTED ENCODE_ERROR(3)
#define EFI_DEVICE_ERROR ENCODE_ERROR(7)

typedef VOID* EFI_HANDLE;

typedef VOID* EFI_EVENT;
#define EVT_TIMER 0x80000000
#define EVT_RUNTIME 0x40000000
#define EVT_NOTIFY_WAIT 0x00000100
#define EVT_NOTIFY_SIGNAL 0x00000200
#define EVT_SIGNAL_EXIT_BOOT_SERVICES 0x00000201
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202

typedef UINT64 EFI_LBA;

typedef UINTN EFI_TPL;

typedef struct _EFI_TABLE_HEADER {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;


// Text output protocol
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

// Set the text color
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This,
    IN UINTN Attribute
);

// Query the mode of the text output device
typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This,
    IN UINTN ModeNumber,
    OUT UINTN* Columns,
    OUT UINTN* Rows
);

// Set the mode of the text output device
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_MODE)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This,
    IN UINTN ModeNumber
);

// Set the cursor position
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This,
    IN UINTN Column,
    IN UINTN Row
);

// Text mode
typedef struct {
    INT32 MaxMode;
    INT32 Mode;
    INT32 Attribute;
    INT32 CursorColumn;
    INT32 CursorRow;
    BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

// EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET Reset;
    EFI_TEXT_STRING OutputString;
    void* TestString;
    EFI_TEXT_QUERY_MODE QueryMode;
    EFI_TEXT_SET_MODE SetMode;
    EFI_TEXT_SET_ATTRIBUTE SetAttribute;                 // Change colors
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    void* EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE* Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;


// Text input protocol (keyboard)
typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET)(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL* This,
    IN BOOLEAN ExtendedVerification
);

typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL* This,
    OUT EFI_INPUT_KEY* Key
);

typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_INPUT_RESET Reset;
    EFI_INPUT_READ_KEY ReadKeyStroke;
    EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

// Graphics output protocol
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
    UINT32 RedMask;
    UINT32 GreenMask;
    UINT32 BlueMask;
    UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info;
    UINTN SizeOfInfo;
    UINTN FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL* This,
    IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL* BltBuffer OPTIONAL,
    IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN SourceX,
    IN UINTN SourceY,
    IN UINTN DestinationX,
    IN UINTN DestinationY,
    IN UINTN Width,
    IN UINTN Height,
    IN UINTN Delta OPTIONAL
);

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL* This,
    IN UINT32 ModeNumber,
    OUT UINTN* SizeOfInfo,
    OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION** Info
);

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL* This,
    IN UINT32 ModeNumber
);

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

// Runtime Services
typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef VOID (EFIAPI *EFI_RESET_SYSTEM)(
    IN EFI_RESET_TYPE ResetType,
    IN EFI_STATUS ResetStatus,
    IN UINTN DataSize,
    IN CHAR16* ResetData OPTIONAL
);

typedef struct {
    EFI_TABLE_HEADER Hdr;
    void* GetTime;
    void* SetTime;
    void* GetWakeupTime;
    void* SetWakeupTime;
    void* SetVirtualAddressMap;
    void* ConvertPointer;
    void* GetVariable;
    void* GetNextVariableName;
    void* SetVariable;
    void* GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM ResetSystem;
    void* UpdateCapsule;
    void* QueryCapsuleCapabilities;
    void* QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

// Boot services
typedef EFI_STATUS (EFIAPI *EFI_WAIT_FOR_EVENT)(
    IN UINTN NumberOfEvents,
    IN EFI_EVENT* Event,
    OUT UINTN* Index
);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
    IN EFI_GUID* Protocol,
    IN VOID* Registration OPTIONAL,
    OUT VOID** Interface
);

typedef struct {
    EFI_TABLE_HEADER Hdr;
    void* RaiseTPL;
    void* RestoreTPL;
    void* AllocatePages;
    void* FreePages;
    void* GetMemoryMap;
    void* AllocatePool;                                         // Allocates pool memory (literally just malloc)
    void* FreePool;                                             // Frees pool memory (literally just free)
    void* CreateEvent;
    void* SetTimer;
    EFI_WAIT_FOR_EVENT WaitForEvent;
    void* SignalEvent;
    void* CloseEvent;
    void* CheckEvent;
    void* InstallProtocolInterface;
    void* ReinstallProtocolInterface;
    void* UninstallProtocolInterface;
    void* HandleProtocol;
    VOID* Reserved;
    void* RegisterProtocolNotify;
    void* LocateHandle;
    void* LocateDevicePath;
    void* InstallConfigurationTable;
    void* LoadImage;                                            // Allows the loading of an EFI image (another UEFI application)
    void* StartImage;
    void* Exit;
    void* UnloadImage;
    void* ExitBootServices;                                     // Exits the boot services (required before loading and running a kernel)
    void* GetNextMonotonicCount;
    void* Stall;
    void* SetWatchdogTimer;
    void* ConnectController;
    void* DisconnectController;
    void* OpenProtocol;
    void* CloseProtocol;
    void* OpenProtocolInformation;
    void* ProtocolsPerHandle;
    void* LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL LocateProtocol;
    void* InstallMultipleProtocolInterfaces;
    void* UninstallMultipleProtocolInterfaces;
    void* CalculateCrc32;
    void* CopyMem;
    void* SetMem;
    void* CreateEventEx;
} EFI_BOOT_SERVICES;

// The system table
typedef struct _EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER Hdr;
    CHAR16* FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;
    EFI_RUNTIME_SERVICES* RuntimeServices;
    EFI_BOOT_SERVICES* BootServices;
    UINTN NumberOfTableEntries;
    //EFI_CONFIGURATION_TABLE* ConfigurationTable;
    void* ConfigurationTable;
} EFI_SYSTEM_TABLE;

typedef EFI_STATUS (EFIAPI *EFI_IMAGE_ENTRY_POINT)(
    IN EFI_HANDLE ImageHandle, 
    IN EFI_SYSTEM_TABLE* SystemTable
);

#define EFI_BLACK 0x00
#define EFI_BLUE 0x01
#define EFI_GREEN 0x02
#define EFI_CYAN 0x03
#define EFI_RED 0x04
#define EFI_MAGENTA 0x05
#define EFI_BROWN 0x06
#define EFI_LIGHTGRAY 0x07
#define EFI_BRIGHT 0x08
#define EFI_DARKGRAY 0x08
#define EFI_LIGHTBLUE 0x09
#define EFI_LIGHTGREEN 0x0A
#define EFI_LIGHTCYAN 0x0B
#define EFI_LIGHTRED 0x0C
#define EFI_LIGHTMAGENTA 0x0D
#define EFI_YELLOW 0x0E
#define EFI_WHITE 0x0F

#define EFI_BACKGROUND_BLACK 0x00
#define EFI_BACKGROUND_BLUE 0x10
#define EFI_BACKGROUND_GREEN 0x20
#define EFI_BACKGROUND_CYAN 0x30
#define EFI_BACKGROUND_RED 0x40
#define EFI_BACKGROUND_MAGENTA 0x50
#define EFI_BACKGROUND_BROWN 0x60
#define EFI_BACKGROUND_LIGHTGRAY 0x70

#define EFI_TEXT_ATTR(Foreground, Background) ((Foreground) | ((Background) << 4))

// TODO: Look at the spec and replace with typedef enum or something
#define KEY_UP 0x01
#define KEY_DOWN 0x02
#define KEY_ESC 0x17

#endif
