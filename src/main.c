#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <fat.h>
#include <sys/dirent.h>
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

int dumpFunc(const char *path, int item, int dump_source, int dump_target, int logs)
{
			int res = IOSUHAX_Open(NULL);
	    if(res < 0)
	    {
	        console_printf(2, "IOSUHAX_open failed");
	        console_printf(1, "Ensure you are using MochaCFW");
	        sleep(2);
	        return 0;
	    }

	    int fsaFd = IOSUHAX_FSA_Open();
	    if(fsaFd < 0)
	    {
	        console_printf(2, "IOSUHAX_FSA_Open failed");
	        console_printf(2, "Run MochaCFW again");
	        sleep(2);
	        return 0;
	    }

			char *targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
			if(targetPath)
			{
					fatInitDefault();

					const char *dev_path = (!dump_source && (item == 2)) ? "/dev/odd03" : NULL;

					char mount_path[255];
					if (!dump_source && (item == 2)) strcpy(mount_path, "/vol/storage_odd_content");
					else if (dump_source) snprintf(mount_path, sizeof(mount_path), "/vol/storage_usb01%s", strchr(path, '/'));
					else snprintf(mount_path, sizeof(mount_path), "/vol/storage_mlc01%s", strchr(path, '/'));

					int res = mount_fs("dev", fsaFd, dev_path, mount_path);
					if(res < 0)
					{
							console_printf(1, "Mount of %s to %s failed", dev_path, mount_path);
		          console_printf(2, "Ensure the drive is connected.");
		          console_printf(1, "Press X to return.");

		          InitVPadFunctionPointers();
		          VPADInit();
		          int vpadError = -1;
		          VPADData vpad;
		          usleep(150000);

		          for(;;) {
		            VPADRead(0, &vpad, 1, &vpadError);
		            if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_X))
		            {
		                break;
		            }
		            usleep(50000);
		          }
							return 0;
					}

					VirtualMountDevice("dev:/");
					VirtualMountDevice("sd:/");
					VirtualMountDevice("usb:/");

					char toDrive[127] =  "sd";
					char toDir[255] = "sd:/dumpling/not_set/";
					strcpy(targetPath, "dev:/");
					if (dump_target) strcpy(toDrive, "usb");
					if (!dump_source && (item == 0)) snprintf(toDir, sizeof(toDir), "%s:/dumpling/online_files/mlc01/%s", toDrive, path + 13);
					if (!dump_source && (item == 1)) snprintf(toDir, sizeof(toDir), "%s:/dumpling/friends_list/", toDrive);
					if (!dump_source && (item == 2)) snprintf(toDir, sizeof(toDir), "%s:/dumpling/disc", toDrive);
					if ((dump_source) ? (item == 0) : (item == 3)) snprintf(toDir, sizeof(toDir), "%s:/dumpling/games/", toDrive);
					if ((dump_source) ? (item == 1) : (item == 4)) snprintf(toDir, sizeof(toDir), "%s:/dumpling/updates/", toDrive);
					if ((dump_source) ? (item == 2) : (item == 5)) snprintf(toDir, sizeof(toDir), "%s:/dumpling/dlc/", toDrive);
					if ((dump_source) ? (item == 3) : (item == 6)) snprintf(toDir, sizeof(toDir), "%s:/dumpling/saves/", toDrive);
					if (!dump_source && (item == 7)) snprintf(toDir, sizeof(toDir), "%s:/dumpling/nand/", toDrive);

					DumpDir(targetPath, toDir, logs);
					free(targetPath);
			}

			if (dump_source | (item != 1)) {
				console_printf(1, "Dump complete");
				usleep(500000);
			}

			fatUnmount("sd");
			fatUnmount("usb");
			IOSUHAX_sdio_disc_interface.shutdown();
			IOSUHAX_usb_disc_interface.shutdown();

			unmount_fs("dev");
			UnmountVirtualPaths();

			IOSUHAX_FSA_Close(fsaFd);
			IOSUHAX_Close();

			return 1;
}

int titles_menu(int dump_source, int dump_target, int selectedItem) {
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

for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
			consoleArrayTv[i] = NULL;

for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
			consoleArrayDrc[i] = NULL;

	VPADInit();

	int vpadError = -1;
	VPADData vpad;

	int initScreen = 1;

	int stringcount = 0;
	char **folders = NULL;

	int res = IOSUHAX_Open(NULL);
	if(res < 0)
	{
			console_printf(2, "IOSUHAX_open failed");
			console_printf(1, "Ensure you are using MochaCFW");
			sleep(1);
			return 0;
	}

	int fsaFd = IOSUHAX_FSA_Open();
	if(fsaFd < 0)
	{
			console_printf(2, "IOSUHAX_FSA_Open failed");
			console_printf(2, "Run MochaCFW again");
			sleep(1);
			return 0;
	}

	fatInitDefault();
	const char *dev_path = NULL;

	char mount_path[255];
	if (dump_source) strcpy(mount_path, "/vol/storage_usb01/usr/title/00050000");
	else						 strcpy(mount_path, "/vol/storage_mlc01/usr/title/00050000");

	int ret = mount_fs("dev", fsaFd, dev_path, mount_path);
	if (ret < 0) return 0;

	VirtualMountDevice("dev:/");
	VirtualMountDevice("sd:/");
	VirtualMountDevice("usb:/");

	u32 size_of_folders = 0;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir ("dev:/")) != NULL) {
	/* print all the files and directories within directory */
			while ((ent = readdir (dir)) != NULL) {
					stringcount++;
					if (folders == NULL) {
							folders = malloc(sizeof(char *) * stringcount);
					} else {
							folders = realloc(folders, sizeof(char *) * stringcount);
					}
					int single_string_len = strlen(ent->d_name);
					char *single_string = malloc(sizeof(char) * single_string_len + 1);
					strcpy(single_string, ent->d_name);
					folders[stringcount - 1] = single_string;
					size_of_folders++;
			}
			closedir (dir);
	} else {
			perror ("");
			return -1;
	}

	for (int str = 0; str < stringcount; str++) {
			puts(folders[str]);
	}

	int checkBox[(sizeof(folders) / 4)];

	u32 i;
	for(i = 0; i < (sizeof(folders) / 4); i++) {
		checkBox[i] = 0;
	}

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


					console_print_pos(0, 1, "-- dumpling v0.5 pre-release by emiyl --");
					console_print_pos(0, 2, "Source: %s // Target: %s", (dump_source) ? "USB" : "System", (dump_target) ? "USB" : "SD");

					console_print_pos(0, 4, "Press A to select and START to begin dumping.");
					console_print_pos(0, 5, "Use L/R to change source and ZL/ZR to change target.");

					u32 i;

					for(i = 0; i < size_of_folders; i++)
					{
							if (checkBox[i]) console_print_pos(0, 7 + i, "[x]");
							else console_print_pos(0, 7 + i, "[ ]");

							if (selectedItem == (int)i) console_print_pos(4, 7 + i, "> %s", folders[i]);
							else console_print_pos(4, 7 + i, "  %s", folders[i]);
					}

					console_print_pos(0, 16, "Hold B to cancel. Press X to return to main menu.");

					// Flip buffers
					OSScreenFlipBuffersEx(0);
					OSScreenFlipBuffersEx(1);
			}

			if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_DOWN))
			{
					selectedItem = (selectedItem + 1) % size_of_folders;
					initScreen = 1;
					usleep(150000);
			}

			if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_UP))
			{
					selectedItem--;
					if(selectedItem < 0)
							selectedItem =  size_of_folders - 1;
					initScreen = 1;
					usleep(150000);
			}

			if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_R | VPAD_BUTTON_RIGHT | VPAD_BUTTON_L | VPAD_BUTTON_LEFT)))
			{
					u32 i;
					for(i = 0; i < size_of_folders; i++) {
						checkBox[i] = 0;
					}
					dump_source = !dump_source;
					if (selectedItem > 4)
						selectedItem = 3;
					initScreen = 1;
					usleep(150000);
			}

			if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_ZR | VPAD_BUTTON_ZL)))
			{
					dump_target = !dump_target;
					initScreen = 1;
					usleep(150000);
			}

			if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_A))
			{
					checkBox[selectedItem] = !checkBox[selectedItem];
					initScreen = 1;
					usleep(150000);
			}

			if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_X))
			{
					fatUnmount("sd");
					fatUnmount("usb");
					IOSUHAX_sdio_disc_interface.shutdown();
					IOSUHAX_usb_disc_interface.shutdown();

					unmount_fs("dev");
					UnmountVirtualPaths();

					IOSUHAX_FSA_Close(fsaFd);
					IOSUHAX_Close();
					return 1;
			}

			if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_PLUS))
			{
					u32 i;
					for(i = 0; i < size_of_folders; i++)
					{
							if (checkBox[i]) {
									char selection_path[255];
									snprintf(selection_path, sizeof(selection_path), "storage_%s01:/usr/title/00050000/%s", (dump_source) ? "usb" : "mlc", folders[i]);
									dumpFunc(selection_path, i, dump_source, dump_target, 1);
							}
							initScreen = 1;
					}
			}

			if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_HOME))
			{
					fatUnmount("sd");
					fatUnmount("usb");
					IOSUHAX_sdio_disc_interface.shutdown();
					IOSUHAX_usb_disc_interface.shutdown();

					unmount_fs("dev");
					UnmountVirtualPaths();

					IOSUHAX_FSA_Close(fsaFd);
					IOSUHAX_Close();
					return 1;
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

	for (int str = 0; str < stringcount; str++) {
			free(folders[str]);
	}
	free(folders);

	log_printf("Release memory\n");
	//memoryRelease();
	log_deinit();
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

    int vpadError = -1;
    VPADData vpad;

    int initScreen = 1;

    static const char* mlc_selection_text[] =
    {
        "Online Files",
        "Friends List",
        "Disc dump",
        "All digital games",
        "All digital updates",
        "All digital DLC",
        "All saves data",
        "Full MLC01",
    };

    static const char* mlc_selection_paths[] =
    {
        NULL,
        NULL,
        "storage_odd_content:/",
        "storage_mlc:/usr/title/00050000",
        "storage_mlc:/usr/title/0005000E",
        "storage_mlc:/usr/title/0005000C",
        "storage_mlc:/usr/save/00050000",
        "storage_mlc:/",
    };

    static const char* usb_selection_text[] =
    {
        "All digital games",
        "All digital updates",
        "All digital DLC",
        "All saves data",
    };

    static const char* usb_selection_paths[] =
    {
        "storage_usb:/usr/title/00050000",
        "storage_usb:/usr/title/0005000E",
        "storage_usb:/usr/title/0005000C",
        "storage_usb:/usr/save/00050000",
    };

    static const char* online_files_paths[] =
    {
			"storage_mlc:/usr/save/system/act",
			"storage_mlc:/sys/title/0005001b/10054000/",
			"storage_mlc:/sys/title/0005001b/10056000",
    };

    static const char* friends_list_paths[] =
    {
			"storage_mlc:/sys/title/00050030/1001500A",
			"storage_mlc:/sys/title/00050030/1001510A",
			"storage_mlc:/sys/title/00050030/1001520A",
    };

    int selectedItem = 0;
		int dump_source = 0;
		int dump_target = 0;

		int checkBox[(((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4)];

		u32 i;
		for(i = 0; i < (((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4); i++) {
			checkBox[i] = 0;
		}

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


            console_print_pos(0, 1, "-- dumpling v0.5 pre-release by emiyl --");
            console_print_pos(0, 2, "Source: %s // Target: %s", (dump_source) ? "USB" : "System", (dump_target) ? "USB" : "SD");

            console_print_pos(0, 4, "Press A to select and START to begin dumping.");
            console_print_pos(0, 5, "Use L/R to change source and ZL/ZR to change target.");

            u32 i;

            for(i = 0; i < (((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4); i++)
            {
								if (checkBox[i]) console_print_pos(0, 7 + i, "[x]");
								else console_print_pos(0, 7 + i, "[ ]");

                if (selectedItem == (int)i) console_print_pos(4, 7 + i, "> %s", (dump_source) ? usb_selection_text[i] : mlc_selection_text[i]);
                else console_print_pos(4, 7 + i, "  %s", (dump_source) ? usb_selection_text[i] : mlc_selection_text[i]);
            }

            console_print_pos(0, 16, "Hold B to cancel. Press X to dump invidual titles.");

            // Flip buffers
            OSScreenFlipBuffersEx(0);
            OSScreenFlipBuffersEx(1);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_DOWN))
        {
            selectedItem = (selectedItem + 1) % (((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4);
            initScreen = 1;
            usleep(150000);
        }

				if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_UP))
				{
						selectedItem--;
						if(selectedItem < 0)
								selectedItem =  (((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4) - 1;
						initScreen = 1;
						usleep(150000);
				}

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_R | VPAD_BUTTON_RIGHT | VPAD_BUTTON_L | VPAD_BUTTON_LEFT)))
        {
						u32 i;
						for(i = 0; i < (((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4); i++) {
							checkBox[i] = 0;
						}
            dump_source = !dump_source;
						if (selectedItem > 4)
							selectedItem = 3;
            initScreen = 1;
            usleep(150000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_ZR | VPAD_BUTTON_ZL)))
        {
            dump_target = !dump_target;
            initScreen = 1;
            usleep(150000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_A))
        {
						checkBox[selectedItem] = !checkBox[selectedItem];
						initScreen = 1;
            usleep(150000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_X))
        {
						usleep(150000);
						titles_menu(dump_source, dump_target, selectedItem);
						initScreen = 1;
						usleep(150000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_PLUS))
        {
						u32 i, j;
						for(i = 0; i < (((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4); i++)
						{
								if (checkBox[i]) {
										if (!dump_source && (i < 2)) {
												if (i == 0) for (j=0;j<sizeof(online_files_paths)/4;j++) dumpFunc(online_files_paths[j], i, dump_source, dump_target, 1);
												if (i == 1) {
													for (j=0;j<sizeof(friends_list_paths)/4;j++) {dumpFunc(friends_list_paths[j], i, dump_source, dump_target, 0);}
													console_printf(1, "Dump complete");
													usleep(500000);
												}
										} else dumpFunc((dump_source) ? usb_selection_paths[i] : mlc_selection_paths[i], i, dump_source, dump_target, 1);
								}
								initScreen = 1;
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

    log_printf("Release memory\n");
    //memoryRelease();
    log_deinit();

    return EXIT_SUCCESS;
}
