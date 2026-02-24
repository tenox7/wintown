/* charts.h - Chart system for WiNTown
 * Comprehensive charting system with multiple data series and time ranges
 */

#ifndef _CHARTS_H
#define _CHARTS_H

#include <windows.h>

/* Chart constants */
#define CHART_HISTLEN        240    /* History length (matches HISTLEN from sim.h) */
#define CHART_SHORT_RANGE    120    /* 10 years of monthly data (120 months) */
#define CHART_LONG_RANGE     120    /* 120 years of yearly data */
#define CHART_SERIES_COUNT   15     /* Number of different chart series */

/* Chart series types */
#define CHART_POPULATION     0      /* Total city population */
#define CHART_RESIDENTIAL    1      /* Residential population */
#define CHART_COMMERCIAL     2      /* Commercial population */
#define CHART_INDUSTRIAL     3      /* Industrial population */
#define CHART_FUNDS          4      /* City funds / cash flow */
#define CHART_CRIME          5      /* Crime level */
#define CHART_POLLUTION      6      /* Pollution level */
#define CHART_LAND_VALUE     7      /* Land value average */
#define CHART_INFRASTRUCTURE 8      /* Infrastructure count (roads + rail) */
#define CHART_POWER          9      /* Powered zones ratio */
#define CHART_GROWTH_RATE    10     /* Population growth rate */
#define CHART_APPROVAL       11     /* City approval rating */
#define CHART_R_DEMAND       12     /* Residential demand */
#define CHART_C_DEMAND       13     /* Commercial demand */
#define CHART_I_DEMAND       14     /* Industrial demand */

/* Chart time ranges */
#define CHART_RANGE_10_YEARS  0     /* 10 years (120 months) */
#define CHART_RANGE_120_YEARS 1     /* 120 years */

/* Chart data visibility mask bits */
#define CHART_MASK_POPULATION     (1 << CHART_POPULATION)
#define CHART_MASK_RESIDENTIAL    (1 << CHART_RESIDENTIAL)
#define CHART_MASK_COMMERCIAL     (1 << CHART_COMMERCIAL)
#define CHART_MASK_INDUSTRIAL     (1 << CHART_INDUSTRIAL)
#define CHART_MASK_FUNDS          (1 << CHART_FUNDS)
#define CHART_MASK_CRIME          (1 << CHART_CRIME)
#define CHART_MASK_POLLUTION      (1 << CHART_POLLUTION)
#define CHART_MASK_LAND_VALUE     (1 << CHART_LAND_VALUE)
#define CHART_MASK_INFRASTRUCTURE (1 << CHART_INFRASTRUCTURE)
#define CHART_MASK_POWER          (1 << CHART_POWER)
#define CHART_MASK_GROWTH_RATE    (1 << CHART_GROWTH_RATE)
#define CHART_MASK_APPROVAL       (1 << CHART_APPROVAL)
#define CHART_MASK_R_DEMAND       (1 << CHART_R_DEMAND)
#define CHART_MASK_C_DEMAND       (1 << CHART_C_DEMAND)
#define CHART_MASK_I_DEMAND       (1 << CHART_I_DEMAND)

/* Default visible charts */
#define CHART_DEFAULT_MASK  (CHART_MASK_POPULATION | CHART_MASK_RESIDENTIAL | \
                            CHART_MASK_COMMERCIAL | CHART_MASK_INDUSTRIAL | \
                            CHART_MASK_FUNDS | CHART_MASK_CRIME)

/* Chart colors (Windows system palette) */
#define CHART_COLOR_POPULATION     RGB(0, 128, 0)      /* Dark Green */
#define CHART_COLOR_RESIDENTIAL    RGB(0, 255, 0)      /* Light Green */
#define CHART_COLOR_COMMERCIAL     RGB(0, 0, 128)      /* Dark Blue */
#define CHART_COLOR_INDUSTRIAL     RGB(255, 255, 0)    /* Yellow */
#define CHART_COLOR_FUNDS          RGB(0, 0, 255)      /* Blue */
#define CHART_COLOR_CRIME          RGB(255, 0, 0)      /* Red */
#define CHART_COLOR_POLLUTION      RGB(128, 128, 0)    /* Olive */
#define CHART_COLOR_LAND_VALUE     RGB(128, 0, 128)    /* Purple */
#define CHART_COLOR_INFRASTRUCTURE RGB(128, 128, 128)  /* Gray */
#define CHART_COLOR_POWER          RGB(255, 165, 0)    /* Orange */
#define CHART_COLOR_GROWTH_RATE    RGB(0, 255, 255)    /* Cyan */
#define CHART_COLOR_APPROVAL       RGB(255, 192, 203)  /* Pink */
#define CHART_COLOR_R_DEMAND       RGB(0, 200, 100)    /* Light Green */
#define CHART_COLOR_C_DEMAND       RGB(100, 100, 255)  /* Light Blue */
#define CHART_COLOR_I_DEMAND       RGB(255, 200, 0)    /* Light Orange */

/* Chart window dimensions */
#define CHART_WINDOW_WIDTH   600
#define CHART_WINDOW_HEIGHT  450
#define CHART_GRAPH_MARGIN   20  /* Margin for axes and labels */
#define CHART_GRAPH_X        CHART_GRAPH_MARGIN
#define CHART_GRAPH_Y        CHART_GRAPH_MARGIN
#define CHART_GRAPH_WIDTH    (CHART_WINDOW_WIDTH - 2 * CHART_GRAPH_MARGIN)
#define CHART_GRAPH_HEIGHT   (CHART_WINDOW_HEIGHT - 2 * CHART_GRAPH_MARGIN)

/* Chart window timer */
#define CHART_TIMER_ID       4
#define CHART_TIMER_INTERVAL 5000  /* Update charts every 5 seconds for periodic refresh */

/* Chart data storage structure */
typedef struct {
    short monthlyData[CHART_HISTLEN];   /* Monthly data (240 entries, 0-119 for 10 years) */
    short yearlyData[CHART_HISTLEN];    /* Yearly data (120 entries in indices 120-239) */
    short maxValue;                      /* Maximum value for scaling */
    short minValue;                      /* Minimum value for scaling */
    const char* name;                    /* Series name */
    COLORREF color;                      /* Chart color */
    int enabled;                         /* Is this series visible */
} ChartSeries;

/* Main chart data structure */
typedef struct {
    ChartSeries series[CHART_SERIES_COUNT];
    int currentRange;                    /* CHART_RANGE_10_YEARS or CHART_RANGE_120_YEARS */
    int visibilityMask;                  /* Bitmask of visible series */
    int needsRedraw;                     /* Flag indicating chart needs redraw */
    HWND hwnd;                           /* Chart window handle */
    HDC hdcMem;                          /* Memory DC for off-screen rendering */
    HBITMAP hBitmap;                     /* Bitmap for off-screen rendering */
    RECT graphRect;                      /* Rectangle for graph area */
} ChartData;

/* Global chart data */
extern ChartData* g_chartData;

/* Chart system functions */
int InitChartSystem(void);
void CleanupChartSystem(void);
void UpdateChartData(void);
void AddChartDataPoint(int seriesType, short value);
void ScrollChartData(void);
void SetChartVisibility(int seriesType, int visible);
int GetChartVisibility(int seriesType);
void SetChartRange(int range);
int GetChartRange(void);
void ToggleChartSeries(int seriesType);
void ClearChartData(void);

/* Chart window functions */
int InitChartWindowGraphics(HWND hwnd);
void ShowChartWindow(int show);
void ShowChartContextMenu(HWND hwnd, int x, int y);
LRESULT CALLBACK ChartWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/* Chart rendering functions */
void RedrawChart(HDC hdc);
void DrawChartBackground(HDC hdc);
void DrawChartGrid(HDC hdc);
void DrawChartAxes(HDC hdc);
void DrawChartSeries(HDC hdc, int seriesType);
void DrawChartLegend(HDC hdc);
void CalculateChartScaling(void);

/* Chart utility functions */
const char* GetChartSeriesName(int seriesType);
COLORREF GetChartSeriesColor(int seriesType);
short GetChartDataValue(int seriesType, int index);
int GetChartDataCount(void);

/* Chart window class name */
#define CHART_WINDOW_CLASS "WiNTownChart"

#endif /* _CHARTS_H */