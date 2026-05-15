#define INITGUID

#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <stdio.h>
#include <string.h>

#define DOSBOX_PATH  "C:\\WINDOWS\\SYSTEM32\\DOSBox.exe"
#define DEFAULT_CONF "C:\\WINDOWS\\SYSTEM32\\dosbox.conf"
#define CONF_DIR     "C:\\WINDOWS\\SYSTEM32\\CONF\\"

#define TYPE_DOS    0
#define TYPE_WIN32  1


/*=====================================================*/
/* CRC16 */
/*=====================================================*/

unsigned short crc16_string(const char *s)
{
    unsigned short crc=0xFFFF;

    while(*s)
    {
        crc^=(unsigned char)*s++;

        for(int i=0;i<8;i++)
        {
            if(crc&1)
                crc=(crc>>1)^0xA001;
            else
                crc>>=1;
        }
    }

    return crc;
}


/*=====================================================*/
/* .COM => DOS sempre */
/*=====================================================*/

int is_com(const char *path)
{
    const char *e=strrchr(path,'.');

    if(!e)
        return 0;

    return lstrcmpiA(e,".com")==0;
}


/*=====================================================*/
/* detector refinado */
/*=====================================================*/

int exe_type(const char *path)
{
    FILE *f=fopen(path,"rb");

    if(!f)
        return TYPE_DOS;

    unsigned short mz;

    if(fread(&mz,2,1,f)!=1)
    {
        fclose(f);
        return TYPE_DOS;
    }

    if(mz!=0x5A4D)
    {
        fclose(f);
        return TYPE_DOS;
    }

    fseek(f,0,SEEK_END);

    long size=ftell(f);

    unsigned int peofs=0;

    fseek(f,0x3C,SEEK_SET);

    fread(&peofs,4,1,f);

    if(peofs<64 || peofs>(size-4))
    {
        fclose(f);
        return TYPE_DOS;
    }

    fseek(f,peofs,SEEK_SET);

    unsigned char sig[4]={0};

    fread(sig,1,4,f);

    fclose(f);

    if(
        sig[0]=='P' &&
        sig[1]=='E' &&
        sig[2]==0 &&
        sig[3]==0
    )
        return TYPE_WIN32;

    return TYPE_DOS;
}


/*=====================================================*/
/* pega EXE real + preserva args */
/*=====================================================*/

void get_real_exe(char *out)
{
    char *cmd=GetCommandLineA();

    if(*cmd=='"')
    {
        cmd++;

        char *q=strchr(cmd,'"');

        if(!q)
        {
            out[0]=0;
            return;
        }

        cmd=q+1;
    }
    else
    {
        char *sp=strchr(cmd,' ');

        if(!sp)
        {
            out[0]=0;
            return;
        }

        cmd=sp+1;
    }

    while(*cmd==' ')
        cmd++;

    if(*cmd=='"')
    {
        cmd++;

        char *q=strchr(cmd,'"');

        if(!q)
        {
            out[0]=0;
            return;
        }

        memcpy(out,cmd,q-cmd);

        out[q-cmd]=0;
    }
    else
    {
        char *sp=strchr(cmd,' ');

        if(sp)
        {
            memcpy(out,cmd,sp-cmd);

            out[sp-cmd]=0;
        }
        else
            lstrcpyA(out,cmd);
    }
}


/*=====================================================*/
/* cria config.conf.lnk */
/*=====================================================*/

void create_shortcut(
    const char *target,
    const char *shortcut
)
{
    CoInitialize(NULL);

    IShellLink *psl;

    if(SUCCEEDED(
        CoCreateInstance(
            &CLSID_ShellLink,
            NULL,
            CLSCTX_INPROC_SERVER,
            &IID_IShellLink,
            (LPVOID*)&psl
        )))
    {
        psl->lpVtbl->SetPath(
            psl,
            target
        );

        IPersistFile *ppf;

        if(SUCCEEDED(
            psl->lpVtbl->QueryInterface(
                psl,
                &IID_IPersistFile,
                (LPVOID*)&ppf
            )))
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
}



/*=====================================================*/
/* MAIN */
/*=====================================================*/

int WINAPI WinMain(
HINSTANCE a,
HINSTANCE b,
LPSTR c,
int d
)
{
    char exe[MAX_PATH];

    get_real_exe(exe);

    if(!exe[0])
        return 0;


    char self[MAX_PATH];

    GetModuleFileNameA(
        NULL,
        self,
        MAX_PATH
    );

    if(
        lstrcmpiA(
            exe,
            self
        )==0
    )
        return 0;


    /*========================================*/
    /* WIN32 */
    /*========================================*/

    if(
        !is_com(exe) &&
        exe_type(exe)==TYPE_WIN32
    )
    {
        STARTUPINFOA si={0};
        PROCESS_INFORMATION pi={0};

        si.cb=sizeof(si);

        CreateProcessA(
            exe,
            c,
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



    /*========================================*/
    /* DOS */
    /*========================================*/

    char dir[MAX_PATH];

    lstrcpyA(dir,exe);

    char *slash=
        strrchr(dir,'\\');

    if(!slash)
        return 0;


    char name[MAX_PATH];

    lstrcpyA(
        name,
        slash+1
    );

    *slash=0;


    char folder[MAX_PATH];

    char *last=
        strrchr(
            dir,
            '\\'
        );

    if(last)
        lstrcpyA(
            folder,
            last+1
        );
    else
        lstrcpyA(
            folder,
            dir
        );


    unsigned short crc=
        crc16_string(dir);


    char conf[MAX_PATH];

    wsprintfA(
        conf,
        "%s%s_%04X.conf",
        CONF_DIR,
        folder,
        crc
    );


    if(
        GetFileAttributesA(conf)
        ==
        INVALID_FILE_ATTRIBUTES
    )
    {
        CopyFileA(
            DEFAULT_CONF,
            conf,
            TRUE
        );
    }


    char shortcut[MAX_PATH];

    wsprintfA(
        shortcut,
        "%s\\config.conf.lnk",
        dir
    );


    if(
        GetFileAttributesA(shortcut)
        ==
        INVALID_FILE_ATTRIBUTES
    )
    {
        create_shortcut(
            conf,
            shortcut
        );
    }


    SetEnvironmentVariableA(
        "SDL_STDIO_REDIRECT",
        "0"
    );


    char cmd[4096];

    wsprintfA(
        cmd,

        "\"%s\" "
        "-conf \"%s\" "
        "-noconsole "
        "-exit "
		"-c \"echo off\" "
		"-c \"cls\" "
        "-c \"mount c '%s'\" "
        "-c \"c:\" "
		"-c \"cls\" "
        "-c \"%s\" "
        "-c \"exit\"",

        DOSBOX_PATH,
        conf,
        dir,
        name
    );


    STARTUPINFOA si={0};
    PROCESS_INFORMATION pi={0};

    si.cb=sizeof(si);

    CreateProcessA(
        NULL,
        cmd,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        dir,
        &si,
        &pi
    );

    return 0;
}