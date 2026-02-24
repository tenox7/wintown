/* gdifix.c - trying to deal with old NT
 * 
 */

#include "tools.h"
#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "gdifix.h"

/* Maximum bitmap size to prevent buffer overflow attacks */
#define MAX_BITMAP_SIZE (16 * 1024 * 1024)  /* 16MB limit */

HANDLE LoadImageFromFile(LPCSTR filename, UINT fuLoad) {
#if(_MSC_VER > 900)
    /* Use newer LoadImage API if available */
    return LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, fuLoad);
#else
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bmiHeader;
    HBITMAP hBitmap = NULL;
    RGBQUAD *colorTable = NULL;
    BYTE *bitmapBits = NULL;
    FILE *file;
    BITMAPINFO *bmi;
    size_t bmiSize, fsize, colorTableSize;
    HDC hdc;
    /* Manual bitmap loading for older compilers */
    
    hdc = GetDC(NULL);
    if (hdc == NULL) {
        return NULL;
    }

    file = fopen(filename, "rb");
    if (file == NULL) {
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    if (fread(&bmfHeader, sizeof(BITMAPFILEHEADER), 1, file) != 1) {
        fclose(file);
        ReleaseDC(NULL, hdc);
        return NULL;
    }
    
    if (bmfHeader.bfType != 0x4D42) {
        fclose(file);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    if (fread(&bmiHeader, sizeof(BITMAPINFOHEADER), 1, file) != 1) {
        fclose(file);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    bmiSize = sizeof(BITMAPINFOHEADER) + (bmiHeader.biBitCount <= 8 ? (1 << bmiHeader.biBitCount) * sizeof(RGBQUAD) : 0);
    bmi = (BITMAPINFO *)malloc(bmiSize);
    if (bmi == NULL) {
        fclose(file);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    memcpy(&bmi->bmiHeader, &bmiHeader, sizeof(BITMAPINFOHEADER));

    if (bmiHeader.biBitCount <= 8) {
        colorTableSize = (1 << bmiHeader.biBitCount) * sizeof(RGBQUAD);
        colorTable = (RGBQUAD *)malloc(colorTableSize);
        if (colorTable == NULL) {
            free(bmi);
            fclose(file);
            ReleaseDC(NULL, hdc);
            return NULL;
        }
        if (fread(colorTable, colorTableSize, 1, file) != 1) {
            free(colorTable);
            free(bmi);
            fclose(file);
            ReleaseDC(NULL, hdc);
            return NULL;
        }
        memcpy(bmi->bmiColors, colorTable, colorTableSize);
    }

    if (!bmiHeader.biSizeImage) {
        fseek(file, 0, SEEK_END);
        fsize = ftell(file);
        
        /* Validate calculated size to prevent buffer overflow */
        if (fsize <= bmfHeader.bfOffBits || fsize > MAX_BITMAP_SIZE) {
            free(colorTable);
            free(bmi);
            fclose(file);
            ReleaseDC(NULL, hdc);
            return NULL;
        }
        
        bmiHeader.biSizeImage = fsize - bmfHeader.bfOffBits;
    }
    
    /* Additional validation for biSizeImage */
    if (bmiHeader.biSizeImage > MAX_BITMAP_SIZE || bmiHeader.biSizeImage == 0) {
        free(colorTable);
        free(bmi);
        fclose(file);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    bitmapBits = (BYTE *)malloc(bmiHeader.biSizeImage);
    if (bitmapBits == NULL) {
        free(colorTable);
        free(bmi);
        fclose(file);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    fseek(file, bmfHeader.bfOffBits, SEEK_SET);
    if (fread(bitmapBits, 1, bmiHeader.biSizeImage, file) != bmiHeader.biSizeImage) {
        free(bitmapBits);
        free(colorTable);
        free(bmi);
        fclose(file);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    hBitmap = CreateDIBitmap(hdc, &bmiHeader, CBM_INIT, bitmapBits, bmi, DIB_RGB_COLORS);

    free(bitmapBits);
    free(colorTable);
    free(bmi);
    fclose(file);
    ReleaseDC(NULL, hdc);

    addDebugLog("LoadBitmapFromFile loaded %s\n", filename);
    return hBitmap;
#endif
}

/* Convert any bitmap to 8-bit using the system palette */
HBITMAP convertTo8Bit(HBITMAP hSrcBitmap, HDC hdc, HPALETTE hSystemPalette) {
    BITMAP bm;
    
    /* Get source bitmap info */
    if (!GetObject(hSrcBitmap, sizeof(BITMAP), &bm)) {
        return NULL;
    }
    
    /* For non-8-bit images, create a compatible bitmap and use StretchBlt with palette */
    {
        HDC hdcSrc, hdcDest;
        HBITMAP hDestBitmap, hOldSrc, hOldDest;
        
        /* Create compatible DCs */
        hdcSrc = CreateCompatibleDC(hdc);
        hdcDest = CreateCompatibleDC(hdc);
        
        if (!hdcSrc || !hdcDest) {
            if (hdcSrc) DeleteDC(hdcSrc);
            if (hdcDest) DeleteDC(hdcDest);
            return NULL;
        }
        
        /* Apply system palette to both DCs */
        if (hSystemPalette) {
            SelectPalette(hdcSrc, hSystemPalette, FALSE);
            RealizePalette(hdcSrc);
            SelectPalette(hdcDest, hSystemPalette, FALSE);
            RealizePalette(hdcDest);
        }
        
        /* Create destination bitmap compatible with the display (8-bit) */
        hDestBitmap = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);
        
        if (hDestBitmap) {
            /* Select bitmaps */
            hOldSrc = SelectObject(hdcSrc, hSrcBitmap);
            hOldDest = SelectObject(hdcDest, hDestBitmap);
            
            /* Set stretch mode for better quality */
            SetStretchBltMode(hdcDest, COLORONCOLOR);
            
            /* Copy with palette conversion using StretchBlt */
            if (!StretchBlt(hdcDest, 0, 0, bm.bmWidth, bm.bmHeight, 
                           hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY)) {
                DeleteObject(hDestBitmap);
                hDestBitmap = NULL;
            }
            
            /* Restore selections */
            SelectObject(hdcSrc, hOldSrc);
            SelectObject(hdcDest, hOldDest);
        }
        
        DeleteDC(hdcSrc);
        DeleteDC(hdcDest);
        
        return hDestBitmap;
    }
}

/* Validate tileset meets game requirements */
int validateTilesetFormat(HBITMAP hBitmap) {
    BITMAP bm;
    
    if (!hBitmap) {
        return 0;
    }
    
    if (!GetObject(hBitmap, sizeof(BITMAP), &bm)) {
        return 0;
    }
    
    /* Check dimensions - tilesets must be 512x480 */
    if (bm.bmWidth != 512 || bm.bmHeight != 480) {
        return 0;
    }
    
    return 1;
}

