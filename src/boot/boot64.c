/********************************************************************************************************************
 * File: boot64.c
 * Author: xWatexx (aka alobley)
 * Created: 01/25/2025
 * Last Modified: 01/25/2025
 * Version: 0.1 Alpha
 * 
 * Description:
 *    This file is the source code for my custom AMD64 UEFI boot manager. It is meant to be a simple, portable 
 *    and efficient.
 * 
 * 
 * Dependencies: UEFI-supported compiler (Recommended: clang)
*********************************************************************************************************************/
#include "bootutil.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define COLOR_THEME_COLORFUL EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLUE)                           // Default color theme
#define COLOR_THEME_DARK EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)                               // Dark color theme (classic)

#define COLORFUL_HILIGHT_COLOR EFI_TEXT_ATTR(EFI_BLUE, EFI_CYAN)                           // Default highlighted text color
#define DARK_HILIGHT_COLOR EFI_TEXT_ATTR(EFI_BACKGROUND_LIGHTGRAY, EFI_BLACK)                             // Dark highlighted text color

UINT16 currentTheme = COLOR_THEME_COLORFUL;
UINT16 currentHilight = COLORFUL_HILIGHT_COLOR;

void printf(CHAR16* fmt, ...);
BOOLEAN PrintNum(UINTN num, UINT8 base, BOOLEAN s);

EFI_SYSTEM_TABLE* st;
EFI_HANDLE image;

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* cout;
EFI_SIMPLE_TEXT_INPUT_PROTOCOL* cin;
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* cerr;

EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;

EFI_BOOT_SERVICES* bs;
EFI_RUNTIME_SERVICES* rs;

INT32 originalTextMode = 0;
INT32 bestMode = 0;

BOOLEAN isGraphicsMode = TRUE;

// Wait for any key to be pressed and return it
EFI_INPUT_KEY GetKey(void){
    EFI_EVENT events[1];
    EFI_INPUT_KEY key;
    key.ScanCode = 0;
    key.UnicodeChar = u'\0';

    events[0] = cin->WaitForKey;

    UINTN index = 0;
    bs->WaitForEvent(1, &events[0], &index);
    if(index == 0){
        cin->ReadKeyStroke(cin, &key);
        return key;
    }

    return key;
}

void PrintConsoleAttributes(void){
    UINTN cols = 0;
    UINTN rows = 0;

    cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);

    // Print the current console info to the screen
    printf(u"Console Info:\r\n");
    printf(u"     Current Text Mode: %d (%dx%d)\r\n", cout->Mode->Mode, cols, rows);
    printf(u"     Max Mode: %d\r\n", cout->Mode->MaxMode);
    printf(u"     Attribute: 0x%x\r\n", cout->Mode->Attribute);
    printf(u"     Cursor Column: %d\r\n", cout->Mode->CursorColumn);
    printf(u"     Cursor Row: %d\r\n", cout->Mode->CursorRow);
    printf(u"     Cursor Visible: %d\r\n\r\n", cout->Mode->CursorVisible);

    // Print all available text modes
    printf(u"Available text modes: \r\n");
    INT32 max = cout->Mode->MaxMode;
    for(INT32 i = 0; i < max; i++){
        cout->QueryMode(cout, i, &cols, &rows);
        if(cols == 0 || rows == 0){
            printf(u"     Mode %d: INVALID MODE!\r\n", i);
        }else{
            printf(u"     Mode %d: %dx%d\r\n", i, cols, rows);
        }
    }
}

// Find the highest available text mode and switch to it
EFI_STATUS SetHighestTextMode(){
    UINTN maxCols = 0;
    UINTN maxRows = 0;
    UINTN cols = 0;
    UINTN rows = 0;
    EFI_STATUS status = EFI_SUCCESS;

    originalTextMode = cout->Mode->Mode;

    for(INT32 i = 0; i < cout->Mode->MaxMode; i++){
        cout->QueryMode(cout, i, &cols, &rows);
        if(cols == 0 || rows == 0 || EFI_ERROR(status)){
            continue;
        }

        if(cols > maxCols && rows > maxRows){
            maxCols = cols;
            maxRows = rows;
            bestMode = i;
        }
    }

    status = cout->SetMode(cout, bestMode);
    if(EFI_ERROR(status)){
        // Go back to default mode
        cout->SetMode(cout, originalTextMode);
        status = EFI_UNSUPPORTED;
    }

    return status;
}

EFI_STATUS SetTextMode(UINTN rows){
    EFI_INPUT_KEY key;
    key.ScanCode = 0;
    key.UnicodeChar = u'\0';
    UINTN lastMode = cout->Mode->Mode;
    EFI_STATUS status = EFI_SUCCESS;

    while(TRUE){
        cout->ClearScreen(cout);
        PrintConsoleAttributes();
        printf(u"\r\nSelect Text Mode 0-%d: %d", cout->Mode->MaxMode - 1, lastMode);

        // Move the cursor back one space
        //cout->SetCursorPosition(cout, cout->Mode->CursorColumn - 1, cout->Mode->CursorRow);

        cout->SetCursorPosition(cout, 0, rows - 1);
        printf(u"ESC = Exit");

        // Main loop
        key = GetKey();

        UINTN thisMode = key.UnicodeChar - u'0';
        if(thisMode >= 0 && thisMode < (UINTN)cout->Mode->MaxMode){
            EFI_STATUS status = cout->SetMode(cout, thisMode);
            if(EFI_ERROR(status)){
                cout->SetMode(cout, lastMode);                                      // Revert to the last mode
                if(status == EFI_DEVICE_ERROR){
                    printf(u"\r\nUnsupported mode 0x%x\r\n", status);
                }else if(status == EFI_UNSUPPORTED){
                    printf(u"\r\nInvalid mode 0x%x\r\n", status);
                }
            }else{
                lastMode = thisMode;
            }
        }

        if(key.ScanCode == KEY_ESC){
            // The user pressed the escape key, shut down the system
            break;
        }
    }
    return status;
}

EFI_STATUS SetGraphicsMode(void){
    UINTN cols = 0;
    UINTN rows = 0;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
    UINTN modeSize = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    EFI_STATUS status = 0;

    INT32 numOptions = 0;
    INT32 currentOption = 0;

    INT32 printStart = 0;

    cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);
    if(!isGraphicsMode){
        cout->ClearScreen(cout);
        printf(u"Graphics mode is not supported on this system!\r\n");
        cout->SetCursorPosition(cout, 0, rows - 1);
        printf(u"ESC = Exit");

        while(TRUE) {
            EFI_INPUT_KEY key = GetKey();
            if(key.ScanCode == KEY_ESC){
                return EFI_UNSUPPORTED;
            }
        }
    }
    
    while(TRUE){
        cout->ClearScreen(cout);
        cout->OutputString(cout, u"Graphics mode information:\r\n");

        //status = gop->QueryMode(gop, gop->Mode->Mode, &modeSize, &info);
        if(EFI_ERROR(status)){
            cout->OutputString(cout, u"Failed to query graphics mode information!\r\n");
            cout->SetCursorPosition(cout, 0, rows - 1);
            cout->OutputString(cout, u"ESC = Exit");

            gk:
            EFI_INPUT_KEY key = GetKey();
            if(key.ScanCode == KEY_ESC){
                return EFI_UNSUPPORTED;
            }
            goto gk;
        }

        printf(u"     Max Mode: %d\r\n", gop->Mode->MaxMode);
        printf(u"     Current Mode: %d\r\n", gop->Mode->Mode);
        printf(u"     Screen Dimensions: %ux%u\r\n", gop->Mode->Info->HorizontalResolution, gop->Mode->Info->VerticalResolution);
        printf(u"     Framebuffer Address: 0x%x\r\n", gop->Mode->FrameBufferBase);
        printf(u"     Framebuffer Size: %u Bytes\r\n", gop->Mode->FrameBufferSize);
        printf(u"     Pixel Format: %d\r\n", gop->Mode->Info->PixelFormat);
        printf(u"     Pixels Per Scan Line: %u\r\n", gop->Mode->Info->PixelsPerScanLine);

        numOptions = gop->Mode->MaxMode;
        cout->OutputString(cout, u"Available GOP Modes:\r\n");

        // Get the number of columns and rows for the menu
        UINTN cols = 0;
        UINTN rows = 0;
        cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);

        // Get the screen dimensions for text mode
        UINTN topRow = cout->Mode->CursorRow + 2;
        UINTN bottomRow = rows - 2;             // Leave room for controls at the bottom

        for(INT32 i = printStart; i < numOptions; i++){
            gop->QueryMode(gop, i, &modeSize, &info);
            if((i + topRow) - printStart > bottomRow){
                break;
            }
            if(i == currentOption){
                cout->SetAttribute(cout, currentHilight);
                printf(u"     Graphics Mode %d: (%ux%u)\r\n", i, info->HorizontalResolution, info->VerticalResolution);
                cout->SetAttribute(cout, currentTheme);
            }else{
                printf(u"     Graphics Mode %d: (%ux%u)\r\n", i, info->HorizontalResolution, info->VerticalResolution);
            }
        }

        cout->SetCursorPosition(cout, 0, rows - 1);
        printf(u"ESC = Go Back   Up/Down = Navigate   Enter = Select");

        EFI_INPUT_KEY key = GetKey();
        
        switch(key.ScanCode){
            case KEY_ESC:
                // Escape key pressed, return to main menu
                return EFI_SUCCESS;
            case KEY_UP:
                // Move the cursor up
                if(currentOption > 0){
                    currentOption--;
                }else{
                    break;
                }

                if(currentOption < printStart){
                    printStart--;
                }
                break;
            case KEY_DOWN:
                // Move the cursor down
                if(currentOption < numOptions - 1){
                    currentOption++;
                }else{
                    break;
                }

                // Scroll down to show the next menu option
                if((UINTN)currentOption > bottomRow - topRow){
                    printStart++;
                }
                break;
            default:
                if(key.UnicodeChar == u'\r'){
                    // The user pressed the enter key (selected a choice)
                    gop->SetMode(gop, currentOption);
                    cout->ClearScreen(cout);
                }
                break;
        }
    }
    return status;
}

void Shutdown(void){
    rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    __builtin_unreachable();
}

static CHAR16* themes[] = {
    u"1. Colorful (Default)",
    u"2. Dark"
};

void ChangeTheme(UINTN rows){
    while(1){
        cout->SetAttribute(cout, currentTheme);
        cout->ClearScreen(cout);
        printf(u"Select a color theme:\r\n");
        for(UINTN i = 0; i < ARRAY_SIZE(themes); i++){
            printf(themes[i]);
            printf(u"\r\n");
        }

        cout->SetCursorPosition(cout, 0, rows - 1);
        printf(u"ESC = Exit   1-2 = Select Theme");

        EFI_INPUT_KEY key = GetKey();
        if(key.UnicodeChar == u'1'){
            currentTheme = COLOR_THEME_COLORFUL;
            currentHilight = COLORFUL_HILIGHT_COLOR;
        }
        
        if(key.UnicodeChar == u'2'){
            currentTheme = COLOR_THEME_DARK;
            currentHilight = DARK_HILIGHT_COLOR;
        }
        
        if(key.ScanCode == KEY_ESC){
            break;
        }
    }
}

static CHAR16* menuOptions[] = {
    u"Boot Dedication OS",
    u"Change Text Mode",
    u"Set Graphics Mode",
    u"Change Color Theme",
    u"Reboot",
    u"Shutdown",
    u"Exit"
};

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable){
    // Set the global variables
    st = SystemTable;
    cout = SystemTable->ConOut;
    cin = SystemTable->ConIn;
    image = ImageHandle;
    bs = SystemTable->BootServices;
    rs = SystemTable->RuntimeServices;
    cerr = SystemTable->StdErr;

    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    if(EFI_ERROR(bs->LocateProtocol(&gopGuid, NULL, (VOID**)&gop))){
        isGraphicsMode = FALSE;
    }

    cout->Reset(SystemTable->ConOut, FALSE);
    cout->SetAttribute(SystemTable->ConOut, currentTheme);

    SetHighestTextMode();

    UINTN cols = 0;
    UINTN rows = 0;
    cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);
    cout->SetCursorPosition(cout, 0, 0);

    EFI_INPUT_KEY key;
    key.ScanCode = 0;
    key.UnicodeChar = u'\0';

    UINT8 selectedOption = 0;

    BOOLEAN exit = FALSE;
    while(!exit){
        cout->ClearScreen(cout);

        cols = 0;
        rows = 0;
        cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);
        cout->SetCursorPosition(cout, 0, 0);

        // Print the rest of the menu options
        for(UINTN i = 0; i < ARRAY_SIZE(menuOptions); i++){
            if(i != selectedOption){
                printf(u"%s\r\n", menuOptions[i]);
            }else{
                cout->SetAttribute(cout, currentHilight);
                printf(menuOptions[selectedOption]);
                printf(u"\r\n");
                cout->SetAttribute(cout, currentTheme);
            }
        }

        cout->SetCursorPosition(cout, 0, rows - 1);
        printf(u"ESC = Shutdown   Up/Down = Navigate   Enter = Select");

        key = GetKey();

        switch(key.ScanCode){
            case KEY_ESC:
                // Escape key pressed, return to UEFI interface
                Shutdown();
                break;
            case KEY_UP:
                // Move the cursor up
                if(selectedOption > 0){
                    selectedOption--;
                }
                break;
            case KEY_DOWN:
                // Move the cursor down
                if(selectedOption < ARRAY_SIZE(menuOptions) - 1){
                    selectedOption++;
                }
                break;
            default:
                if(key.UnicodeChar == u'\r'){
                    // The user pressed the enter key (selected a choice)
                    cout->ClearScreen(cout);
                    switch(selectedOption){
                        case 0:
                            // Boot Dedication OS
                            cout->OutputString(cout, u"Booting Dedication OS...\r\n");

                            cout->OutputString(cout, u"Sorry! This OS doesn't exist yet!\r\n");

                            cout->SetCursorPosition(cout, 0, rows - 1);
                            cout->OutputString(cout, u"ESC = Exit");

                            key = GetKey();

                            break;
                        case 1:
                            // Change Text Mode
                            SetTextMode(rows);
                            break;
                        case 2:
                            // Set Graphics Mode
                            SetGraphicsMode();

                            break;
                        case 3:
                            // Change Color Theme
                            ChangeTheme(rows);

                            break;
                        case 4:
                            // Reboot
                            cout->OutputString(cout, u"Rebooting...\r\n");
                            rs->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
                            __builtin_unreachable();

                            break;
                        case 5:
                            // Shutdown
                            cout->OutputString(cout, u"Shutting down...\r\n");
                            Shutdown();

                            break;
                        case 6:
                            // Exit
                            exit = TRUE;

                            break;
                        default:
                            break;
                    }
                }
                break;
        }
    }

    return EFI_SUCCESS;
}

// Print a number to the screen
BOOLEAN PrintNum(UINTN num, UINT8 base, BOOLEAN s){
    static CHAR16* digits = u"0123456789ABCDEF";
    // Include 64-bit binary support
    CHAR16 buffer[65];
    UINTN i = 0;

    if(base > 16){
        printf(u"Invalid base: %d\r\n", (INT32)base);
        return FALSE;
    }

    // Check if the number is negative
    if(s && (INTN)num < 0){
        cout->OutputString(cout, u"-");
        num = -(INTN)num;
    }
    
    // Convert the number to a string
    do {
        buffer[i++] = digits[num % base];
        num /= base;
    } while(num > 0);

    // Null-terminate the string
    buffer[i] = u'\0';

    // Reverse the buffer
    for(UINTN j = 0; j < i / 2; j++){
        CHAR16 temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    cout->OutputString(cout, buffer);

    return TRUE;
}

// Print a formatted string to stdout
void printf(CHAR16* fmt, ...){
    CHAR16 c[2];
    c[0] = u'\0', c[1] = u'\0';
    va_list args;
    va_start(args, fmt);
    for(UINTN i = 0; fmt[i] != '\0'; i++){
        if(fmt[i] == u'%'){
            i++;
            switch(fmt[i]){
                case u's': {
                    // Print another string
                    CHAR16* str = va_arg(args, CHAR16*);
                    cout->OutputString(cout, str);
                    break;
                }
                case u'd': {
                    // Print a signed 32-bit integer
                    INT32 num = va_arg(args, INT32);
                    PrintNum((UINTN)num, 10, TRUE);
                    break;
                }
                case u'c': {
                    // Print a character
                    CHAR16 ch = va_arg(args, CHAR16);
                    if(ch == u'\0'){
                        // Edge case: null character
                        cout->OutputString(cout, u"None");
                    }
                    c[0] = ch;
                    cout->OutputString(cout, c);
                    break;
                }
                case u'x': {
                    // Print a hexadecimal number
                    UINT32 num = va_arg(args, UINT32);
                    PrintNum((UINTN)num, 16, TRUE);
                    break;
                }
                case u'%': {
                    // Print a percent sign
                    c[0] = u'%';
                    cout->OutputString(cout, c);
                    break;
                }
                case u'u': {
                    // Print an unsigned 32-bit integer
                    UINT32 num = va_arg(args, UINT32);
                    PrintNum((UINTN)num, 10, TRUE);
                    break;
                }
                case u'o': {
                    // Print an octal number
                    UINT32 num = va_arg(args, UINT32);
                    PrintNum(num, 8, TRUE);
                    break;
                }
                case u'b': {
                    // Print a binary number (not standard but helpful)
                    UINT32 num = va_arg(args, UINT32);
                    PrintNum(num, 2, TRUE);
                    break;
                }
                case u'l': {
                    i++;
                    // Potential long integer
                    if(fmt[i] == u'd'){
                        // Print a signed 64-bit integer
                        INT64 num = va_arg(args, INT64);
                        PrintNum((UINTN)num, 10, TRUE);
                    } else if(fmt[i] == u'x'){
                        // Print a hexadecimal number (specifically 64-bit, but a UINTN on AMD64 is 64-bit)
                        UINT64 num = va_arg(args, UINT64);
                        PrintNum((UINTN)num, 16, TRUE);                // While many don't need the cast, it's best practice to include it.
                    } else if(fmt[i] == u'u'){
                        // Print an unsigned 64-bit integer
                        UINT64 num = va_arg(args, UINT64);
                        PrintNum((UINTN)num, 10, TRUE);
                    } else if(fmt[i] == u'o'){
                        // Print an octal number
                        UINT64 num = va_arg(args, UINT64);
                        PrintNum((UINTN)num, 8, TRUE);
                    } else if(fmt[i] == u'b'){
                        // Print a binary number
                        UINT64 num = va_arg(args, UINT64);
                        PrintNum((UINTN)num, 2, TRUE);
                    } else {
                        // Invalid or unsupported format specifier
                        cout->OutputString(cout, u"Invalid format specifier\r\n");
                    }
                    break;
                }
                default:
                    // Invalid or unsupported format specifier
                    cout->OutputString(cout, u"Invalid format specifier\r\n");
                    break;
            }
        } else {
            c[0] = fmt[i];
            cout->OutputString(cout, c);
        }
    }

    va_end(args);
}
