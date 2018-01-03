#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include "cs2cs.h"
#include "file.h"

/*
    EnumSystemCodePages & EnumCodePagesProc, + (1200 UTF-16LE, 1201 UTF-16BE)
    GetCPInfoEx
    https://en.wikipedia.org/wiki/Windows_code_page
    Most applications (Windows Kernel) written today handle character data primarily as Unicode, using the UTF-16 encoding. However, many legacy applications continue to use character sets based on code pages.
    http://www.iana.org/assignments/character-sets
    Use of UTF-32 strings on Windows (where wchar_t is 16 bits) is almost non-existent.
*/
// call: EnumSystemCodePages ( &EnumCodePagesProc, CP_SUPPORTED );
BOOL CALLBACK EnumCodePagesProc ( LPTSTR lcpstr )
{
    CPINFOEX cpx;
    if ( GetCPInfoEx ( (UINT)atoi(lcpstr), 0, &cpx ) ) {
        fprintf ( stderr, _T("%s\n"), cpx.CodePageName );
    }
    return TRUE;
}

void help(const char* app) {
    fprintf(stderr, "Charset to Charset, forcibly, by CodePages. Similar to iconv.\n@YX Hao #201708\n\n");
    fprintf(stderr, "Usage: %s <-f> <cp-in> <-t|-tb> <cp-out> [file-in [file-out]]\n-tb\tOutput with BOM, for UTF-16LE, UTF-16BE, UTF-8, GB18030 and UTF-7.\n", app);
    fprintf(stderr, "\nValid CodePages:\n");
    EnumSystemCodePages( &EnumCodePagesProc, CP_SUPPORTED );
    //EnumCodePagesProc("1200");
    //EnumCodePagesProc("1201");
    fprintf(stderr, "1200  (UTF-16LE)\n");
    fprintf(stderr, "1201  (UTF-16BE)\n");
    fprintf(stderr, "\nTips:\nNon-Printable Characters should be stored in files!\nRarely used Charsets are NOT supported: UTF-32LE, UTF-32BE, UTF-EBCDIC, UTF-1 ...\n");
    exit(22); //EINVAL
}

int main(int argc, const char** argv) {
    void *str_in, *str_out;
    size_t len_in = 0, len_out = 0, len_pre = 0;
    UINT CP_IN, CP_OUT;
    FILE *fp;
    //
    if (argc > 7 || argc < 5) { help(argv[0]); }
    //
    if (argc == 5) {
        fp = stdin;
    }
    else {
        fp = fopen(argv[5], "rb");
        if (!fp) { return errno; }
    }
    //
    len_in = read_file(fp, &str_in);
    if (fp != stdin) {fclose(fp);}
    //
    CP_IN = (UINT)atoi( argv[2] );
    CP_OUT = (UINT)atoi( argv[4] );
    //
    len_out = cs2cs(CP_IN, (char*)str_in + len_pre, CP_OUT, &str_out, stricmp(argv[3], "-tb") == 0);
    //
    if (argc == 7) {
        fp = fopen(argv[6], "wb");
        if (!fp) { return errno; }
    }
    else {
        fp = stdout;
    }
    //
    fwrite(str_out, 1, len_out, fp);
    if (fp != stdout) {fclose(fp);}
    //
    free(str_in);
    free(str_out);
    return len_out?0:errno;
}