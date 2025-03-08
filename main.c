#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdint.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "tinytiffwriter.h"

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 200
#define BUTTON_WIDTH 150
#define BUTTON_HEIGHT 30

// Global variables
HWND hwndButton, hwndStatus;
HINSTANCE hInstance;

// FITS header card structure (80 bytes each)
#pragma pack(push, 1)
typedef struct {
    char card[80];  // Fixed 80-byte card format: keyword(8) + value(72)
} FITSHeaderCard;

// FITS header block (2880 bytes = 36 cards of 80 bytes each)
typedef struct {
    FITSHeaderCard cards[36];  // 36 cards per block
} FITHeader;
#pragma pack(pop)

// Helper function to parse header card values
int parse_card_value(const char* card, const char* keyword, int* value) {
    char temp[81] = {0};  // +1 for null terminator
    strncpy_s(temp, sizeof(temp), card, 80);
    
    // Check if this is the keyword we're looking for
    if (strncmp(temp, keyword, strlen(keyword)) == 0) {
        // FITS format: keyword(8) + "= " + value + comment
        // Find the equals sign and parse after it
        char* equals = strchr(temp, '=');
        if (equals) {
            return sscanf_s(equals + 1, "%d", value);
        }
    }
    return 0;
}

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void HandleConversion(HWND hwnd);
int ConvertFITtoTIF(const wchar_t* inputPath);
void ShowError(HWND hwnd, const wchar_t* format, ...);
void UpdateStatus(const wchar_t* message, BOOL isError);

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    
    hInstance = hInst;

    // Register the window class
    const wchar_t CLASS_NAME[] = L"FITConverter";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    // Create the window
    HWND hwnd = CreateWindowExW(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        L"FIT to TIF Converter",        // Window text
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME), // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position
        WINDOW_WIDTH, WINDOW_HEIGHT,     // Size
        NULL,                           // Parent window
        NULL,                           // Menu
        hInstance,                      // Instance handle
        NULL                            // Additional data
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Create "Select FIT File" button
            hwndButton = CreateWindowW(
                L"BUTTON",
                L"Select FIT File",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                (WINDOW_WIDTH - BUTTON_WIDTH) / 2,
                50,
                BUTTON_WIDTH,
                BUTTON_HEIGHT,
                hwnd,
                (HMENU)1,
                hInstance,
                NULL
            );

            // Create status text
            hwndStatus = CreateWindowW(
                L"STATIC",
                L"Select a .FIT file to convert to .TIF format",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                10,
                100,
                WINDOW_WIDTH - 20,
                50,
                hwnd,
                NULL,
                hInstance,
                NULL
            );
            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) {  // Button clicked
                HandleConversion(hwnd);
            }
            return 0;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }

        case WM_GETMINMAXINFO: {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            lpMMI->ptMinTrackSize.x = WINDOW_WIDTH;
            lpMMI->ptMinTrackSize.y = WINDOW_HEIGHT;
            lpMMI->ptMaxTrackSize.x = WINDOW_WIDTH;
            lpMMI->ptMaxTrackSize.y = WINDOW_HEIGHT;
            return 0;
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void HandleConversion(HWND hwnd) {
    wchar_t filename[MAX_PATH];
    
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"FIT Files\0*.FIT\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Select FIT File";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"FIT";

    filename[0] = '\0';

    if (GetOpenFileNameW(&ofn)) {
        if (ConvertFITtoTIF(filename)) {
            UpdateStatus(L"Conversion successful!", FALSE);
        }
    }
}

uint8_t write_simple_tiff(const char *filepath, void *image_data, int bitpix, size_t width, size_t height, uint8_t channels) {
    TinyTIFFWriterFile* tif=TinyTIFFWriter_open(filepath, bitpix, TinyTIFFWriter_UInt, channels, width, height, TinyTIFFWriter_AutodetectSampleInterpetation);
    if (tif) {
        // const uint8_t* data=readImage();
        if (TINYTIFF_TRUE != TinyTIFFWriter_writeImage(tif, image_data)) {
            ShowError(NULL, L"TinyTIFFWriter_writeImage failed");
            TinyTIFFWriter_close(tif);
            return 0;
        }
        TinyTIFFWriter_close(tif);
    } else {
        ShowError(NULL, L"Could not create TIF writer");
        return 0;
    }

    return 1;
}

int ConvertFITtoTIF(const wchar_t* inputPath) {
    FILE* inFile = NULL;
    FILE* outFile = NULL;
    int success = 0;
    int width = 0, height = 0, channels = 0, bitpix = 0, bzero = 0;

    // Open input file
    _wfopen_s(&inFile, inputPath, L"rb");
    if (!inFile) {
        ShowError(NULL, L"Could not open input file");
        goto cleanup;
    }

    // Read first header block
    // FITHeader header;
    // if (fread(&header, sizeof(header), 1, inFile) != 1) {
    //     ShowError(NULL, L"Could not read FITS header");
    //     goto cleanup;
    // }

    // Parse header cards
    // for (int i = 0; i < 36; i++) {
    //     const char* card = header.cards[i].card;
        
    //     // Debug output for each card
    //     char debug_card[81] = {0};
    //     strncpy_s(debug_card, sizeof(debug_card), card, 80);
    //     printf("Card %d: '%s'\n", i, debug_card);

    //     // Parse specific values
    //     parse_card_value(card, "BITPIX", &bitpix);
    //     parse_card_value(card, "NAXIS1", &width);
    //     parse_card_value(card, "NAXIS2", &height);
    //     parse_card_value(card, "NAXIS3", &channels);
    //     parse_card_value(card, "BZERO", &bzero);

    //     // Check for END card
    //     if (strncmp(card, "END", 3) == 0) {
    //         printf("END card found\n");
    //         break;
    //     }
    // }
    
    // keep reading cards until END card is found
    int i = 0;
    while (1) {
        FITSHeaderCard card;
        if (fread(&card, sizeof(card), 1, inFile) != 1) {
            ShowError(NULL, L"Could not read FITS header");
            goto cleanup;
        }
        if (strncmp(card.card, "END", 3) == 0) {
            printf("END card found\n"); 
            break;
        }

        // Debug output for each card
        char debug_card[81] = {0};
        strncpy_s(debug_card, sizeof(debug_card), card.card, 80);
        printf("Card %d: '%s'\n", i, debug_card);

        // Parse specific values
        parse_card_value(card.card, "BITPIX", &bitpix);
        parse_card_value(card.card, "NAXIS1", &width);
        parse_card_value(card.card, "NAXIS2", &height);
        parse_card_value(card.card, "NAXIS3", &channels);
        parse_card_value(card.card, "BZERO", &bzero);
    }

    size_t file_position = ftell(inFile);
    size_t offset = 2880 - (file_position % 2880);

    // Debug output
    printf("Parsed FITS Header Values:\n");
    printf("BITPIX: %d\n", bitpix);
    printf("Width (NAXIS1): %d\n", width);
    printf("Height (NAXIS2): %d\n", height);
    printf("Channels (NAXIS3): %d\n", channels);
    printf("Current file position: %ld\n", ftell(inFile));
    printf("Padding: %lld\n", offset);

    // Seek forward to data with offset
    fseek(inFile, offset, SEEK_CUR);

    // Validate dimensions
    if (width <= 0 || width > 65535) {
        ShowError(NULL, L"Invalid image width: %d", width);
        goto cleanup;
    }
    if (height <= 0 || height > 65535) {
        ShowError(NULL, L"Invalid image height: %d", height);
        goto cleanup;
    }
    if (channels != 1 && channels != 3) {
        ShowError(NULL, L"Invalid number of channels: %d (must be 1 or 3)", channels);
        goto cleanup;
    }

    // Validate BITPIX
    if (bitpix != 8 && bitpix != 16 && bitpix != -32 && bitpix != -64) {
        ShowError(NULL, L"Unsupported BITPIX value: %d", bitpix);
        goto cleanup;
    }

    size_t data_size = width * height * channels;
    void *image_data;
    size_t pixel_size;
    if (bitpix == 8) {
        image_data = malloc(data_size * sizeof(uint8_t));
        pixel_size = sizeof(uint8_t);
    } else if (bitpix == 16) {
        image_data = malloc(data_size * sizeof(uint16_t));
        pixel_size = sizeof(uint16_t);
    } else if (bitpix == -32) {
        image_data = malloc(data_size * sizeof(float));
        pixel_size = sizeof(float);
    } else {
        ShowError(NULL, L"Unsupported BITPIX value: %d", bitpix);
        goto cleanup;
    }

    // size_t r = fread(image_data, pixel_size, data_size, inFile);
    // if (r != data_size) {
    //     ShowError(NULL, L"Could not read image data, got %d expected %d", r, data_size);
    //     free(image_data);
    //     goto cleanup;
    // }

    // read image data, one channel at a time
    void* tmpPixel = malloc(pixel_size);
    for (int i = 0; i < channels; i++) {
        for (int j = 0; j < width * height; j++) {
            // fread(image_data + j * channels + i, pixel_size, 1, inFile);
            fread(tmpPixel, pixel_size, 1, inFile);
            if (bitpix == 8) {
                ((uint8_t*)image_data)[j * channels + i] = ((uint8_t*)tmpPixel)[0];
            } else if (bitpix == 16) {
                ((uint16_t*)image_data)[j * channels + i] = ((uint16_t*)tmpPixel)[0];
            } else if (bitpix == -32) {
                ((float*)image_data)[j * channels + i] = ((float*)tmpPixel)[0];
            }
        }
    }
    free(tmpPixel);

    uint8_t *data_8bit = (uint8_t *)malloc(width * height * channels);
    if (!data_8bit) {
        ShowError(NULL, L"Could not allocate memory for image data");
        goto cleanup;
    }

    if (bitpix == 8) {
        memcpy(data_8bit, image_data, width * height * channels);
    } else if (bitpix == 16) {
        for (int i = 0; i < width * height * channels; i++) {
            // float d = ((((uint16_t*)image_data)[i] - bzero) >> 8) / 255.0f;
            uint8_t d = (((uint16_t*)image_data)[i]) >> 8;
            // data_8bit[i] = (uint8_t)(d * 255);
            data_8bit[i] = d;
        }
    } else if (bitpix == -32) {
        ShowError(NULL, L"Unsupported BITPIX value for conversion: %d", bitpix);
        goto cleanup;
    } else if (bitpix == -64) { 
        ShowError(NULL, L"Unsupported BITPIX value for conversion: %d", bitpix);
        goto cleanup;
    }

    // Create output filename (replace .FIT with .TIF)
    wchar_t filepath_w[MAX_PATH];
    wcscpy_s(filepath_w, MAX_PATH, inputPath);
    wchar_t* ext = wcsrchr(filepath_w, L'.');
    if (ext) {
        wcscpy_s(ext, 5, L".TIF");
        // wcscpy_s(ext, 5, L".JPG");
    }

    // filepath to simple char array
    char filepath[MAX_PATH];
    wcstombs(filepath, filepath_w, MAX_PATH);

    // write png version
    if (!stbi_write_jpg(filepath, width, height, channels, data_8bit, 100)) {
        ShowError(NULL, L"Could not write image data");
        goto cleanup;
    }

    // write tiff version
    if (!write_simple_tiff(filepath, image_data, bitpix, width, height, channels)) {
        ShowError(NULL, L"Could not write tiff image data");
        goto cleanup;
    }

    success = 1;

cleanup:
    if (inFile) fclose(inFile);
    if (outFile) fclose(outFile);
    free(image_data);
    return success;
}

void ShowError(HWND hwnd, const wchar_t* format, ...) {
    wchar_t message[1024];  // Buffer for formatted message
    va_list args;
    
    va_start(args, format);
    vswprintf_s(message, _countof(message), format, args);
    va_end(args);
    
    MessageBoxW(hwnd, message, L"Error", MB_OK | MB_ICONERROR);
    UpdateStatus(message, TRUE);
    printf("Error: %ls\n", message);
}

void UpdateStatus(const wchar_t* message, BOOL isError) {
    UNREFERENCED_PARAMETER(isError);
    SetWindowTextW(hwndStatus, message);
    printf("Status: %ls\n", message);
}
