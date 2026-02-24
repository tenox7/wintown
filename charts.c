/* charts.c - Chart system implementation for WiNTown
 * Comprehensive charting system with multiple data series and time ranges
 */

#include "charts.h"
#include "sim.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* External debug logging function */
extern void addDebugLog(const char* format, ...);

/* External window handles and variables */
extern HWND hwndMain;

/* Menu IDs - matching definitions from main.c */
#define IDM_VIEW_CHARTSWINDOW 4106

/* External RCI demand variables */
extern short RValve;
extern short CValve;
extern short IValve;

/* Global chart data instance */
ChartData* g_chartData = NULL;

/* Chart series configuration */
static const char* chartSeriesNames[CHART_SERIES_COUNT] = {
    "Population",
    "Residential", 
    "Commercial",
    "Industrial",
    "Funds",
    "Crime",
    "Pollution",
    "Land Value",
    "Infrastructure",
    "Power",
    "Growth Rate",
    "Approval",
    "R Demand",
    "C Demand",
    "I Demand"
};

static const COLORREF chartSeriesColors[CHART_SERIES_COUNT] = {
    CHART_COLOR_POPULATION,
    CHART_COLOR_RESIDENTIAL,
    CHART_COLOR_COMMERCIAL,
    CHART_COLOR_INDUSTRIAL,
    CHART_COLOR_FUNDS,
    CHART_COLOR_CRIME,
    CHART_COLOR_POLLUTION,
    CHART_COLOR_LAND_VALUE,
    CHART_COLOR_INFRASTRUCTURE,
    CHART_COLOR_POWER,
    CHART_COLOR_GROWTH_RATE,
    CHART_COLOR_APPROVAL,
    CHART_COLOR_R_DEMAND,
    CHART_COLOR_C_DEMAND,
    CHART_COLOR_I_DEMAND
};

/* Initialize chart system */
int InitChartSystem(void) {
    int i;
    
    if (g_chartData) {
        CleanupChartSystem();
    }
    
    g_chartData = (ChartData*)malloc(sizeof(ChartData));
    if (!g_chartData) {
        return 0;
    }
    
    memset(g_chartData, 0, sizeof(ChartData));
    
    /* Initialize chart series */
    for (i = 0; i < CHART_SERIES_COUNT; i++) {
        g_chartData->series[i].name = chartSeriesNames[i];
        g_chartData->series[i].color = chartSeriesColors[i];
        g_chartData->series[i].enabled = (CHART_DEFAULT_MASK & (1 << i)) ? 1 : 0;
        g_chartData->series[i].maxValue = 1;
        g_chartData->series[i].minValue = 0;
        memset(g_chartData->series[i].monthlyData, 0, sizeof(g_chartData->series[i].monthlyData));
        memset(g_chartData->series[i].yearlyData, 0, sizeof(g_chartData->series[i].yearlyData));
    }
    
    g_chartData->currentRange = CHART_RANGE_10_YEARS;
    g_chartData->visibilityMask = CHART_DEFAULT_MASK;
    g_chartData->needsRedraw = 1;
    g_chartData->hwnd = NULL;
    g_chartData->hdcMem = NULL;
    g_chartData->hBitmap = NULL;
    
    /* Set up graph rectangle */
    g_chartData->graphRect.left = CHART_GRAPH_X;
    g_chartData->graphRect.top = CHART_GRAPH_Y;
    g_chartData->graphRect.right = CHART_GRAPH_X + CHART_GRAPH_WIDTH;
    g_chartData->graphRect.bottom = CHART_GRAPH_Y + CHART_GRAPH_HEIGHT;
    
    return 1;
}

/* Cleanup chart system */
void CleanupChartSystem(void) {
    if (!g_chartData) {
        return;
    }
    
    if (g_chartData->hBitmap) {
        DeleteObject(g_chartData->hBitmap);
    }
    
    if (g_chartData->hdcMem) {
        DeleteDC(g_chartData->hdcMem);
    }
    
    free(g_chartData);
    g_chartData = NULL;
}

/* Update chart data from current simulation state */
void UpdateChartData(void) {
    short populationValue;
    short residentialValue;
    short commercialValue;
    short industrialValue;
    short fundsValue;
    short crimeValue;
    short pollutionValue;
    short landValueValue;
    short infrastructureValue;
    short powerValue;
    short growthRateValue;
    short approvalValue;
    short rDemandValue;
    short cDemandValue;
    short iDemandValue;
    
    if (!g_chartData) {
        return;
    }
    
    /* Calculate chart values from simulation data */
    populationValue = (short)(CityPop / 100);  /* Scale down population */
    residentialValue = (short)(ResPop / 8);    /* Match original scaling */
    commercialValue = (short)ComPop;
    industrialValue = (short)IndPop;
    
    /* Scale funds to show in thousands of dollars (0-255 range = $0-255k) */
    fundsValue = (short)(TotalFunds / 1000);
    if (fundsValue < 0) fundsValue = 0;
    if (fundsValue > 255) fundsValue = 255;
    
    crimeValue = (short)CrimeAverage;
    pollutionValue = (short)PollutionAverage;
    landValueValue = (short)LVAverage;
    
    infrastructureValue = (short)((RoadTotal + RailTotal) / 10);  /* Scale infrastructure */
    
    /* Calculate power percentage */
    if (PwrdZCnt + UnpwrdZCnt > 0) {
        powerValue = (short)((PwrdZCnt * 100) / (PwrdZCnt + UnpwrdZCnt));
    } else {
        powerValue = 0;
    }
    
    /* Calculate growth rate */
    if (LastTotalPop > 0) {
        growthRateValue = (short)(((TotalPop - LastTotalPop) * 100) / LastTotalPop + 128);
    } else {
        growthRateValue = 128;  /* Neutral growth */
    }
    if (growthRateValue < 0) growthRateValue = 0;
    if (growthRateValue > 255) growthRateValue = 255;
    
    /* Calculate approval rating */
    if (CityYes + CityNo > 0) {
        approvalValue = (short)((CityYes * 100) / (CityYes + CityNo));
    } else {
        approvalValue = 50;  /* Neutral approval */
    }
    
    /* Scale RCI demand values from -2000..2000 range to 0..255 for charting */
    rDemandValue = (short)((RValve + 2000) / 16);  /* -2000..2000 -> 0..250 */
    cDemandValue = (short)((CValve + 2000) / 16);
    iDemandValue = (short)((IValve + 2000) / 16);
    if (rDemandValue > 255) rDemandValue = 255;
    if (cDemandValue > 255) cDemandValue = 255;
    if (iDemandValue > 255) iDemandValue = 255;
    
    /* Add data points to chart series */
    AddChartDataPoint(CHART_POPULATION, populationValue);
    AddChartDataPoint(CHART_RESIDENTIAL, residentialValue);
    AddChartDataPoint(CHART_COMMERCIAL, commercialValue);
    AddChartDataPoint(CHART_INDUSTRIAL, industrialValue);
    AddChartDataPoint(CHART_FUNDS, fundsValue);
    AddChartDataPoint(CHART_CRIME, crimeValue);
    AddChartDataPoint(CHART_POLLUTION, pollutionValue);
    AddChartDataPoint(CHART_LAND_VALUE, landValueValue);
    AddChartDataPoint(CHART_INFRASTRUCTURE, infrastructureValue);
    AddChartDataPoint(CHART_POWER, powerValue);
    AddChartDataPoint(CHART_GROWTH_RATE, growthRateValue);
    AddChartDataPoint(CHART_APPROVAL, approvalValue);
    AddChartDataPoint(CHART_R_DEMAND, rDemandValue);
    AddChartDataPoint(CHART_C_DEMAND, cDemandValue);
    AddChartDataPoint(CHART_I_DEMAND, iDemandValue);
    
    g_chartData->needsRedraw = 1;
}

/* Add a data point to a chart series */
void AddChartDataPoint(int seriesType, short value) {
    int yearlyStartIndex;
    
    if (!g_chartData || seriesType < 0 || seriesType >= CHART_SERIES_COUNT) {
        return;
    }
    
    /* Update monthly data (shift and add new value at index 0) */
    if (CHART_SHORT_RANGE > 1) {
        memmove(&g_chartData->series[seriesType].monthlyData[1],
                &g_chartData->series[seriesType].monthlyData[0],
                (CHART_SHORT_RANGE - 1) * sizeof(short));
    }
    g_chartData->series[seriesType].monthlyData[0] = value;
    
    /* Update yearly data every 12 months */
    if (CityMonth == 12) {  /* End of year */
        yearlyStartIndex = CHART_HISTLEN - CHART_LONG_RANGE;
        
        /* Verify array bounds before memmove */
        if (yearlyStartIndex >= 0 && 
            yearlyStartIndex + CHART_LONG_RANGE <= CHART_HISTLEN &&
            CHART_LONG_RANGE > 1) {
            
            memmove(&g_chartData->series[seriesType].yearlyData[yearlyStartIndex + 1],
                    &g_chartData->series[seriesType].yearlyData[yearlyStartIndex],
                    (CHART_LONG_RANGE - 1) * sizeof(short));
            g_chartData->series[seriesType].yearlyData[yearlyStartIndex] = value;
        }
    }
    
    /* Update min/max values for scaling */
    if (value > g_chartData->series[seriesType].maxValue) {
        g_chartData->series[seriesType].maxValue = value;
    }
    if (value < g_chartData->series[seriesType].minValue) {
        g_chartData->series[seriesType].minValue = value;
    }
}

/* Scroll chart data (called during time progression) */
void ScrollChartData(void) {
    if (!g_chartData) {
        return;
    }
    
    /* Update chart data from current state */
    UpdateChartData();
}

/* Set visibility of a chart series */
void SetChartVisibility(int seriesType, int visible) {
    if (!g_chartData || seriesType < 0 || seriesType >= CHART_SERIES_COUNT) {
        return;
    }
    
    g_chartData->series[seriesType].enabled = visible ? 1 : 0;
    
    if (visible) {
        g_chartData->visibilityMask |= (1 << seriesType);
    } else {
        g_chartData->visibilityMask &= ~(1 << seriesType);
    }
    
    g_chartData->needsRedraw = 1;
}

/* Get visibility of a chart series */
int GetChartVisibility(int seriesType) {
    if (!g_chartData || seriesType < 0 || seriesType >= CHART_SERIES_COUNT) {
        return 0;
    }
    
    return g_chartData->series[seriesType].enabled;
}

/* Set chart time range */
void SetChartRange(int range) {
    if (!g_chartData) {
        return;
    }
    
    if (range == CHART_RANGE_10_YEARS || range == CHART_RANGE_120_YEARS) {
        g_chartData->currentRange = range;
        g_chartData->needsRedraw = 1;
    }
}

/* Get chart time range */
int GetChartRange(void) {
    if (!g_chartData) {
        return CHART_RANGE_10_YEARS;
    }
    
    return g_chartData->currentRange;
}

/* Toggle visibility of a chart series */
void ToggleChartSeries(int seriesType) {
    if (!g_chartData || seriesType < 0 || seriesType >= CHART_SERIES_COUNT) {
        return;
    }
    
    SetChartVisibility(seriesType, !g_chartData->series[seriesType].enabled);
}

/* Clear all chart data */
void ClearChartData(void) {
    int i;
    
    if (!g_chartData) {
        return;
    }
    
    for (i = 0; i < CHART_SERIES_COUNT; i++) {
        memset(g_chartData->series[i].monthlyData, 0, sizeof(g_chartData->series[i].monthlyData));
        memset(g_chartData->series[i].yearlyData, 0, sizeof(g_chartData->series[i].yearlyData));
        g_chartData->series[i].maxValue = 1;
        g_chartData->series[i].minValue = 0;
    }
    
    g_chartData->needsRedraw = 1;
}

/* Get chart series name */
const char* GetChartSeriesName(int seriesType) {
    if (seriesType < 0 || seriesType >= CHART_SERIES_COUNT) {
        return "Unknown";
    }
    
    return chartSeriesNames[seriesType];
}

/* Get chart series color */
COLORREF GetChartSeriesColor(int seriesType) {
    if (seriesType < 0 || seriesType >= CHART_SERIES_COUNT) {
        return RGB(0, 0, 0);
    }
    
    return chartSeriesColors[seriesType];
}

/* Get chart data value at specific index */
short GetChartDataValue(int seriesType, int index) {
    short value;
    
    if (!g_chartData || seriesType < 0 || seriesType >= CHART_SERIES_COUNT) {
        return 0;
    }
    
    if (g_chartData->currentRange == CHART_RANGE_10_YEARS) {
        if (index >= 0 && index < CHART_SHORT_RANGE) {
            value = g_chartData->series[seriesType].monthlyData[index];
        } else {
            value = 0;
        }
    } else {
        if (index >= 0 && index < CHART_LONG_RANGE) {
            value = g_chartData->series[seriesType].yearlyData[CHART_HISTLEN - CHART_LONG_RANGE + index];
        } else {
            value = 0;
        }
    }
    
    
    return value;
}

/* Get chart data count for current range */
int GetChartDataCount(void) {
    if (!g_chartData) {
        return 0;
    }
    
    return (g_chartData->currentRange == CHART_RANGE_10_YEARS) ? CHART_SHORT_RANGE : CHART_LONG_RANGE;
}

/* Initialize chart window graphics resources */
int InitChartWindowGraphics(HWND hwnd) {
    HDC hdc;
    RECT clientRect;
    
    if (!g_chartData) {
        InitChartSystem();
    }
    
    g_chartData->hwnd = hwnd;
    
    /* Get actual window size */
    GetClientRect(hwnd, &clientRect);
    
    /* Update graph rectangle based on actual window size, leaving room for legend at bottom */
    g_chartData->graphRect.left = CHART_GRAPH_MARGIN;
    g_chartData->graphRect.top = CHART_GRAPH_MARGIN;
    g_chartData->graphRect.right = clientRect.right - CHART_GRAPH_MARGIN;
    g_chartData->graphRect.bottom = clientRect.bottom - 60;  /* Leave 60 pixels for legend */
    
    /* Create memory DC for off-screen rendering */
    hdc = GetDC(hwnd);
    if (!hdc) {
        return 0;
    }
    
    g_chartData->hdcMem = CreateCompatibleDC(hdc);
    if (!g_chartData->hdcMem) {
        ReleaseDC(hwnd, hdc);
        return 0;
    }
    
    g_chartData->hBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    if (!g_chartData->hBitmap) {
        DeleteDC(g_chartData->hdcMem);
        ReleaseDC(hwnd, hdc);
        return 0;
    }
    
    if (SelectObject(g_chartData->hdcMem, g_chartData->hBitmap) == NULL) {
        DeleteObject(g_chartData->hBitmap);
        DeleteDC(g_chartData->hdcMem);
        ReleaseDC(hwnd, hdc);
        return 0;
    }
    ReleaseDC(hwnd, hdc);
    
    return 1;
}

/* Show or hide chart window */
void ShowChartWindow(int show) {
    if (g_chartData && g_chartData->hwnd) {
        if (show) {
            SetTimer(g_chartData->hwnd, CHART_TIMER_ID, CHART_TIMER_INTERVAL, NULL);
            ShowWindow(g_chartData->hwnd, SW_SHOW);
        } else {
            KillTimer(g_chartData->hwnd, CHART_TIMER_ID);
            ShowWindow(g_chartData->hwnd, SW_HIDE);
        }
    }
}

/* Show chart context menu */
void ShowChartContextMenu(HWND hwnd, int x, int y) {
    HMENU hMenu;
    HMENU hSubMenu;
    POINT pt;
    int i;
    
    if (!g_chartData) {
        return;
    }
    
    hMenu = CreatePopupMenu();
    if (!hMenu) {
        return;
    }
    
    /* Add chart series toggles */
    for (i = 0; i < CHART_SERIES_COUNT; i++) {
        AppendMenu(hMenu, MF_STRING | (g_chartData->series[i].enabled ? MF_CHECKED : MF_UNCHECKED), 
                   5000 + i, g_chartData->series[i].name);
    }
    
    /* Add separator */
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    
    /* Add time range options */
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING | (g_chartData->currentRange == CHART_RANGE_10_YEARS ? MF_CHECKED : MF_UNCHECKED), 
               6000, "10 Years");
    AppendMenu(hSubMenu, MF_STRING | (g_chartData->currentRange == CHART_RANGE_120_YEARS ? MF_CHECKED : MF_UNCHECKED), 
               6001, "120 Years");
    AppendMenu(hMenu, MF_POPUP, (UINT)hSubMenu, "Time Range");
    
    /* Convert client coordinates to screen coordinates */
    pt.x = x;
    pt.y = y;
    ClientToScreen(hwnd, &pt);
    
    /* Show menu */
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    
    /* Clean up */
    DestroyMenu(hSubMenu);
    DestroyMenu(hMenu);
}

/* Chart window procedure */
LRESULT CALLBACK ChartWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;
    POINT pt;
    
    switch (message) {
        case WM_CREATE:
            /* Initialize chart graphics when window is created */
            InitChartWindowGraphics(hwnd);
            /* Start slower periodic timer for chart updates */
            SetTimer(hwnd, CHART_TIMER_ID, CHART_TIMER_INTERVAL, NULL);
            return 0;
            
        case WM_PAINT: {
            RECT clientRect;
            
            hdc = BeginPaint(hwnd, &ps);
            
            /* Get actual window size */
            GetClientRect(hwnd, &clientRect);
            
            /* Initialize graphics if not already done */
            if (g_chartData && !g_chartData->hdcMem) {
                InitChartWindowGraphics(hwnd);
            }
            
            if (g_chartData && g_chartData->hdcMem) {
                RedrawChart(g_chartData->hdcMem);
                BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom,
                       g_chartData->hdcMem, 0, 0, SRCCOPY);
            } else {
                /* Draw a simple background when graphics aren't ready */
                FillRect(hdc, &clientRect, GetStockObject(WHITE_BRUSH)); 
                TextOut(hdc, 10, 10, "Chart system initializing...", 27);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
            
        case WM_RBUTTONDOWN:
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ShowChartContextMenu(hwnd, pt.x, pt.y);
            return 0;
            
        case WM_COMMAND:
            /* Handle context menu commands */
            if (LOWORD(wParam) >= 5000 && LOWORD(wParam) < 5000 + CHART_SERIES_COUNT) {
                /* Toggle chart series */
                ToggleChartSeries(LOWORD(wParam) - 5000);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else if (LOWORD(wParam) == 6000) {
                /* 10 years range */
                SetChartRange(CHART_RANGE_10_YEARS);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else if (LOWORD(wParam) == 6001) {
                /* 120 years range */
                SetChartRange(CHART_RANGE_120_YEARS);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            break;
            
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                KillTimer(hwnd, CHART_TIMER_ID);
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            break;
            
        case WM_SIZE:
            /* Window was resized - update chart graphics resources */
            if (g_chartData && g_chartData->hdcMem) {
                /* Clean up old graphics resources */
                if (g_chartData->hBitmap) {
                    DeleteObject(g_chartData->hBitmap);
                    g_chartData->hBitmap = NULL;
                }
                if (g_chartData->hdcMem) {
                    DeleteDC(g_chartData->hdcMem);
                    g_chartData->hdcMem = NULL;
                }
                
                /* Recreate graphics resources with new size */
                InitChartWindowGraphics(hwnd);
                
                /* Force redraw */
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
            
        case WM_TIMER:
            if (wParam == CHART_TIMER_ID) {
                /* Periodic chart refresh */
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            break;
            
        case WM_CLOSE:
            /* Hide window instead of destroying and update menu checkmark */
            KillTimer(hwnd, CHART_TIMER_ID);
            ShowWindow(hwnd, SW_HIDE);
            
            /* Update menu checkmark */
            if (hwndMain) {
                HMENU hMenu = GetMenu(hwndMain);
                HMENU hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */
                if (hViewMenu) {
                    CheckMenuItem(hViewMenu, IDM_VIEW_CHARTSWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
                }
            }
            return 0;
            
        case WM_DESTROY:
            KillTimer(hwnd, CHART_TIMER_ID);
            if (g_chartData) {
                g_chartData->hwnd = NULL;
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/* Redraw entire chart */
void RedrawChart(HDC hdc) {
    if (!g_chartData) {
        return;
    }
    
    DrawChartBackground(hdc);
    DrawChartGrid(hdc);
    DrawChartAxes(hdc);
    
    /* Draw all visible series */
    {
        int i;
        for (i = 0; i < CHART_SERIES_COUNT; i++) {
            if (g_chartData->series[i].enabled) {
                DrawChartSeries(hdc, i);
            }
        }
    }
    
    DrawChartLegend(hdc);
    g_chartData->needsRedraw = 0;
}

/* Draw chart background */
void DrawChartBackground(HDC hdc) {
    RECT rect;
    HBRUSH hBrush;
    
    /* Get actual client area size */
    GetClientRect(g_chartData->hwnd, &rect);
    
    /* Clear entire window with light grey background using system color */
    hBrush = CreateSolidBrush(RGB(192, 192, 192));
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
}

/* Draw chart grid */
void DrawChartGrid(HDC hdc) {
    HPEN hPen;
    HPEN hOldPen;
    int i;
    int x, y;
    int stepX, stepY;
    int graphWidth, graphHeight;
    
    graphWidth = g_chartData->graphRect.right - g_chartData->graphRect.left;
    graphHeight = g_chartData->graphRect.bottom - g_chartData->graphRect.top;
    
    hPen = CreatePen(PS_DOT, 1, RGB(240, 240, 240));
    hOldPen = SelectObject(hdc, hPen);
    
    /* Vertical grid lines */
    stepX = graphWidth / 10;
    for (i = 1; i < 10; i++) {
        x = g_chartData->graphRect.left + i * stepX;
        MoveToEx(hdc, x, g_chartData->graphRect.top, NULL);
        LineTo(hdc, x, g_chartData->graphRect.bottom);
    }
    
    /* Horizontal grid lines */
    stepY = graphHeight / 8;
    for (i = 1; i < 8; i++) {
        y = g_chartData->graphRect.top + i * stepY;
        MoveToEx(hdc, g_chartData->graphRect.left, y, NULL);
        LineTo(hdc, g_chartData->graphRect.right, y);
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

/* Draw chart axes */
void DrawChartAxes(HDC hdc) {
    HPEN hPen;
    HPEN hOldPen;
    HFONT hFont;
    HFONT hOldFont;
    char text[64];
    int timeRange;
    int i;
    int x, y;
    int maxValue;
    int stepX, stepY;
    int graphWidth, graphHeight;
    HBRUSH hChartBrush;
    
    graphWidth = g_chartData->graphRect.right - g_chartData->graphRect.left;
    graphHeight = g_chartData->graphRect.bottom - g_chartData->graphRect.top;
    
    /* Create a readable font for axis labels */
    hFont = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
    hOldFont = SelectObject(hdc, hFont);
    
    /* Set transparent background for text */
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(80, 80, 80));
    
    /* Fill chart area with white background */
    hChartBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &g_chartData->graphRect, hChartBrush);
    DeleteObject(hChartBrush);
    
    /* Draw chart border */
    hPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
    hOldPen = SelectObject(hdc, hPen);
    
    MoveToEx(hdc, g_chartData->graphRect.left, g_chartData->graphRect.top, NULL);
    LineTo(hdc, g_chartData->graphRect.right, g_chartData->graphRect.top);
    LineTo(hdc, g_chartData->graphRect.right, g_chartData->graphRect.bottom);
    LineTo(hdc, g_chartData->graphRect.left, g_chartData->graphRect.bottom);
    LineTo(hdc, g_chartData->graphRect.left, g_chartData->graphRect.top);
    
    /* Find maximum value across all enabled series for Y-axis scaling */
    maxValue = 1;
    for (i = 0; i < CHART_SERIES_COUNT; i++) {
        if (g_chartData->series[i].enabled && g_chartData->series[i].maxValue > maxValue) {
            maxValue = g_chartData->series[i].maxValue;
        }
    }
    
    /* Add 10% margin above max value for better visual scaling */
    maxValue = maxValue + (maxValue / 10);
    if (maxValue < 1) maxValue = 1;
    
    /* Draw Y-axis labels (values) */
    SetTextColor(hdc, RGB(60, 60, 60));
    stepY = graphHeight / 4;  /* 4 Y-axis labels */
    for (i = 0; i <= 4; i++) {
        int value = (maxValue * (4 - i)) / 4;
        y = g_chartData->graphRect.top + i * stepY;
        
        sprintf(text, "%d", value);
        TextOut(hdc, g_chartData->graphRect.left - 30, y - 6, text, (int)(int)strlen(text));
        
        /* Draw tick marks */
        MoveToEx(hdc, g_chartData->graphRect.left - 3, y, NULL);
        LineTo(hdc, g_chartData->graphRect.left, y);
    }
    
    /* Draw X-axis labels (time) */
    timeRange = g_chartData->currentRange;
    stepX = graphWidth / 5;  /* 5 X-axis labels */
    for (i = 0; i <= 5; i++) {
        x = g_chartData->graphRect.left + i * stepX;
        
        if (timeRange == CHART_RANGE_10_YEARS) {
            /* 10 years = 120 months, show in years */
            int years = 10 - (i * 2);  /* 10, 8, 6, 4, 2, 0 years ago */
            if (years == 0) {
                sprintf(text, "Now");
            } else {
                sprintf(text, "%dy", years);
            }
        } else {
            /* 120 years */
            int years = 120 - (i * 24);  /* 120, 96, 72, 48, 24, 0 years ago */
            if (years == 0) {
                sprintf(text, "Now");
            } else {
                sprintf(text, "%dy", years);
            }
        }
        
        TextOut(hdc, x - 10, g_chartData->graphRect.bottom + 5, text, (int)strlen(text));
        
        /* Draw tick marks */
        MoveToEx(hdc, x, g_chartData->graphRect.bottom, NULL);
        LineTo(hdc, x, g_chartData->graphRect.bottom + 3);
    }
    
    /* Y-axis title removed - redundant */
    
    /* X-axis title removed - redundant */
    
    /* Instructions moved to window title */
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

/* Draw a single chart series */
void DrawChartSeries(HDC hdc, int seriesType) {
    HPEN hPen;
    HPEN hOldPen;
    int i;
    int dataCount;
    int x, y;
    int prevX, prevY;
    short value;
    short maxVal, minVal;
    
    if (!g_chartData || seriesType < 0 || seriesType >= CHART_SERIES_COUNT) {
        return;
    }
    
    if (!g_chartData->series[seriesType].enabled) {
        return;
    }
    
    dataCount = GetChartDataCount();
    if (dataCount <= 1) {
        return;
    }
    
    /* Find overall max value across all enabled series for consistent scaling */
    maxVal = 1;
    minVal = 0;
    for (i = 0; i < CHART_SERIES_COUNT; i++) {
        if (g_chartData->series[i].enabled && g_chartData->series[i].maxValue > maxVal) {
            maxVal = g_chartData->series[i].maxValue;
        }
    }
    
    /* Add 10% margin above max value for better visual scaling */
    maxVal = maxVal + (maxVal / 10);
    if (maxVal < 1) maxVal = 1;
    
    
    hPen = CreatePen(PS_SOLID, 2, g_chartData->series[seriesType].color);
    hOldPen = SelectObject(hdc, hPen);
    
    /* Draw line series */
    for (i = 0; i < dataCount; i++) {
        value = GetChartDataValue(seriesType, dataCount - 1 - i);  /* Reverse order for time axis */
        
        x = g_chartData->graphRect.left + (i * (g_chartData->graphRect.right - g_chartData->graphRect.left)) / (dataCount - 1);
        y = g_chartData->graphRect.bottom - ((value - minVal) * (g_chartData->graphRect.bottom - g_chartData->graphRect.top)) / (maxVal - minVal);
        
        if (i == 0) {
            MoveToEx(hdc, x, y, NULL);
        } else {
            LineTo(hdc, x, y);
        }
        
        prevX = x;
        prevY = y;
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

/* Draw chart legend */
void DrawChartLegend(HDC hdc) {
    HFONT hFont;
    HFONT hOldFont;
    HPEN hPen;
    HPEN hOldPen;
    int i;
    int x, y;
    int legendCount;
    int itemsPerRow;
    int col;
    int itemWidth;
    RECT clientRect;
    
    /* Count enabled series for layout */
    legendCount = 0;
    for (i = 0; i < CHART_SERIES_COUNT; i++) {
        if (g_chartData->series[i].enabled) {
            legendCount++;
        }
    }
    
    if (legendCount == 0) {
        return; /* No legend to draw */
    }
    
    /* Get window size for legend positioning */
    GetClientRect(g_chartData->hwnd, &clientRect);
    
    /* Create readable font for legend */
    hFont = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
    hOldFont = SelectObject(hdc, hFont);
    
    SetBkMode(hdc, TRANSPARENT);
    
    /* Calculate legend layout - items in rows at bottom */
    itemWidth = 100;  /* Width per legend item */
    itemsPerRow = (clientRect.right - 20) / itemWidth;  /* How many items fit per row */
    if (itemsPerRow < 1) itemsPerRow = 1;
    
    /* Start legend below chart area */
    x = 10;
    y = g_chartData->graphRect.bottom + 35;
    
    col = 0;
    for (i = 0; i < CHART_SERIES_COUNT; i++) {
        if (g_chartData->series[i].enabled) {
            /* Draw colored line indicator */
            hPen = CreatePen(PS_SOLID, 3, g_chartData->series[i].color);
            hOldPen = SelectObject(hdc, hPen);
            
            MoveToEx(hdc, x, y, NULL);
            LineTo(hdc, x + 15, y);
            
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            /* Draw series name */
            SetTextColor(hdc, RGB(60, 60, 60));
            TextOut(hdc, x + 20, y - 6, g_chartData->series[i].name, (int)strlen(g_chartData->series[i].name));
            
            /* Move to next position */
            col++;
            if (col >= itemsPerRow) {
                /* Start new row */
                col = 0;
                x = 10;
                y += 15;
            } else {
                /* Next column */
                x += itemWidth;
            }
        }
    }
    
    /* Add power information at the bottom */
    if (PwrdZCnt > 0 || UnpwrdZCnt > 0) {
        char powerBuffer[128];
        y += 20;
        SetTextColor(hdc, RGB(60, 60, 60));
        sprintf(powerBuffer, "Power: Powered=%d Unpowered=%d", PwrdZCnt, UnpwrdZCnt);
        TextOut(hdc, 10, y, powerBuffer, (int)strlen(powerBuffer));
    }
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}