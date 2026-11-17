#ifndef _GDI_FIX
#define _GDI_FIX

/* I'm not sure where Visual C++ 2 (MSC9) falls onto this.
	I'm not sure it even matters, if you have VC 2
	You'll have access to VC 4	*/

/* Changed 2026-10 for NT 3.5: its SDK, without the API of Windows 95 and NT 3.51,
	takes the old paths too (OLD_NT); NT 3.5's LoadImage and CheckMenuRadioItem
	are stubs that fail	*/
#if(_MSC_VER < 1000) || !defined(BST_CHECKED)
#define OLD_NT			1
#endif

#ifdef OLD_NT
#define WS_EX_CLIENTEDGE 	0
#define WS_EX_TOOLWINDOW 	0
#define MB_ICONWARNING  	MB_ICONASTERISK
#define MB_ICONERROR    	MB_ICONSTOP

#define	DEFAULT_GUI_FONT	SYSTEM_FIXED_FONT

#define OFN_EXPLORER		0

#define	LR_LOADFROMFILE		0
#define	LR_CREATEDIBSECTION	0
#define	LR_DEFAULTCOLOR		0
#define	IMAGE_BITMAP		0

#define	BST_UNCHECKED		0
#define	BST_CHECKED		1
#endif


HANDLE LoadImageFromFile (LPCSTR filename, UINT fuLoad);

/* Tileset conversion and validation functions */
HBITMAP convertTo8Bit(HBITMAP hSrcBitmap, HDC hdc, HPALETTE hSystemPalette);
int validateTilesetFormat(HBITMAP hBitmap);

/* CheckMenuRadioItem compatibility */
#ifdef OLD_NT
    /* Simple fallback for older versions - uncheck all then check selected */
    #define CHECK_MENU_RADIO_ITEM(hmenu, first, last, check, flags) \
        { int __i; \
          for (__i = (first); __i <= (last); __i++) { \
              CheckMenuItem((hmenu), __i, MF_UNCHECKED | (flags)); \
          } \
          CheckMenuItem((hmenu), (check), MF_CHECKED | (flags)); \
        }
#else
    /* Use CheckMenuRadioItem if it's supported */
    #define CHECK_MENU_RADIO_ITEM(hmenu, first, last, check, flags) \
        CheckMenuRadioItem((hmenu), (first), (last), (check), (flags))
#endif

#if(_MSC_VER >= 1000)
/* use original Ex calls/structures where available (CreateDIBSection: NT 3.5 too) */
#define NEW32 1
#endif

#endif /* _GDI_FIX */
