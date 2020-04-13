#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <fat.h>
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/fs_functions.h"
#include <dynamic_libs/gx2_functions.h>
#include <dynamic_libs/sys_functions.h>
#include <dynamic_libs/vpad_functions.h>
#include <dynamic_libs/padscore_functions.h>
#include <dynamic_libs/socket_functions.h>
#include <dynamic_libs/ax_functions.h>
#include "fs/fs_utils.h"
#include "fs/sd_fat_devoptab.h"
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <iosuhax_disc_interface.h>
#include "system/memory.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "common/common.h"
#include "sd_dumper.h"
#include "virtualpath.h"

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

/*int CheckCancel(void)
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
}*/

int dump_func(const char *mount_path, int selectedItem, int fsaFd, int initScreen, int dump_source, int dump_target)
{
		int res = mount_fs("dev", fsaFd, NULL, mount_path);
		if(res < 0)
		{
				console_printf(1, "Mount of %s to %s failed", NULL, mount_path);
		}
		else
		{
				char *targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
				if(targetPath)
				{
						strcpy(targetPath, "dev:/");
						char sdPath[255] = "sd:/dumpling";
						char dumpTo[127] = "dumps";
						if (!dump_source && (selectedItem == 0)) snprintf(dumpTo, sizeof(dumpTo), "online_files/%s", mount_path + 13);
						if (!dump_source && (selectedItem == 1)) strcpy(dumpTo, "friends_list");
						if ((!dump_source && (selectedItem == 2)) | (dump_source && (selectedItem == 0))) strcpy(dumpTo, "games");
						if ((!dump_source && (selectedItem == 3)) | (dump_source && (selectedItem == 1))) strcpy(dumpTo, "updates");
						if ((!dump_source && (selectedItem == 4)) | (dump_source && (selectedItem == 2))) strcpy(dumpTo, "dlc");
						if ((!dump_source && (selectedItem == 5)) | (dump_source && (selectedItem == 3))) strcpy(dumpTo, "saves");
						if (!dump_source && (selectedItem == 6)) strcpy(dumpTo, "nand");
						snprintf(sdPath, sizeof(sdPath), "%s:/dumpling/%s",  (dump_target) ? "usb" : "sd", dumpTo);
						DumpDir(targetPath, sdPath);

						free(targetPath);
				}
				unmount_fs("dev");
				if (selectedItem != 1) console_printf(1, "Dump complete");
		}
		if (selectedItem != 1) sleep(1);
		initScreen = 1;
		return initScreen;
}

//just to be able to call async
void someFunc(void *arg)
{
    (void)arg;
}

static int mcp_hook_fd = -1;
int MCPHookOpen()
{
    //take over mcp thread
    mcp_hook_fd = MCP_Open();
    if(mcp_hook_fd < 0)
        return -1;
    IOS_IoctlAsync(mcp_hook_fd, 0x62, (void*)0, 0, (void*)0, 0, someFunc, (void*)0);
    //let wupserver start up
    sleep(1);
    if(IOSUHAX_Open("/dev/mcp") < 0)
    {
        MCP_Close(mcp_hook_fd);
        mcp_hook_fd = -1;
        return -1;
    }
    return 0;
}

void MCPHookClose()
{
    if(mcp_hook_fd < 0)
        return;
    //close down wupserver, return control to mcp
    IOSUHAX_Close();
    //wait for mcp to return
    sleep(1);
    MCP_Close(mcp_hook_fd);
    mcp_hook_fd = -1;
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

		int fsaFd = -1;
		int iosuhaxMount = 0;

		int res = IOSUHAX_Open(NULL);
		if(res < 0)
				res = MCPHookOpen();
		if(res < 0)
		{
				log_printf("IOSUHAX_open failed\nEnsure you are using MochaCFW\n");
        sleep(2);
				return 0;
		}
		else
		{
				iosuhaxMount = 1;
				fatInitDefault();

				fsaFd = IOSUHAX_FSA_Open();
				if(fsaFd < 0)
				{
						log_printf("IOSUHAX_FSA_Open failed\n");
				}

				//mount_fs("slccmpt01", fsaFd, "/dev/slccmpt01", "/vol/storage_slccmpt01");
				//mount_fs("storage_odd_tickets", fsaFd, "/dev/odd01", "/vol/storage_odd_tickets");
				//mount_fs("storage_odd_updates", fsaFd, "/dev/odd02", "/vol/storage_odd_updates");
				//mount_fs("storage_odd_content", fsaFd, "/dev/odd03", "/vol/storage_odd_content");
				//mount_fs("storage_odd_content2", fsaFd, "/dev/odd04", "/vol/storage_odd_content2");
				//mount_fs("storage_slc", fsaFd, NULL, "/vol/system");
				//mount_fs("storage_mlc", fsaFd, NULL, "/vol/storage_mlc01");
				//mount_fs("storage_usb", fsaFd, NULL, "/vol/storage_usb01");

				VirtualMountDevice("sd:/");
				//VirtualMountDevice("slccmpt01:/");
				//VirtualMountDevice("storage_odd_tickets:/");
				//VirtualMountDevice("storage_odd_updates:/");
				//VirtualMountDevice("storage_odd_content:/");
				//VirtualMountDevice("storage_odd_content2:/");
				//VirtualMountDevice("storage_slc:/");
				//VirtualMountDevice("storage_mlc:/");
				//VirtualMountDevice("storage_usb:/");
				VirtualMountDevice("usb:/");
		}

    int vpadError = -1;
    VPADData vpad;

    int initScreen = 1;

    static const char* mlc_paths_output[] =
    {
				"Online Files",
        "Friends List",
        "All digital games",
        "All digital updates",
        "All digital DLC",
        "All save data",
        "Full MLC01 (Very long time!)",
    };

    static const char* usb_paths_output[] =
    {
        "All digital games",
        "All digital updates",
        "All digital DLC",
        "All save data",
    };

    static const char* mlc_selection_paths[] =
    {
				NULL,
        NULL,
        "/vol/storage_mlc01/usr/title/00050000/", // digital games directory
        "/vol/storage_mlc01/usr/title/0005000E/", // updates directory
        "/vol/storage_mlc01/usr/title/0005000C/", // DLC directory
        "/vol/storage_mlc01/usr/save/00050000/", // save data directory
        "/vol/storage_mlc01/", // nand directory
    };

    static const char* usb_selection_paths[] =
    {
        "/vol/storage_usb01/usr/title/00050000/", // digital games directory
        "/vol/storage_usb01/usr/title/0005000E/", // updates directory
        "/vol/storage_usb01/usr/title/0005000C/", // DLC directory
        "/vol/storage_usb01/usr/save/00050000/", // save data directory
    };

    int selectedItem = 0;

		int dump_source = 0;
		int dump_target = 0;

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


            console_print_pos(0, 1, "-- Dumpling v0.3.0 by emiyl --");
            console_print_pos(0, 2, "Source: %s // Target: %s", (dump_source) ? "USB" : "MLC", (dump_target) ? "USB" : "SD");

            console_print_pos(0, 4, "Select what to dump to USB storage and press A to start dump.");
            console_print_pos(0, 5, "Use L/R to change source and ZL/ZR to change target.");

            u32 i;
						for(i = 0; i < (((dump_source) ? sizeof(usb_selection_paths) : sizeof(mlc_selection_paths)) / 4); i++)
            {
                if(selectedItem == (int)i)
                {
                    console_print_pos(0, 7 + i, "--> %s", ((dump_source) ? usb_paths_output : mlc_paths_output)[i]); //, selection_paths_description[i]);
                }
                else
                {
                    console_print_pos(0, 7 + i, "    %s", ((dump_source) ? usb_paths_output : mlc_paths_output)[i]); //, selection_paths_description[i]);
                }
            }

            console_print_pos(0, 16, "Hold B to cancel dumping.");

            // Flip buffers
            OSScreenFlipBuffersEx(0);
            OSScreenFlipBuffersEx(1);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_DOWN))
        {
            selectedItem = (selectedItem + 1) % (((dump_source) ? sizeof(usb_selection_paths) : sizeof(mlc_selection_paths)) / 4);
            initScreen = 1;
            usleep(200000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_UP))
        {
            selectedItem--;
            if(selectedItem < 0)
                selectedItem =  (((dump_source) ? sizeof(usb_selection_paths) : sizeof(mlc_selection_paths)) / 4) - 1;
            initScreen = 1;
            usleep(200000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_L)))
        {
            dump_source = !dump_source;
						selectedItem = 0;
            initScreen = 1;
            usleep(200000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_R)))
        {
            dump_source = !dump_source;
						selectedItem = 0;
            initScreen = 1;
            usleep(200000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_ZL)))
        {
            dump_target = !dump_target;
            initScreen = 1;
            usleep(200000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_ZR)))
        {
            dump_target = !dump_target;
            initScreen = 1;
            usleep(200000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_A))
        {
						if (dump_source | (selectedItem > 1))
							initScreen = dump_func(((dump_source) ? usb_selection_paths : mlc_selection_paths)[selectedItem], selectedItem, fsaFd, initScreen, dump_source, dump_target);
						else if (selectedItem == 0) {
							initScreen = dump_func("/vol/storage_mlc01/usr/save/system/act/"											, selectedItem, fsaFd, initScreen, dump_source, dump_target); // account.dat
							initScreen = dump_func("/vol/storage_mlc01/sys/title/0005001b/10054000/content/ccerts", selectedItem, fsaFd, initScreen, dump_source, dump_target); // ccerts
							initScreen = dump_func("/vol/storage_mlc01/sys/title/0005001b/10054000/content/scerts", selectedItem, fsaFd, initScreen, dump_source, dump_target); // scerts
							initScreen = dump_func("/vol/storage_mlc01/sys/title/0005001b/10056000/"							, selectedItem, fsaFd, initScreen, dump_source, dump_target); // mii data
						}
						else if (selectedItem == 1) {
							initScreen = dump_func("/vol/storage_mlc01/sys/title/00050030/1001500A/", selectedItem, fsaFd, initScreen, dump_source, dump_target); // JP friends list
							initScreen = dump_func("/vol/storage_mlc01/sys/title/00050030/1001510A/", selectedItem, fsaFd, initScreen, dump_source, dump_target); // US friends list
							initScreen = dump_func("/vol/storage_mlc01/sys/title/00050030/1001520A/", selectedItem, fsaFd, initScreen, dump_source, dump_target); // EU friends list
							console_printf(1, "Dump complete");
							sleep(1);
						}
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
		if(iosuhaxMount)
		{
				fatUnmount("sd");
				fatUnmount("usb");
				IOSUHAX_sdio_disc_interface.shutdown();
				IOSUHAX_usb_disc_interface.shutdown();

				/*unmount_fs("slccmpt01");
				unmount_fs("storage_odd_tickets");
				unmount_fs("storage_odd_updates");
				unmount_fs("storage_odd_content");
				unmount_fs("storage_odd_content2");
				unmount_fs("storage_slc");
				unmount_fs("storage_mlc");
				unmount_fs("storage_usb");*/
				IOSUHAX_FSA_Close(fsaFd);
				if(mcp_hook_fd >= 0)
						MCPHookClose();
				else
						IOSUHAX_Close();
		}
		else
		{
				unmount_sd_fat("sd:/");
		}

		UnmountVirtualPaths();

    IOSUHAX_FSA_Close(fsaFd);
    IOSUHAX_Close();

    log_printf("Release memory\n");
    //memoryRelease();
    log_deinit();

    return EXIT_SUCCESS;
}
