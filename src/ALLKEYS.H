// ALLKEYS.H - scancode names and scancode->character (reconstructed, ALLKEYS.H lines 104-268)
// The original is two big switch statements compiled to jump tables; the tables were decoded
// from the binary. Scancodes >= 0x80 are E0-prefixed (extended) keys, see key_interrupt().

const char *getkeyname(int keyn)
{
    switch (keyn) {
    case 1: return "ESCAPE";
    case 2: return "1"; case 3: return "2"; case 4: return "3"; case 5: return "4";
    case 6: return "5"; case 7: return "6"; case 8: return "7"; case 9: return "8";
    case 10: return "9"; case 11: return "0"; case 12: return "-"; case 13: return "=";
    case 14: return "BACKSPACE"; case 15: return "TAB";
    case 16: return "Q"; case 17: return "W"; case 18: return "E"; case 19: return "R";
    case 20: return "T"; case 21: return "Y"; case 22: return "U"; case 23: return "I";
    case 24: return "O"; case 25: return "P"; case 26: return "["; case 27: return "]";
    case 28: return "ENTER"; case 29: return "LEFT CONTROL";
    case 30: return "A"; case 31: return "S"; case 32: return "D"; case 33: return "F";
    case 34: return "G"; case 35: return "H"; case 36: return "J"; case 37: return "K";
    case 38: return "L"; case 39: return ";"; case 40: return "'"; case 41: return "`";
    case 42: return "LEFT SHIFT"; case 43: return "\\";
    case 44: return "Z"; case 45: return "X"; case 46: return "C"; case 47: return "V";
    case 48: return "B"; case 49: return "N"; case 50: return "M"; case 51: return ",";
    case 52: return "."; case 53: return "/"; case 54: return "RIGHT SHIFT";
    case 55: return "KEYPAD *"; case 56: return "LEFT ALT"; case 57: return "SPACEBAR";
    case 58: return "CAPS LOCK";
    case 59: return "F1"; case 60: return "F2"; case 61: return "F3"; case 62: return "F4";
    case 63: return "F5"; case 64: return "F6"; case 65: return "F7"; case 66: return "F8";
    case 67: return "F9"; case 68: return "F10";
    case 69: return "NUM LOCK"; case 70: return "SCROLL LOCK";
    case 71: return "KEYPAD 7"; case 72: return "KEYPAD 8"; case 73: return "KEYPAD 9";
    case 74: return "KEYPAD -"; case 75: return "KEYPAD 4"; case 76: return "KEYPAD 5";
    case 77: return "KEYPAD 6"; case 78: return "KEYPAD +"; case 79: return "KEYPAD 1";
    case 80: return "KEYPAD 2"; case 81: return "KEYPAD 3"; case 82: return "KEYPAD 0";
    case 83: return "KEYPAD DEL"; case 84: return "SYSREQ";
    case 87: return "F11"; case 88: return "F12";
    case 156: return "KEYPAD ENTER"; case 157: return "RIGHT CONTROL";
    case 181: return "KEYPAD /"; case 183: return "CTRL PRINT SCREEN"; case 184: return "RIGHT ALT";
    case 199: return "HOME"; case 200: return "UP ARROW"; case 201: return "PAGE UP";
    case 203: return "LEFT ARROW"; case 205: return "RIGHT ARROW";
    case 207: return "END"; case 208: return "DOWN ARROW"; case 209: return "PAGE DOWN";
    case 210: return "INSERT"; case 211: return "DELETE";
    default: {
        // Port: the original returned a pointer to this buffer on its own stack.
        static char stringn[16];
        itoa(keyn, stringn, 10);
        return stringn;
    }
    }
}

char skey2char(int keyn)
{
    static const char map[58] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0, 0,
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\'', '`', 0, '\\',
        'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, 0, 0, ' '};
    return keyn >= 2 && keyn <= 57 ? map[keyn] : 0;
}
