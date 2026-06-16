#pragma once

#define ANSI_RESET "\033[0m"

// Text colors
#define ANSI_BLACK   "\033[30m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"

// Bright text colors
#define ANSI_GRAY       "\033[90m"
#define ANSI_BR_RED     "\033[91m"
#define ANSI_BR_GREEN   "\033[92m"
#define ANSI_BR_YELLOW  "\033[93m"
#define ANSI_BR_BLUE    "\033[94m"
#define ANSI_BR_MAGENTA "\033[95m"
#define ANSI_BR_CYAN    "\033[96m"
#define ANSI_BR_WHITE   "\033[97m"

// Background colors
#define ANSI_BG_BLACK   "\033[40m"
#define ANSI_BG_RED     "\033[41m"
#define ANSI_BG_GREEN   "\033[42m"
#define ANSI_BG_YELLOW  "\033[43m"
#define ANSI_BG_BLUE    "\033[44m"
#define ANSI_BG_MAGENTA "\033[45m"
#define ANSI_BG_CYAN    "\033[46m"
#define ANSI_BG_WHITE   "\033[47m"

// Bright background colors
#define ANSI_BG_GRAY       "\033[100m"
#define ANSI_BG_BR_RED     "\033[101m"
#define ANSI_BG_BR_GREEN   "\033[102m"
#define ANSI_BG_BR_YELLOW  "\033[103m"
#define ANSI_BG_BR_BLUE    "\033[104m"
#define ANSI_BG_BR_MAGENTA "\033[105m"
#define ANSI_BG_BR_CYAN    "\033[106m"
#define ANSI_BG_BR_WHITE   "\033[107m"

// Text styles
#define ANSI_BOLD      "\033[1m"
#define ANSI_DIM       "\033[2m"
#define ANSI_ITALIC    "\033[3m"
#define ANSI_UNDERLINE "\033[4m"
#define ANSI_BLINK     "\033[5m"
#define ANSI_INVERSE   "\033[7m"
#define ANSI_STRIKE    "\033[9m"


// 256-color (xterm palette)
#define ANSI_FG256(n) "\033[38;5;" #n "m"
#define ANSI_BG256(n) "\033[48;5;" #n "m"

// Truecolor RGB
#define ANSI_FG_RGB(r, g, b) "\033[38;2;" #r ";" #g ";" #b "m"
#define ANSI_BG_RGB(r, g, b) "\033[48;2;" #r ";" #g ";" #b "m"




#define ANSI_CLEAR "\x1b[0m"
#define ANSI_CLS   "\x1b[2J"
#define ANSI_HOME  "\x1b[H"

#define ANSI_SAVE_CURSOR        "\x1b[s"
#define ANSI_RESTORE_CURSOR_POS "\x1b[u"

#define ANSI_MOVE_CURSOR_RIGHT(n) "\x1b[" #n "C"
#define ANSI_MOVE_CURSOR_LEFT(n)  "\x1b[" #n "D"

#define ANSI_ERASE_FROM_CURSOR_TO_END_OF_SCREEN "\x1b[0J"
#define ANSI_ERASE_LINE                         "\x1b[K"
