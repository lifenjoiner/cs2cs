/*
  OEM <=> UTF-16 (UTF-16LE) <=> UTF-8
  https://msdn.microsoft.com/en-us/library/windows/desktop/dd374081
  https://msdn.microsoft.com/library/windows/desktop/dd319072
  MultiByteToWideChar --> WideCharToMultiByte
  https://msdn.microsoft.com/library/windows/desktop/dd374130
  Unicode
  https://en.wikipedia.org/wiki/Plane_(Unicode)
  https://en.wikipedia.org/wiki/Unicode_block
  //
  '_mbstowcs_l --> _wcstombs_l' are not avaliable even on win 7!
*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

void *U16LE_BOM = "\xFF\xFE";
void *U16BE_BOM = "\xFE\xFF";
//void *U32LE_BOM = "\xFF\xFE\x00\x00";
//void *U32BE_BOM = "\x00\x00\xFE\xFF";
void *GB18030_BOM = "\x84\x31\x95\x33";
void *U8_BOM = "\xEF\xBB\xBF";

////////////////////////////////////////////////////////////////////////////////
//winnls.h
int cs_to_utf16(UINT cp_in, const char *str_in, wchar_t **str_w) {
    int len_w;
    //
    len_w = MultiByteToWideChar(cp_in, 0, str_in, -1, NULL, 0);
    if (len_w == 0) {return 0;}
    *str_w = calloc(len_w, sizeof(wchar_t));
    len_w = MultiByteToWideChar(cp_in, 0, str_in, -1, *str_w, len_w);
    return len_w - 1;
}

int utf16_to_cs(const wchar_t *str_w, UINT cp_out, char **str_m) {
    const wchar_t *str_wn;
    int len_m;
    //
    str_wn = (cp_out != 65000 && memcmp(str_w, U16LE_BOM, 2) == 0) ? str_w + 1 : str_w;
    len_m = WideCharToMultiByte(cp_out, 0, str_wn, -1, NULL, 0, NULL, NULL);
    if (len_m == 0) {return 0;}
    *str_m = calloc(len_m, sizeof(char));
    len_m = WideCharToMultiByte(cp_out, 0, str_wn, -1, *str_m, len_m, NULL, NULL);
    return len_m - 1;
}

int process_BOM(void **str, int len_str, int len_null, void *BOM, int len_BOM) {
    int len_new_t;
    len_new_t = len_str + len_null + len_BOM;
    if (len_BOM >= 0) {
        *str = realloc(*str, len_new_t);
        if (*str == 0) fprintf(stderr, "0x%X\n", errno);
        memmove(*str + len_BOM, *str, len_str + len_null);
        memcpy(*str, BOM, len_BOM);
    }
    else {
        memmove(*str, *str - len_BOM, len_new_t);
        *str = realloc(*str, len_new_t);
    }
    return len_str + len_BOM;
}

/*not utf16*/
int cs2cs(UINT cp_in, void *str_in, UINT cp_out, void **str_out, int BOM_out) {
    wchar_t *str_w;
    int len_r, len_pre = 0;
    //
    //https://en.wikipedia.org/wiki/Byte_order_mark
    // BOM in
    switch (cp_in) {
    case 1200: //UTF-16LE
        if (memcmp(str_in, U16LE_BOM, 2) == 0) { len_pre = 2; }
        len_r = wcslen(str_in) * 2 - len_pre;
        //
        if (cp_out == cp_in) {
            *str_out = str_in + len_pre;
        }
        else if (cp_out == 1201) {
            *str_out = malloc(len_r + 2);
            _swab(str_in + len_pre, *str_out, len_r);
        }
        else {
            len_r = utf16_to_cs(
            (wchar_t*)((BOM_out && cp_out == 65000) ? str_in : (char*)str_in + len_pre)
            , cp_out, (char**)str_out);
        }
        break;
    case 1201: //UTF-16BE
        if (memcmp(str_in, U16BE_BOM, 2) == 0) { len_pre = 2; }
        len_r = wcslen(str_in) * 2 - len_pre;
        //
        if (cp_out == cp_in) {
            *str_out = str_in + len_pre;
        }
        else if (cp_out == 1200) {
            *str_out = malloc(len_r + 2);
            _swab(str_in + len_pre, *str_out, len_r);
        }
        else {
            str_w = str_in;
            _swab(str_in, (void*)str_w, len_r + len_pre); // overwrite
            len_r = utf16_to_cs(
            (wchar_t*)((BOM_out && cp_out == 65000) ? str_w : (char*)str_w + len_pre)
            , cp_out, (char**)str_out);
        }
        break;
    /*
    case 12000: //UTF-32LE
        break;
    case 12001: //UTF-32BE
        break;
    */
    default: // NOT WideChar
        //GB18030
        if (cp_in == 54936 && memcmp(str_in, GB18030_BOM , 4) == 0) { len_pre = 4; }
        //UTF-8
        if (cp_in == 65001 && memcmp(str_in, U8_BOM, 3) == 0) { len_pre = 3; }
        // UTF-7 BOM is from UTF-16 BOM and following char
        len_r = strlen(str_in) - len_pre;
        //
        // if (cp_out == cp_in) // BOM: in or out
        if (cp_out == 1200) {
            len_r = cs_to_utf16(cp_in, (char*)str_in + len_pre, (wchar_t**)str_out) * 2;
            len_r = process_BOM(str_out, len_r, 2, U16LE_BOM, (memcmp(*str_out, U16LE_BOM, 2) == 0)?-2:0);
        }
        else if (cp_out == 1201) {
            len_r = cs_to_utf16(cp_in, (char*)str_in + len_pre, &str_w) * 2;
            len_r = process_BOM((void**)&str_w, len_r, 2, U16LE_BOM, (memcmp(str_w, U16LE_BOM, 2) == 0)?-2:0);
            *str_out = malloc(len_r + 2);
            _swab((char*)str_w, (char*)*str_out, len_r + 2); // null needed
        }
        else {
            len_r = cs_to_utf16(cp_in, str_in + len_pre, &str_w);
            if (len_r == 0) {return 0;}
            if (BOM_out && cp_out == 65000) {
                len_r = process_BOM((void*)&str_w, len_r, 2, U16LE_BOM, 2);
            }
            len_r = utf16_to_cs(str_w, cp_out, (char**)str_out);
        }
    }
    // BOM out
    switch (cp_out) {
    case 1200: //UTF-16LE
        len_r = process_BOM(str_out, len_r, 2, U16LE_BOM, BOM_out?2:0);
        break;
    case 1201: //UTF-16BE
        len_r = process_BOM(str_out, len_r, 2, U16BE_BOM, BOM_out?2:0);
        break;
    /*
    case 12000: //UTF-32LE
        break;
    case 12001: //UTF-32BE
        break;
    */
    case 54936: //GB18030
        len_r = process_BOM(str_out, len_r, 1, GB18030_BOM , BOM_out?4:0);
        break;
    case 65001: //UTF-8
        len_r = process_BOM(str_out, len_r, 1, U8_BOM, BOM_out?3:0);
        break;
    default:
        len_r = process_BOM(str_out, len_r, 1, "", 0);
    }
    //
    free(str_w);
    return len_r;
}
