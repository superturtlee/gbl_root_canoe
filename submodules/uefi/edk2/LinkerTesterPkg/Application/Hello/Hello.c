#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/LoadedImage.h>

// Placeholder for injected EFI - will be replaced by linker
__attribute__((section(".abl_placeholder"), used))
unsigned int injected_efi_len = 16;

__attribute__((section(".abl_placeholder"), used))
unsigned char injected_efi[] = "ABL_PLACEHOLDER!";

EFI_STATUS
LoadInjectedEfi(VOID)
{
  EFI_STATUS Status;
  EFI_HANDLE ImageHandle;

  Print(L"Loading injected EFI (size: %u bytes)...\n", injected_efi_len);

  // Load the injected EFI image
  Status = gBS->LoadImage(
    FALSE,
    gImageHandle,
    NULL,
    injected_efi,
    injected_efi_len,
    &ImageHandle
  );

  if (EFI_ERROR(Status)) {
    Print(L"Failed to load injected image: %r\n", Status);
    return Status;
  }

  Print(L"Starting injected EFI...\n");

  // Start the image
  Status = gBS->StartImage(ImageHandle, NULL, NULL);

  if (EFI_ERROR(Status)) {
    Print(L"Failed to start injected image: %r\n", Status);
  }

  return Status;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  Print(L"Hello EFI Application\n");
  Print(L"This is the loader with placeholder\n");
  Print(L"\n");

  LoadInjectedEfi();

  Print(L"\nHello EFI completed\n");

  return EFI_SUCCESS;
}
