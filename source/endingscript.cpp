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
	{"An Ordinary NSMB Mod"end, 0, 25, End::ScriptEntry::Red, true},
	{"Created By"end, 0, 57, End::ScriptEntry::Red, false},
	{"Ndymario"end, 0, 95, End::ScriptEntry::Blue, false},
	// Page 1
	{"An Ordinary NSMB Mod"end, 1, 25, End::ScriptEntry::Red, true},
	{"Programmers"end, 1, 57, End::ScriptEntry::Red, false},
	{"Ndymario"end, 1, 85, End::ScriptEntry::Blue, false},
	{"TheGameratorT"end, 1, 105, End::ScriptEntry::Blue, false},
	// Page 2
	{"An Ordinary NSMB Mod"end, 2, 25, End::ScriptEntry::Red, true},
	{"Graphics"end, 2, 57, End::ScriptEntry::Red, false},
	{"Slugmotif"end, 2, 95, End::ScriptEntry::Blue, false},
	// Page 3
	{"An Ordinary NSMB Mod"end, 3, 25, End::ScriptEntry::Red, true},
	{"Beta Testers"end, 3, 58, End::ScriptEntry::Red, true},
	{"Frosty Cake"end, 3, 87, End::ScriptEntry::Blue, false},
	{"Illy"end, 3, 105, End::ScriptEntry::Blue, false},
	{"Mosquito"end, 3, 123, End::ScriptEntry::Blue, false},
	{"Mr. Ztardust"end, 3, 141, End::ScriptEntry::Blue, false},
	// Page 4
	{"An Ordinary NSMB Mod"end, 4, 25, End::ScriptEntry::Red, true},
	{"Beta Testers"end, 4, 58, End::ScriptEntry::Red, false},
	{"SeanM"end, 4, 87, End::ScriptEntry::Blue, false},
	// Page 5
	{"An Ordinary NSMB Mod"end, 5, 25, End::ScriptEntry::Red, true},
	{"Special Thanks"end, 5, 58, End::ScriptEntry::Red, true},
	{"Will Smith - Lighting Code"end, 5, 87, End::ScriptEntry::Blue, false},
	{"Garhoogin - NitroPaint"end, 5, 105, End::ScriptEntry::Blue, false},
	{"TheGameratorT - Support"end, 5, 123, End::ScriptEntry::Blue, false},
	// Page 6
	{"An Ordinary NSMB Mod"end, 6, 25, End::ScriptEntry::Red, true},
	{"Special Thanks"end, 6, 58, End::ScriptEntry::Red, false},
	{"You - The Player"end, 6, 87, End::ScriptEntry::Blue, false},
	{"NSMB Central"end, 6, 105, End::ScriptEntry::Blue, false},
	// Page 7
	{"An Ordinary NSMB Mod"end, 7, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 7, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 7, 160, End::ScriptEntry::Blue, true},
	// Page 8
	{"An Ordinary NSMB Mod"end, 8, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 8, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 8, 160, End::ScriptEntry::Blue, true},
	// Page 9
	{"An Ordinary NSMB Mod"end, 9, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 9, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 9, 160, End::ScriptEntry::Blue, true},
	// Page 10
	{"An Ordinary NSMB Mod"end, 10, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 10, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 10, 160, End::ScriptEntry::Blue, true},
	// Page 11
	{"An Ordinary NSMB Mod"end, 11, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 11, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 11, 160, End::ScriptEntry::Blue, true},
	// Page 12
	{"An Ordinary NSMB Mod"end, 12, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 12, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 12, 160, End::ScriptEntry::Blue, true},
	// Page 13
	{"An Ordinary NSMB Mod"end, 13, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 13, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 13, 160, End::ScriptEntry::Blue, true},
	// Page 14
	{"An Ordinary NSMB Mod"end, 14, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 14, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 14, 160, End::ScriptEntry::Blue, true},
	// Page 15
	{"An Ordinary NSMB Mod"end, 15, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 15, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 15, 160, End::ScriptEntry::Blue, true},
	// Page 16
	{"An Ordinary NSMB Mod"end, 16, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 16, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 16, 160, End::ScriptEntry::Blue, true},
	// Page 17
	{"An Ordinary NSMB Mod"end, 17, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 17, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 17, 160, End::ScriptEntry::Blue, true},
	// Page 18
	{"An Ordinary NSMB Mod"end, 18, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 18, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 18, 160, End::ScriptEntry::Blue, true},
	// Page 19
	{"An Ordinary NSMB Mod"end, 19, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 19, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 19, 160, End::ScriptEntry::Blue, true},
	// Page 20
	{"An Ordinary NSMB Mod"end, 20, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 20, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 20, 160, End::ScriptEntry::Blue, true},
	// Page 21
	{"An Ordinary NSMB Mod"end, 21, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 21, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 21, 160, End::ScriptEntry::Blue, true},
	// Page 22
	{"An Ordinary NSMB Mod"end, 22, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 22, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 22, 160, End::ScriptEntry::Blue, true},
	// Page 23
	{"An Ordinary NSMB Mod"end, 23, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 23, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 23, 160, End::ScriptEntry::Blue, true},
	// Page 24
	{"An Ordinary NSMB Mod"end, 24, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 24, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 24, 160, End::ScriptEntry::Blue, true},
	// Page 25
	{"An Ordinary NSMB Mod"end, 25, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 25, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 25, 160, End::ScriptEntry::Blue, true},
	// Page 26
	{"An Ordinary NSMB Mod"end, 26, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 26, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 26, 160, End::ScriptEntry::Blue, true},
	// Page 27
	{"An Ordinary NSMB Mod"end, 27, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 27, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 27, 160, End::ScriptEntry::Blue, true},
	// Page 28
	{"An Ordinary NSMB Mod"end, 28, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 28, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 28, 160, End::ScriptEntry::Blue, true},
	// Page 29
	{"An Ordinary NSMB Mod"end, 29, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 29, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 29, 160, End::ScriptEntry::Blue, true},
	// Page 30
	{"An Ordinary NSMB Mod"end, 30, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 30, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 30, 160, End::ScriptEntry::Blue, true},
	// Page 31
	{"An Ordinary NSMB Mod"end, 31, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 31, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 31, 160, End::ScriptEntry::Blue, true},
	// Page 32
	{"An Ordinary NSMB Mod"end, 32, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 32, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 32, 160, End::ScriptEntry::Blue, true},
	// Page 33
	{"An Ordinary NSMB Mod"end, 33, 25, End::ScriptEntry::Red, true},
	{"Thanks for playing"end, 33, 95, End::ScriptEntry::Blue, true},
	{"Join the Central Discord"end, 33, 160, End::ScriptEntry::Blue, true},
	// Page 34
	{"An Ordinary NSMB Mod"end, 34, 25, End::ScriptEntry::Red, false},
	{"Thanks for playing"end, 34, 95, End::ScriptEntry::Blue, false},
	{"Join the Central Discord"end, 34, 160, End::ScriptEntry::Blue, false},
	End::ScriptTerminator, // Terminator
};

static_assert(NTR_ARRAY_SIZE(script) < 115, "Script size out of bounds");

