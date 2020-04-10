#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "dynamic_libs/gx2_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "dynamic_libs/vpad_functions.h"
#include "dynamic_libs/padscore_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "dynamic_libs/ax_functions.h"
#include "fs/fs_utils.h"
#include "fs/sd_fat_devoptab.h"
#include <iosuhax_devoptab.h>
#include <iosuhax.h>
#include "system/memory.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "common/common.h"
#include "sd_dumper.h"

#define MAX_CONSOLE_LINES_TV    27
#define MAX_CONSOLE_LINES_DRC   18

static char * consoleArrayTv[MAX_CONSOLE_LINES_TV];
static char * consoleArrayDrc[MAX_CONSOLE_LINES_DRC];

static void console_print_pos(int x, int y, const char *format, ...)
{
	char * tmp = NULL;

	va_list va;
	va_start(va, format);
	if((vasprintf(&tmp, format, va) >= 0) && tmp)
	{
        if(strlen(tmp) > 79)
            tmp[79] = 0;

        OSScreenPutFontEx(0, x, y, tmp);
        OSScreenPutFontEx(1, x, y, tmp);

	}
	va_end(va);

	if(tmp)
        free(tmp);
}

void console_printf(int newline, const char *format, ...)
{
	char * tmp = NULL;

	va_list va;
	va_start(va, format);
	if((vasprintf(&tmp, format, va) >= 0) && tmp)
	{
	    if(newline)
        {
            if(consoleArrayTv[0])
                free(consoleArrayTv[0]);
            if(consoleArrayDrc[0])
                free(consoleArrayDrc[0]);

            for(int i = 1; i < MAX_CONSOLE_LINES_TV; i++)
                consoleArrayTv[i-1] = consoleArrayTv[i];

            for(int i = 1; i < MAX_CONSOLE_LINES_DRC; i++)
                consoleArrayDrc[i-1] = consoleArrayDrc[i];
        }
        else
        {
            if(consoleArrayTv[MAX_CONSOLE_LINES_TV-1])
                free(consoleArrayTv[MAX_CONSOLE_LINES_TV-1]);
            if(consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1])
                free(consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1]);

            consoleArrayTv[MAX_CONSOLE_LINES_TV-1] = NULL;
            consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1] = NULL;
        }

        if(strlen(tmp) > 79)
            tmp[79] = 0;

        consoleArrayTv[MAX_CONSOLE_LINES_TV-1] = (char*)malloc(strlen(tmp) + 1);
        if(consoleArrayTv[MAX_CONSOLE_LINES_TV-1])
            strcpy(consoleArrayTv[MAX_CONSOLE_LINES_TV-1], tmp);

        consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1] = (tmp);
	}
	va_end(va);

    // Clear screens
    OSScreenClearBufferEx(0, 0);
    OSScreenClearBufferEx(1, 0);


	for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
    {
        if(consoleArrayTv[i])
            OSScreenPutFontEx(0, 0, i, consoleArrayTv[i]);
    }

	for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
    {
        if(consoleArrayDrc[i])
            OSScreenPutFontEx(1, 0, i, consoleArrayDrc[i]);
    }

	OSScreenFlipBuffersEx(0);
	OSScreenFlipBuffersEx(1);
}

int CheckCancel(void)
{
    int vpadError = -1;
    VPADData vpad;

    //! update only at 50 Hz, thats more than enough
    VPADRead(0, &vpad, 1, &vpadError);

    if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_B))
    {
        return 1;
    }
    return 0;
}

/* Entry point */
int Menu_Main(void)
{
    //!*******************************************************************
    //!                   Initialize function pointers                   *
    //!*******************************************************************
    //! do OS (for acquire) and sockets first so we got logging
    InitOSFunctionPointers();
    InitSocketFunctionPointers();

    log_init("192.168.178.3");
    log_print("Starting launcher\n");

    InitFSFunctionPointers();
    InitVPadFunctionPointers();

    log_print("Function exports loaded\n");

    //!*******************************************************************
    //!                    Initialize heap memory                        *
    //!*******************************************************************
    log_print("Initialize memory management\n");
    //! We don't need bucket and MEM1 memory so no need to initialize
    //memoryInitialize();

    //!*******************************************************************
    //!                        Initialize FS                             *
    //!*******************************************************************
    log_printf("Mount SD partition\n");
    mount_sd_fat("sd");

	for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
        consoleArrayTv[i] = NULL;

	for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
        consoleArrayDrc[i] = NULL;

    VPADInit();

    // Prepare screen
    int screen_buf0_size = 0;

    // Init screen and screen buffers
    OSScreenInit();
    screen_buf0_size = OSScreenGetBufferSizeEx(0);
    OSScreenSetBufferEx(0, (void *)0xF4000000);
    OSScreenSetBufferEx(1, (void *)(0xF4000000 + screen_buf0_size));

    OSScreenEnableEx(0, 1);
    OSScreenEnableEx(1, 1);

    // Clear screens
    OSScreenClearBufferEx(0, 0);
    OSScreenClearBufferEx(1, 0);

    // Flip buffers
    OSScreenFlipBuffersEx(0);
    OSScreenFlipBuffersEx(1);

    int res = IOSUHAX_Open(NULL);
    if(res < 0)
    {
        console_printf(1, "IOSUHAX_open failed - Ensure you are using MochaCFW\n");
        sleep(2);
        return 0;
    }

    int fsaFd = IOSUHAX_FSA_Open();
    if(fsaFd < 0)
    {
        console_printf(1, "IOSUHAX_FSA_Open failed\n");
        sleep(2);
        return 0;
    }

    int vpadError = -1;
    VPADData vpad;

    int initScreen = 1;

    static const char* paths_output[] =
    {
				"account.dat",
        "ccerts",
        "scerts",
        "Miis",
        "Friends List (JP)",
        "Friends List (US)",
        "Friends List (EU)",
    };

    static const char* selection_paths[] =
    {
				"/vol/storage_mlc01/usr/save/system/act/",
        "/vol/storage_mlc01/sys/title/0005001b/10054000/content/ccerts",
        "/vol/storage_mlc01/sys/title/0005001b/10054000/content/scerts",
        "/vol/storage_mlc01/sys/title/0005001b/10056000/",
        "/vol/storage_mlc01/sys/title/00050030/1001500A/",
        "/vol/storage_mlc01/sys/title/00050030/1001510A/",
        "/vol/storage_mlc01/sys/title/00050030/1001520A/",
    };

    static const char* selection_paths_description[] =
    {
        "",
        "",
        "",
        "",
        "(JP)",
        "(US)",
        "(EU)",
    };

    int selectedItem = 0;

    while(1)
    {
        //! update only at 50 Hz, thats more than enough
        VPADRead(0, &vpad, 1, &vpadError);

        if(initScreen)
        {
            initScreen = 0;

            //! free memory
            for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
            {
                if(consoleArrayTv[i])
                    free(consoleArrayTv[i]);
                consoleArrayTv[i] = 0;
            }

            for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
            {
                if(consoleArrayDrc[i])
                    free(consoleArrayDrc[i]);
                consoleArrayDrc[i] = 0;
            }

            // Clear screens
            OSScreenClearBufferEx(0, 0);
            OSScreenClearBufferEx(1, 0);


            console_print_pos(0, 1, "-- Dumpling v0.0.1 by emiyl --");
            console_print_pos(0, 2, "Based on FT2SD by Dimok");

            console_print_pos(0, 4, "Select what to dump to SD card and press A to start dump.");
            console_print_pos(0, 5, "Hold B to cancel dump.");

            u32 i;
            for(i = 0; i < (sizeof(selection_paths) / 4); i++)
            {
                if(selectedItem == (int)i)
                {
                    console_print_pos(0, 7 + i, "--> %s %s", selection_paths[i], selection_paths_description[i]);
                }
                else
                {
                    console_print_pos(0, 7 + i, "    %s %s", selection_paths[i], selection_paths_description[i]);
                }
            }
            // Flip buffers
            OSScreenFlipBuffersEx(0);
            OSScreenFlipBuffersEx(1);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_DOWN))
        {
            selectedItem = (selectedItem + 1) % (sizeof(selection_paths) / 4);
            initScreen = 1;
            usleep(100000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_UP))
        {
            selectedItem--;
            if(selectedItem < 0)
                selectedItem =  (sizeof(selection_paths) / 4) - 1;
            initScreen = 1;
            usleep(100000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_A))
        {
            const char *dev_path = (selectedItem < 0) ? selection_paths[selectedItem] : NULL;
            const char *mount_path = (selectedItem >= 0) ? selection_paths[selectedItem] : "/vol/storage_ft_content";

            int res = mount_fs("dev", fsaFd, dev_path, mount_path);
            if(res < 0)
            {
                console_printf(1, "Mount of %s to %s failed", dev_path, mount_path);
            }
            else
            {
                char *targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
                if(targetPath)
                {
                    strcpy(targetPath, "dev:/");
										char sdPath[255];
										snprintf(sdPath, sizeof(sdPath), "sd:/dumpling%s", selection_paths[selectedItem]);
                    DumpDir(targetPath, sdPath);

                    free(targetPath);
                }
                unmount_fs("dev");
                console_printf(1, "Dump complete");
            }
            sleep(3);
            initScreen = 1;
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_HOME))
        {
            break;
        }

		usleep(50000);
    }

    //! free memory
	for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
    {
        if(consoleArrayTv[i])
            free(consoleArrayTv[i]);
    }

	for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
    {
        if(consoleArrayDrc[i])
            free(consoleArrayDrc[i]);
    }

    //!*******************************************************************
    //!                    Enter main application                        *
    //!*******************************************************************

    log_printf("Unmount SD\n");
    unmount_sd_fat("sd");

    IOSUHAX_FSA_Close(fsaFd);
    IOSUHAX_Close();

    log_printf("Release memory\n");
    //memoryRelease();
    log_deinit();

    return EXIT_SUCCESS;
}
