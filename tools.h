/* tools.h - Tool handling definitions for WiNTown (Windows NT version)
 * Based on original WiNTown code from WiNTownLegacy project
 */

#ifndef _TOOLS_H
#define _TOOLS_H

#include <windows.h>

/* Tool active flag */
extern int isToolActive;

/* Tool drag state */
extern BOOL isToolDragging;

/* Tool result constants */
#define TOOLRESULT_OK           0
#define TOOLRESULT_FAILED       1
#define TOOLRESULT_NO_MONEY     2
#define TOOLRESULT_NEED_BULLDOZE 3

/* Tool cost constants */
#define TOOL_BULLDOZER_COST     1
#define TOOL_ROAD_COST          10
#define TOOL_RAIL_COST          20
#define TOOL_WIRE_COST          5
#define TOOL_PARK_COST          10
#define TOOL_RESIDENTIAL_COST   100
#define TOOL_COMMERCIAL_COST    100
#define TOOL_INDUSTRIAL_COST    100
#define TOOL_FIRESTATION_COST   500
#define TOOL_POLICESTATION_COST 500
#define TOOL_STADIUM_COST       5000
#define TOOL_SEAPORT_COST       3000
#define TOOL_POWERPLANT_COST    3000
#define TOOL_NUCLEAR_COST       5000
#define TOOL_AIRPORT_COST       10000
#define TOOL_NETWORK_COST       100

/* Tool size constants */
#define TOOL_SIZE_1X1           1  /* Single tile tools (road, rail, wire, etc.) */
#define TOOL_SIZE_3X3           3  /* 3x3 zone tools (residential, commercial, industrial, etc.) */
#define TOOL_SIZE_4X4           4  /* 4x4 building tools (stadium, power plant, etc.) */
#define TOOL_SIZE_6X6           6  /* 6x6 building tools (airport) */

/* Functions for tool management */
void CreateToolbar(HWND hwndParent, int x, int y, int width, int height);
void SelectTool(int toolType);
int ApplyTool(int mapX, int mapY);
int GetCurrentTool(void);
int GetToolResult(void);
int GetToolCost(void);
int GetToolSize(int toolType);
void DrawToolIcon(HDC hdc, int toolType, int x, int y, int desiredWidth, int desiredHeight, int isSelected);
void LoadToolbarBitmaps(void);
void CleanupToolbarBitmaps(void);
void ScreenToMap(int screenX, int screenY, int *mapX, int *mapY, int xOffset, int yOffset);
int HandleToolMouse(int mouseX, int mouseY, int xOffset, int yOffset);
void DrawToolHover(HDC hdc, int mapX, int mapY, int toolType, int xOffset, int yOffset);
void UpdateToolbar(void);

/* Individual tool functions */
int DoBulldozer(int mapX, int mapY);
int DoRoad(int mapX, int mapY);
int DoRail(int mapX, int mapY);
int DoWire(int mapX, int mapY);
int DoPark(int mapX, int mapY);
int DoResidential(int mapX, int mapY);
int DoCommercial(int mapX, int mapY);
int DoIndustrial(int mapX, int mapY);
int DoFireStation(int mapX, int mapY);
int DoPoliceStation(int mapX, int mapY);
int DoPowerPlant(int mapX, int mapY);
int DoNuclearPlant(int mapX, int mapY);
int DoStadium(int mapX, int mapY);
int DoSeaport(int mapX, int mapY);
int DoAirport(int mapX, int mapY);
int DoQuery(int mapX, int mapY);

/* Zone name lookup function */
const char *GetZoneName(short tile);

#endif /* _TOOLS_H */