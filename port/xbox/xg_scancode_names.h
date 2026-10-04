/*
 * xg_scancode_names.h: keyboard key names for hosts without SDL (iOS).
 *
 * Upstream build 85 names keyboard bindings through SDL_GetScancodeName and
 * SDL_GetScancodeFromName. Scancodes are USB HID usages, as in SDL; the names
 * follow SDL's for the keys a game binding can use. Unknown keys give "" and
 * unknown names 0 (SDL_SCANCODE_UNKNOWN), as SDL does.
 */
#ifndef XG_SCANCODE_NAMES_H
#define XG_SCANCODE_NAMES_H

#include <string.h>
#include <strings.h>

static const struct { int code; const char *name; } xg_scancode_names[] = {
	{4, "A"}, {5, "B"}, {6, "C"}, {7, "D"}, {8, "E"}, {9, "F"}, {10, "G"}, {11, "H"}, {12, "I"},
	{13, "J"}, {14, "K"}, {15, "L"}, {16, "M"}, {17, "N"}, {18, "O"}, {19, "P"}, {20, "Q"},
	{21, "R"}, {22, "S"}, {23, "T"}, {24, "U"}, {25, "V"}, {26, "W"}, {27, "X"}, {28, "Y"},
	{29, "Z"}, {30, "1"}, {31, "2"}, {32, "3"}, {33, "4"}, {34, "5"}, {35, "6"}, {36, "7"},
	{37, "8"}, {38, "9"}, {39, "0"}, {40, "Return"}, {41, "Escape"}, {42, "Backspace"},
	{43, "Tab"}, {44, "Space"}, {45, "-"}, {46, "="}, {47, "["}, {48, "]"}, {49, "\\"},
	{51, ";"}, {52, "'"}, {53, "`"}, {54, ","}, {55, "."}, {56, "/"}, {57, "CapsLock"},
	{58, "F1"}, {59, "F2"}, {60, "F3"}, {61, "F4"}, {62, "F5"}, {63, "F6"}, {64, "F7"},
	{65, "F8"}, {66, "F9"}, {67, "F10"}, {68, "F11"}, {69, "F12"}, {73, "Insert"},
	{74, "Home"}, {75, "PageUp"}, {76, "Delete"}, {77, "End"}, {78, "PageDown"},
	{79, "Right"}, {80, "Left"}, {81, "Down"}, {82, "Up"}, {84, "Keypad /"}, {85, "Keypad *"},
	{86, "Keypad -"}, {87, "Keypad +"}, {88, "Keypad Enter"}, {89, "Keypad 1"},
	{90, "Keypad 2"}, {91, "Keypad 3"}, {92, "Keypad 4"}, {93, "Keypad 5"}, {94, "Keypad 6"},
	{95, "Keypad 7"}, {96, "Keypad 8"}, {97, "Keypad 9"}, {98, "Keypad 0"}, {99, "Keypad ."},
	{224, "Left Ctrl"}, {225, "Left Shift"}, {226, "Left Alt"}, {227, "Left GUI"},
	{228, "Right Ctrl"}, {229, "Right Shift"}, {230, "Right Alt"}, {231, "Right GUI"},
};

static inline const char *xg_scancode_name(int code)
{
	for (size_t i = 0; i < sizeof xg_scancode_names / sizeof *xg_scancode_names; i++)
		if (xg_scancode_names[i].code == code)
			return xg_scancode_names[i].name;
	return "";
}

static inline int xg_scancode_from_name(const char *name)
{
	if (!name || !*name)
		return 0;
	for (size_t i = 0; i < sizeof xg_scancode_names / sizeof *xg_scancode_names; i++)
		if (!strcasecmp(xg_scancode_names[i].name, name))
			return xg_scancode_names[i].code;
	return 0;
}

#endif
