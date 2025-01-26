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
#include "efi.h"
#include <stdarg.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

void printf(CHAR16* fmt, ...);

EFI_SYSTEM_TABLE* st;
EFI_HANDLE image;

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* cout;
EFI_SIMPLE_TEXT_INPUT_PROTOCOL* cin;

EFI_BOOT_SERVICES* bs;
EFI_RUNTIME_SERVICES* rs;

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

EFI_STATUS SetTextMode(UINTN rows){
    EFI_INPUT_KEY key;
    key.ScanCode = 0;
    key.UnicodeChar = u'\0';
    UINTN lastMode = cout->Mode->Mode;
    EFI_STATUS status = EFI_SUCCESS;

    while(true){
        cout->ClearScreen(st->ConOut);
        PrintConsoleAttributes();
        printf(u"\r\nSelect Text Mode 0-%d: %d", cout->Mode->MaxMode - 1, lastMode);

        // Move the cursor back one space
        cout->SetCursorPosition(cout, cout->Mode->CursorColumn - 1, cout->Mode->CursorRow);

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

void Shutdown(void){
    rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    __builtin_unreachable();
}

#define COLOR_THEME_COLORFUL EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLUE)                           // Default color theme
#define COLOR_THEME_DARK EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)                               // Dark color theme (classic)

#define COLORFUL_HILIGHT_COLOR EFI_TEXT_ATTR(EFI_BLUE, EFI_CYAN)                           // Default highlighted text color
#define DARK_HILIGHT_COLOR EFI_TEXT_ATTR(EFI_BACKGROUND_LIGHTGRAY, EFI_BLACK)                             // Dark highlighted text color

UINT16 currentTheme = COLOR_THEME_COLORFUL;
UINT16 currentHilight = COLORFUL_HILIGHT_COLOR;

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
    u"Shutdown",
    u"Reboot",
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

    cout->Reset(SystemTable->ConOut, false);
    cout->SetAttribute(SystemTable->ConOut, currentTheme);

    EFI_INPUT_KEY key;
    key.ScanCode = 0;
    key.UnicodeChar = u'\0';

    UINT8 selectedOption = 0;

    bool exit = false;
    while(!exit){
        cout->ClearScreen(cout);

        // Print keybinds
        UINTN cols = 0;
        UINTN rows = 0;
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
                            printf(u"Booting Dedication OS...\r\n");

                            printf(u"Sorry! This OS doesn't exist yet!\r\n");

                            cout->SetCursorPosition(cout, 0, rows - 1);
                            printf(u"ESC = Exit");

                            key = GetKey();

                            break;
                        case 1:
                            // Change Text Mode
                            SetTextMode(rows);
                            break;
                        case 2:
                            // Set Graphics Mode
                            cout->OutputString(cout, u"Graphics mode not supported yet\r\n");

                            cout->SetCursorPosition(cout, 0, rows - 1);
                            printf(u"ESC = Exit");

                            key = GetKey();

                            break;
                        case 3:
                            // Change Color Theme
                            ChangeTheme(rows);

                            break;
                        case 4:
                            // Shutdown
                            cout->OutputString(cout, u"Shutting down...\r\n");
                            Shutdown();

                            break;
                        case 5:
                            // Reboot
                            cout->OutputString(cout, u"Rebooting...\r\n");
                            rs->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);

                            break;
                        case 6:
                            // Exit
                            exit = true;

                            break;
                    }
                }
                break;
        }

        //SetTextMode();
    }

    return EFI_SUCCESS;
}


// Print a formatted string to the screen
// Yes, I know it's bad.
// This function is huge so I'll declare a prototype and leave it at the bottom of the file.
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
                    CHAR16 numStr[21];
                    UINTN j = 0;
                    if(num >= 0){
                        do{
                            numStr[j] = u'0' + num % 10;
                            num /= 10;
                            j++;
                        } while(num > 0);
                    }else{
                        // The number is negative
                        num = -num;
                        do{
                            numStr[j] = u'0' + num % 10;
                            num /= 10;
                            j++;
                        } while(num > 0);
                        numStr[j] = u'-';
                        j++;
                    }
                    numStr[j] = u'\0';
                    for(UINTN k = 0; k < j / 2; k++){
                        CHAR16 temp = numStr[k];
                        numStr[k] = numStr[j - k - 1];
                        numStr[j - k - 1] = temp;
                    }
                    cout->OutputString(cout, numStr);
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
                    UINTN num = va_arg(args, UINTN);
                    CHAR16 numStr[21];
                    UINTN j = 0;
                    do{
                        UINTN digit = num % 16;
                        if(digit < 10){
                            numStr[j] = u'0' + digit;
                        } else {
                            numStr[j] = u'A' + digit - 10;
                        }
                        num /= 16;
                        j++;
                    } while(num > 0);
                    numStr[j] = u'\0';
                    for(UINTN k = 0; k < j / 2; k++){
                        CHAR16 temp = numStr[k];
                        numStr[k] = numStr[j - k - 1];
                        numStr[j - k - 1] = temp;
                    }
                    cout->OutputString(cout, numStr);
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
                    UINTN num = va_arg(args, UINTN);
                    CHAR16 numStr[21];
                    UINTN j = 0;
                    do{
                        numStr[j] = u'0' + num % 10;
                        num /= 10;
                        j++;
                    }while(num > 0);
                    numStr[j] = u'\0';
                    for(UINTN k = 0; k < j / 2; k++){
                        CHAR16 temp = numStr[k];
                        numStr[k] = numStr[j - k - 1];
                        numStr[j - k - 1] = temp;
                    }
                    cout->OutputString(cout, numStr);
                    break;
                }
                case u'l': {
                    // Potential long integer
                    if(fmt[i + 1] == u'd'){
                        // Print a signed 64-bit integer
                        i++;
                        INT64 num = va_arg(args, INT64);
                        CHAR16 numStr[21];
                        UINTN j = 0;
                        if(num >= 0){
                            do{
                                numStr[j] = u'0' + num % 10;
                                num /= 10;
                                j++;
                            } while(num > 0);
                        }else{
                            // The number is negative
                            num = -num;
                            do{
                                numStr[j] = u'0' + num % 10;
                                num /= 10;
                                j++;
                            } while(num > 0);
                            numStr[j] = u'-';
                            j++;
                        }
                        numStr[j] = u'\0';
                        for(UINTN k = 0; k < j / 2; k++){
                            CHAR16 temp = numStr[k];
                            numStr[k] = numStr[j - k - 1];
                            numStr[j - k - 1] = temp;
                        }
                        cout->OutputString(cout, numStr);
                    } else if(fmt[i + 1] == u'x'){
                        // Print a hexadecimal number (specifically 64-bit, but a UINTN on AMD64 is 64-bit)
                        i++;
                        UINT64 num = va_arg(args, UINT64);
                        CHAR16 numStr[21];
                        UINTN j = 0;
                        do{
                            UINTN digit = num % 16;
                            if(digit < 10){
                                numStr[j] = u'0' + digit;
                            } else {
                                numStr[j] = u'A' + digit - 10;
                            }
                            num /= 16;
                            j++;
                        } while(num > 0);
                        numStr[j] = u'\0';
                        for(UINTN k = 0; k < j / 2; k++){
                            CHAR16 temp = numStr[k];
                            numStr[k] = numStr[j - k - 1];
                            numStr[j - k - 1] = temp;
                        }
                        cout->OutputString(cout, numStr);
                    } else if(fmt[i + 1] == u'u'){
                        // Print an unsigned 64-bit integer
                        i++;
                        UINT64 num = va_arg(args, UINT64);
                        CHAR16 numStr[21];
                        UINTN j = 0;
                        do{
                            numStr[j] = u'0' + num % 10;
                            num /= 10;
                            j++;
                        } while(num > 0);
                        numStr[j] = u'\0';
                        for(UINTN k = 0; k < j / 2; k++){
                            CHAR16 temp = numStr[k];
                            numStr[k] = numStr[j - k - 1];
                            numStr[j - k - 1] = temp;
                        }
                        cout->OutputString(cout, numStr);
                    } else {
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
