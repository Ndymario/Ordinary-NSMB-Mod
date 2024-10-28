#include "nsmb.hpp"


/*
	Template file to allow easy ending credits script editing

	End::ScriptEntry { string, page, y position, palette, multipage flag }

	- Due to the game using the char graphic index in the string chars, you are not supposed to use any standard string literal for this purpose
	  To face this issue, the reference contains some utilities to convert strings/chars at compile time like the literal operator ""end (check nsmb/ending/mainscreen/script.h)
	  Make sure to use the wchar_t literal (L""end) instead of a normal one (""end) when using special characters
	  If a given character cannot be converted, compilation will fail (due to End::charConv being consteval)

	- The game reads the string only up to the 41th character, exceeding this limit won't cause anything bad

	- The Y Position is in pixels starting from the top
	
	- The palette is supposed to be a value of the End::ScriptEntry::Palette enum because the other values are used for the white fading

	- Strings starting with a space or with a null terminator aren't handled correctly (give it a try)

	- Multipages entries requires you to duplicate the entry for each page always with the same values
	  Make sure to set the multipage flag to false in the last entry of the sequence

	- Any entry with string == nullptr counts as script terminator (you can use the End::scriptTerminator variable)

	- A max of 128 characters can be loaded at the same time (even if not visible), terminators and spaces excluded
	  Since this code replaces the original script in the binary, a static_assert has been added to avoid overwriting other data

	- The slideshow is not synced with the bottom screen so once the game reaches the end of the script the music will fade out
	  Because of that, try to keep the pages count as close as possible to the original (36 pages) one to avoid a huge delay between the end of the script and the cutscene
*/

ncp_over(0x020EA678, 8)
static constexpr End::ScriptEntry script[] = {

	// Page 0
	{"An Ordinary NSMB Mod"end, 0, 30, End::ScriptEntry::Red, true},
	{"Created By"end, 0, 57, End::ScriptEntry::Red, false},
	{"Ndymario"end, 0, 95, End::ScriptEntry::Blue, false},
	// Page 1
	{"An Ordinary NSMB Mod"end, 1, 30, End::ScriptEntry::Red, true},
	{"Programmers"end, 1, 57, End::ScriptEntry::Red, false},
	{"Ndymario"end, 1, 85, End::ScriptEntry::Blue, false},
	{"TheGameratorT"end, 1, 105, End::ScriptEntry::Blue, false},
	// Page 2
	{"An Ordinary NSMB Mod"end, 2, 30, End::ScriptEntry::Red, true},
	{"Graphics"end, 2, 57, End::ScriptEntry::Red, false},
	{"Slugmotif"end, 2, 95, End::ScriptEntry::Blue, false},
	// Page 3
	{"An Ordinary NSMB Mod"end, 3, 30, End::ScriptEntry::Red, true},
	{"Beta Testers"end, 3, 58, End::ScriptEntry::Red, true},
	{"Frosty Cake"end, 3, 87, End::ScriptEntry::Blue, false},
	{"Illy"end, 3, 105, End::ScriptEntry::Blue, false},
	{"Mosquito"end, 3, 123, End::ScriptEntry::Blue, false},
	{"Mr. Ztardust"end, 3, 141, End::ScriptEntry::Blue, false},
	// Page 4
	{"An Ordinary NSMB Mod"end, 4, 30, End::ScriptEntry::Red, true},
	{"Beta Testers"end, 4, 58, End::ScriptEntry::Red, false},
	{"SeanM"end, 4, 87, End::ScriptEntry::Blue, false},
	// Page 5
	{"An Ordinary NSMB Mod"end, 5, 30, End::ScriptEntry::Red, true},
	// Page 6
	{"An Ordinary NSMB Mod"end, 6, 30, End::ScriptEntry::Red, true},
	// Page 7
	{"An Ordinary NSMB Mod"end, 7, 30, End::ScriptEntry::Red, true},
	// Page 8
	{"An Ordinary NSMB Mod"end, 8, 30, End::ScriptEntry::Red, true},
	// Page 9
	{"An Ordinary NSMB Mod"end, 9, 30, End::ScriptEntry::Red, true},
	// Page 10
	{"An Ordinary NSMB Mod"end, 10, 30, End::ScriptEntry::Red, true},
	// Page 11
	{"An Ordinary NSMB Mod"end, 11, 30, End::ScriptEntry::Red, true},
	// Page 12
	{"An Ordinary NSMB Mod"end, 12, 30, End::ScriptEntry::Red, true},
	// Page 13
	{"An Ordinary NSMB Mod"end, 13, 30, End::ScriptEntry::Red, true},
	// Page 14
	{"An Ordinary NSMB Mod"end, 14, 30, End::ScriptEntry::Red, true},
	// Page 15
	{"An Ordinary NSMB Mod"end, 15, 30, End::ScriptEntry::Red, true},
	// Page 16
	{"An Ordinary NSMB Mod"end, 16, 30, End::ScriptEntry::Red, true},
	// Page 17
	{"An Ordinary NSMB Mod"end, 17, 30, End::ScriptEntry::Red, true},
	// Page 18
	{"An Ordinary NSMB Mod"end, 18, 30, End::ScriptEntry::Red, true},
	// Page 19
	{"An Ordinary NSMB Mod"end, 19, 30, End::ScriptEntry::Red, true},
	// Page 20
	{"An Ordinary NSMB Mod"end, 20, 30, End::ScriptEntry::Red, true},
	// Page 21
	{"An Ordinary NSMB Mod"end, 21, 30, End::ScriptEntry::Red, true},
	// Page 22
	{"An Ordinary NSMB Mod"end, 22, 30, End::ScriptEntry::Red, true},
	// Page 23
	{"An Ordinary NSMB Mod"end, 23, 30, End::ScriptEntry::Red, true},
	// Page 24
	{"An Ordinary NSMB Mod"end, 24, 30, End::ScriptEntry::Red, true},
	// Page 25
	{"An Ordinary NSMB Mod"end, 25, 30, End::ScriptEntry::Red, true},
	// Page 26
	{"An Ordinary NSMB Mod"end, 26, 30, End::ScriptEntry::Red, true},
	// Page 27
	{"An Ordinary NSMB Mod"end, 27, 30, End::ScriptEntry::Red, true},
	// Page 28
	{"An Ordinary NSMB Mod"end, 28, 30, End::ScriptEntry::Red, true},
	// Page 29
	{"An Ordinary NSMB Mod"end, 29, 30, End::ScriptEntry::Red, true},
	// Page 30
	{"An Ordinary NSMB Mod"end, 30, 30, End::ScriptEntry::Red, true},
	// Page 31
	{"An Ordinary NSMB Mod"end, 31, 30, End::ScriptEntry::Red, true},
	// Page 32
	{"An Ordinary NSMB Mod"end, 32, 30, End::ScriptEntry::Red, true},
	// Page 33
	{"An Ordinary NSMB Mod"end, 33, 30, End::ScriptEntry::Red, true},
	// Page 34
	{"An Ordinary NSMB Mod"end, 34, 30, End::ScriptEntry::Red, true},
	// Page 35
	{"An Ordinary NSMB Mod"end, 35, 30, End::ScriptEntry::Red, true},
	// Page 36
	{"An Ordinary NSMB Mod"end, 36, 30, End::ScriptEntry::Red, false},
	End::ScriptTerminator, // Terminator
};

static_assert(NTR_ARRAY_SIZE(script) < 115, "Script size out of bounds");

