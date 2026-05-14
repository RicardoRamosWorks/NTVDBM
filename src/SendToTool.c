#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>

#pragma comment(lib, "ole32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")

#define ICON_DIR      "C:\\WINDOWS\\SYSTEM32\\gameicons\\"
///#define DEFAULT_ICON  "C:\\WINDOWS\\SYSTEM32\\gameicons\\default.ico"

/* ===================================================== */
/* CRC16 */
/* ===================================================== */

unsigned short crc16_file(const char *path)
{
    FILE *f = fopen(path, "rb");

    if (!f)
        return 0;

    unsigned short crc = 0xFFFF;

    unsigned char buffer[4096];

    size_t read;

    while ((read = fread(buffer, 1, sizeof(buffer), f)) > 0)
    {
        for (size_t i = 0; i < read; i++)
        {
            crc ^= buffer[i];

            for (int j = 0; j < 8; j++)
            {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xA001;
                else
                    crc >>= 1;
            }
        }
    }

    fclose(f);

    return crc;
}

/* ===================================================== */
/* DETECTA PE32 */
/* ===================================================== */

int is_pe32(const char *path)
{
    FILE *f = fopen(path, "rb");

    if (!f)
        return 1;

    unsigned char mz[2];

    fread(mz, 1, 2, f);

    if (mz[0] != 'M' || mz[1] != 'Z')
    {
        fclose(f);
        return 1;
    }

    fseek(f, 0x3C, SEEK_SET);

    unsigned int pe_offset;

    fread(&pe_offset, 4, 1, f);

    fseek(f, pe_offset, SEEK_SET);

    unsigned char pe[2];

    fread(pe, 1, 2, f);

    fclose(f);

    if (pe[0] == 'P' && pe[1] == 'E')
        return 1;

    return 0;
}

/* ===================================================== */
/* POSSUI ÍCONE? */
/* ===================================================== */

int has_icon(const char *path)
{
    HICON hLarge = NULL;
    HICON hSmall = NULL;

    UINT count = ExtractIconExA(
        path,
        0,
        &hLarge,
        &hSmall,
        1
    );

    if (hLarge)
        DestroyIcon(hLarge);

    if (hSmall)
        DestroyIcon(hSmall);

    return (count > 0);
}

/* ===================================================== */
/* GERA ÍCONE CRC */
/* ===================================================== */

void build_icon_path(
    const char *target,
    char *iconpath
)
{
    char name[MAX_PATH];

    lstrcpy(name, PathFindFileNameA(target));

    PathRemoveExtensionA(name);

    unsigned short crc = crc16_file(target);

    wsprintfA(
        iconpath,
        "%s%s_%04X.ico",
        ICON_DIR,
        name,
        crc
    );

    /* já existe? usa */
    if (GetFileAttributesA(iconpath) != INVALID_FILE_ATTRIBUTES)
        return;

    /* escolhe template */

    char source[MAX_PATH];

    if (is_pe32(target))
    {
        lstrcpy(
            source,
            "C:\\WINDOWS\\SYSTEM32\\gameicons\\default32.ico"
        );
    }
    else
    {
        lstrcpy(
            source,
            "C:\\WINDOWS\\SYSTEM32\\gameicons\\default16.ico"
        );
    }

    CopyFileA(
        source,
        iconpath,
        FALSE
    );
}

/* ===================================================== */
/* NOME ÚNICO */
/* ===================================================== */

void build_unique_shortcut(
    const char *desktop,
    const char *name,
    char *out
)
{
    wsprintfA(out, "%s\\%s.lnk", desktop, name);

    if (GetFileAttributesA(out) == INVALID_FILE_ATTRIBUTES)
        return;

    for (int i = 2; i < 1000; i++)
    {
        wsprintfA(
            out,
            "%s\\%s (%d).lnk",
            desktop,
            name,
            i
        );

        if (GetFileAttributesA(out) == INVALID_FILE_ATTRIBUTES)
            return;
    }
}

/* ===================================================== */
/* CRIA BAT */
/* ===================================================== */

void create_bat(
    const char *gamepath,
    char *batpath
)
{
    char dir[MAX_PATH];
    char exe[MAX_PATH];

    lstrcpy(dir, gamepath);

    char *slash = strrchr(dir, '\\');

    if (!slash)
        return;

    lstrcpy(exe, slash + 1);

    *slash = 0;

    char name[MAX_PATH];

    lstrcpy(name, exe);

    PathRemoveExtensionA(name);

    wsprintfA(
        batpath,
        "%s\\%s.bat",
        dir,
        name
    );

    FILE *f = fopen(batpath, "w");

if (!f)
    return;

fprintf(f, "@echo off\n");
fprintf(f, "cd /d \"%%~dp0\"\n");
fprintf(f, "start \"\" /min \"%s\"\n", exe);
fprintf(f, "exit\n");

    fclose(f);
}

/* ===================================================== */
/* MAIN */
/* ===================================================== */

int WINAPI WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR lpCmdLine,
    int nShow
)
{
    if (!lpCmdLine || !lpCmdLine[0])
        return 0;

    char target[MAX_PATH];

    lstrcpy(target, lpCmdLine);

    /* remove aspas */

    if (target[0] == '"')
    {
        memmove(target, target + 1, strlen(target));

        char *q = strrchr(target, '"');

        if (q)
            *q = 0;
    }

    char full[MAX_PATH];

    GetFullPathNameA(
        target,
        MAX_PATH,
        full,
        NULL
    );

    /* =====================================================
       desktop
    ===================================================== */

    char desktop[MAX_PATH];

    SHGetFolderPathA(
        NULL,
        CSIDL_DESKTOPDIRECTORY,
        NULL,
        0,
        desktop
    );

    /* =====================================================
       nome
    ===================================================== */

    char name[MAX_PATH];

    lstrcpy(name, PathFindFileNameA(full));

    PathRemoveExtensionA(name);

    /* =====================================================
       shortcut path
    ===================================================== */

    char shortcut[MAX_PATH];

    build_unique_shortcut(
        desktop,
        name,
        shortcut
    );

    /* =====================================================
       working dir
    ===================================================== */

    char workdir[MAX_PATH];

    lstrcpy(workdir, full);

    PathRemoveFileSpecA(workdir);

    /* =====================================================
       destino do atalho
    ===================================================== */

    char final_target[MAX_PATH];

    lstrcpy(final_target, full);

    /* =====================================================
       16-BIT => cria BAT
    ===================================================== */

    if (!is_pe32(full))
    {
        create_bat(
            full,
            final_target
        );
    }

    /* =====================================================
       COM
    ===================================================== */

    CoInitialize(NULL);

    IShellLinkA *psl;

    HRESULT hr = CoCreateInstance(
        &CLSID_ShellLink,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IShellLinkA,
        (LPVOID*)&psl
    );

    if (SUCCEEDED(hr))
    {
        psl->lpVtbl->SetPath(
            psl,
            final_target
        );

        psl->lpVtbl->SetWorkingDirectory(
            psl,
            workdir
        );

        /* =================================================
           ÍCONE
        ================================================= */

        if (is_pe32(full) && has_icon(full))
        {
            /* usa ícone do próprio EXE */

            psl->lpVtbl->SetIconLocation(
                psl,
                full,
                0
            );
        }
        else
        {
            /* usa gameicons */

            char iconpath[MAX_PATH];

            build_icon_path(
                full,
                iconpath
            );

            psl->lpVtbl->SetIconLocation(
                psl,
                iconpath,
                0
            );
        }

        /* =================================================
           salvar
        ================================================= */

        IPersistFile *ppf;

        hr = psl->lpVtbl->QueryInterface(
            psl,
            &IID_IPersistFile,
            (LPVOID*)&ppf
        );

        if (SUCCEEDED(hr))
        {
            WCHAR wsz[MAX_PATH];

            MultiByteToWideChar(
                CP_ACP,
                0,
                shortcut,
                -1,
                wsz,
                MAX_PATH
            );

            ppf->lpVtbl->Save(
                ppf,
                wsz,
                TRUE
            );

            ppf->lpVtbl->Release(ppf);
        }

        psl->lpVtbl->Release(psl);
    }

    CoUninitialize();

    return 0;
}