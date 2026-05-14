#define INITGUID
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <stdio.h>
#include <string.h>

#define DOSBOX_PATH "C:\\WINDOWS\\SYSTEM32\\DOSBox.exe"
#define DEFAULT_CONF "C:\\WINDOWS\\SYSTEM32\\dosbox.conf"
#define CONF_DIR "C:\\WINDOWS\\SYSTEM32\\CONF\\"

unsigned short crc16_string(const char *s)
{
    unsigned short crc = 0xFFFF;

    while (*s)
    {
        crc ^= (unsigned char)*s++;

        for (int i = 0; i < 8; i++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    return crc;
}

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

void create_shortcut(const char *target, const char *shortcut)
{
    HRESULT hres;

    hres = CoInitialize(NULL);

    if (SUCCEEDED(hres))
    {
        IShellLink *psl;

        hres = CoCreateInstance(
            &CLSID_ShellLink,
            NULL,
            CLSCTX_INPROC_SERVER,
            &IID_IShellLink,
            (LPVOID*)&psl
        );

        if (SUCCEEDED(hres))
        {
            IPersistFile *ppf;

            psl->lpVtbl->SetPath(psl, target);

            hres = psl->lpVtbl->QueryInterface(
                psl,
                &IID_IPersistFile,
                (LPVOID*)&ppf
            );

            if (SUCCEEDED(hres))
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

                ppf->lpVtbl->Save(ppf, wsz, TRUE);

                ppf->lpVtbl->Release(ppf);
            }

            psl->lpVtbl->Release(psl);
        }

        CoUninitialize();
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nShow)
{
    char exe[MAX_PATH];

    /* =====================================================
       FIX:
       compatível com atalhos .lnk modernos
    ===================================================== */

    lstrcpy(exe, GetCommandLineA());

    char *p = exe;

    /* pula nome do HDL */
    if (*p == '"')
    {
        p++;

        p = strchr(p, '"');

        if (!p)
            return 0;

        p++;
    }
    else
    {
        p = strchr(p, ' ');

        if (!p)
            return 0;

        p++;
    }

    while (*p == ' ')
        p++;

    lstrcpy(exe, p);

    /* remove aspas */
    if (exe[0] == '"')
    {
        memmove(exe, exe + 1, strlen(exe));

        char *last = strrchr(exe, '"');

        if (last)
            *last = 0;
    }

    if (strlen(exe) == 0)
        return 0;

    /* evita loop */
    char self[MAX_PATH];

    GetModuleFileName(NULL, self, MAX_PATH);

    if (lstrcmpi(exe, self) == 0)
        return 0;

    /* =====================================================
       EXECUTÁVEL WIN32 NORMAL
    ===================================================== */

   if (is_pe32(exe))
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);

    /* pega linha original completa */
    char cmdline[4096];

    lstrcpy(cmdline, GetCommandLineA());

    /* pula nome do HDL */
    char *p = cmdline;

    if (*p == '"')
    {
        p++;

        p = strchr(p, '"');

        if (!p)
            return 0;

        p++;
    }
    else
    {
        p = strchr(p, ' ');

        if (!p)
            return 0;

        p++;
    }

    while (*p == ' ')
        p++;

    /* executa exatamente como veio */
    if (!CreateProcessA(
        NULL,
        p,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    ))
    {
        MessageBoxA(
            NULL,
            p,
            "CreateProcess Failed",
            MB_OK
        );
    }

    return 0;
}

    /* =====================================================
       DOS / 16-BIT
    ===================================================== */

    char dir[MAX_PATH];
    char name[MAX_PATH];
    char folder[MAX_PATH];

    lstrcpy(dir, exe);

    char *slash = strrchr(dir, '\\');

    if (!slash)
        return 0;

    lstrcpy(name, slash + 1);

    *slash = 0;

    char *lastslash = strrchr(dir, '\\');

    if (lastslash)
        lstrcpy(folder, lastslash + 1);
    else
        lstrcpy(folder, dir);

    unsigned short crc = crc16_string(dir);

    char confpath[MAX_PATH];

    wsprintf(
        confpath,
        "%s%s_%04X.conf",
        CONF_DIR,
        folder,
        crc
    );

    DWORD attrs = GetFileAttributes(confpath);

    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        CopyFile(
            DEFAULT_CONF,
            confpath,
            TRUE
        );
    }

    /* =====================================================
       cria config.conf.lnk
    ===================================================== */

    char shortcut[MAX_PATH];

    wsprintf(
        shortcut,
        "%s\\config.conf.lnk",
        dir
    );

    if (GetFileAttributes(shortcut) == INVALID_FILE_ATTRIBUTES)
    {
        create_shortcut(
            confpath,
            shortcut
        );
    }

    /* =====================================================
       COMANDO DOSBOX
    ===================================================== */

    char cmd[4096];

    wsprintf(
        cmd,
        "\"%s\" "
        "-conf \"%s\" "
        "-noconsole "
        "-exit "
        "-c \"mount c '%s'\" "
        "-c \"c:\" "
        "-c \"call %s\" "
		"-c \"exit\"",
        DOSBOX_PATH,
        confpath,
        dir,
        name
    );

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);

    CreateProcess(
        NULL,
        cmd,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    return 0;
}