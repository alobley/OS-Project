#include "bootutil.h"
#include "efi.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable){
    SystemTable->ConOut->Reset(SystemTable->ConOut, false);

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Hello, World!\r\n");

    for(;;);

    return EFI_SUCCESS;
}
