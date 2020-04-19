#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
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
#include <mxml.h>
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

int ifDirExists(const char *path) {
	DIR *dir = opendir(path);
	if (dir != NULL)
	{
		closedir(dir);
		return 1;
	}

	return 0;
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

int dumpFunc(const char *path, const char *dump_dir, int dev, int dump_source, int dump_target, int skipMountErrors)
{
	mcp_hook_fd = -1;
	int res = IOSUHAX_Open(NULL);
	if(res < 0)
        res = MCPHookOpen();
	if(res < 0)
	{
		console_printf(2, "IOSUHAX_open failed");
		console_printf(1, "Ensure you are using CFW");
		sleep(2);
		return 0;
	}

	int fsaFd = IOSUHAX_FSA_Open();
	if(fsaFd < 0)
	{
		console_printf(2, "IOSUHAX_FSA_Open failed");
		console_printf(2, "Run CFW again");
		sleep(2);
		return 0;
	}

	char *targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
	if(targetPath)
	{
		fatInitDefault();

		const char *dev_path = (!dump_source && dev) ? "/dev/odd03" : NULL;

		char mount_path[255];
		if (!dump_source && dev) strcpy(mount_path, "/vol/storage_odd_content");
		else if (dump_source) snprintf(mount_path, sizeof(mount_path), "/vol/storage_usb01%s", strchr(path, '/'));
		else snprintf(mount_path, sizeof(mount_path), "/vol/storage_mlc01%s", strchr(path, '/'));

		int res = mount_fs("dev", fsaFd, dev_path, mount_path);
		if ((res < 0) | !ifDirExists("dev:/"))
		{
			if (!skipMountErrors)
			{
				console_printf(1, "Mount of %s to %s failed", dev_path, mount_path);
				console_printf(2, "Ensure the drive is connected.");
				console_printf(1, "Press B to return.");

				InitVPadFunctionPointers();
				VPADInit();
				int vpadError = -1;
				VPADData vpad;
				usleep(150000);

				for(;;) {
					VPADRead(0, &vpad, 1, &vpadError);
					if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_B))
						break;
					usleep(50000);
				}
			}
			
			unmount_fs("dev");
			UnmountVirtualPaths();

			IOSUHAX_FSA_Close(fsaFd);
			if(mcp_hook_fd >= 0)
				MCPHookClose();
			else
				IOSUHAX_Close();

			return 0;
		}

		VirtualMountDevice("dev:/");
		VirtualMountDevice("sd:/");
		VirtualMountDevice("usb:/");

		if (!ifDirExists("dev:/"))
		{
			if (!skipMountErrors)
			{
				console_printf(1, "Mount of %s to %s failed", dev_path, mount_path);
				console_printf(2, "Ensure the drive is connected.");
				console_printf(1, "Press B to return.");

				InitVPadFunctionPointers();
				VPADInit();
				int vpadError = -1;
				VPADData vpad;
				usleep(150000);

				for(;;) {
					VPADRead(0, &vpad, 1, &vpadError);
					if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_B))
						break;
					usleep(50000);
				}
			}

			fatUnmount("sd");
			fatUnmount("usb");
			IOSUHAX_sdio_disc_interface.shutdown();
			IOSUHAX_usb_disc_interface.shutdown();

			unmount_fs("dev");
			UnmountVirtualPaths();

			IOSUHAX_FSA_Close(fsaFd);
			if(mcp_hook_fd >= 0)
				MCPHookClose();
			else
				IOSUHAX_Close();

			return 0;
		}
		
		if (!ifDirExists("sd:/") && !dump_target)
		{
			console_printf(1, "SD mount failed", dev_path, mount_path);
			console_printf(2, "Ensure the drive is connected.");
			console_printf(1, "Press B to return.");

			InitVPadFunctionPointers();
			VPADInit();
			int vpadError = -1;
			VPADData vpad;
			usleep(150000);

			for(;;) {
				VPADRead(0, &vpad, 1, &vpadError);
				if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_B))
					break;
				usleep(50000);
			}

			fatUnmount("sd");
			fatUnmount("usb");
			IOSUHAX_sdio_disc_interface.shutdown();
			IOSUHAX_usb_disc_interface.shutdown();

			unmount_fs("dev");
			UnmountVirtualPaths();

			IOSUHAX_FSA_Close(fsaFd);
			if(mcp_hook_fd >= 0)
				MCPHookClose();
			else
				IOSUHAX_Close();

			return 0;
		}
		
		if (!ifDirExists("usb:/") && dump_target)
		{
			console_printf(1, "USB mount failed", dev_path, mount_path);
			console_printf(2, "Ensure the drive is connected.");
			console_printf(1, "Press B to return.");

			InitVPadFunctionPointers();
			VPADInit();
			int vpadError = -1;
			VPADData vpad;
			usleep(150000);

			for(;;) {
				VPADRead(0, &vpad, 1, &vpadError);
				if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_B))
					break;
				usleep(50000);
			}

			fatUnmount("sd");
			fatUnmount("usb");
			IOSUHAX_sdio_disc_interface.shutdown();
			IOSUHAX_usb_disc_interface.shutdown();

			unmount_fs("dev");
			UnmountVirtualPaths();

			IOSUHAX_FSA_Close(fsaFd);
			if(mcp_hook_fd >= 0)
				MCPHookClose();
			else
				IOSUHAX_Close();

			return 0;
		}

		char toDrive[127] =  "sd";
		char toDir[255] = "sd:/dumpling/not_set/";
		strcpy(targetPath, "dev:/");

		if (dump_target) strcpy(toDrive, "usb");
		if (!strcmp(dump_dir,"online_files/mlc01"))
			snprintf(toDir, sizeof(toDir), "%s:/dumpling/%s/%s", toDrive, dump_dir, path + 13);
		else
			snprintf(toDir, sizeof(toDir), "%s:/dumpling/%s", toDrive, dump_dir);

		DumpDir(targetPath, toDir);
		free(targetPath);
	}

	console_printf(1, "Dump complete");
	usleep(500000);

	fatUnmount("sd");
	fatUnmount("usb");
	IOSUHAX_sdio_disc_interface.shutdown();
	IOSUHAX_usb_disc_interface.shutdown();

	unmount_fs("dev");
	UnmountVirtualPaths();

	IOSUHAX_FSA_Close(fsaFd);
	if(mcp_hook_fd >= 0)
		MCPHookClose();
	else
		IOSUHAX_Close();

	return 1;
}

int loop = 1;

int getMlcMetaData = 1;
int getUsbMetaData = 1;
char **mlc_meta_names = NULL;
char **mlc_stored_folders = NULL;
char **usb_meta_names = NULL;;
char **usb_stored_folders = NULL;;

const char head_string[50] = "-- dumpling v1.0.1 by emiyl --";

int titles_menu(int dump_source, int dump_target) {
	InitOSFunctionPointers();
	InitSocketFunctionPointers();

	InitFSFunctionPointers();
	InitVPadFunctionPointers();

	for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
		consoleArrayTv[i] = NULL;

	for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
		consoleArrayDrc[i] = NULL;

	VPADInit();

	int vpadError = -1;
	VPADData vpad;

	mcp_hook_fd = -1;
	int res = IOSUHAX_Open(NULL);
	if(res < 0)
        res = MCPHookOpen();
	if(res < 0)
	{
		console_printf(2, "IOSUHAX_open failed");
		console_printf(1, "Ensure you are using CFW");
		sleep(1);
		return 0;
	}

	int fsaFd = IOSUHAX_FSA_Open();
	if(fsaFd < 0)
	{
		console_printf(2, "IOSUHAX_FSA_Open failed");
		console_printf(1, "Run CFW again");
		sleep(1);
		return 0;
	}

	int initScreen = 1;
	int selectedItem = 0;
	u32 page_number = 1;
	
	const char *dev_path = NULL;

	// MLC

	u32 mlc_folder_string_count = 0;
	u32 mlc_page_count = 0;
	char **mlc_folders = NULL;

	int ret = mount_fs("dev", fsaFd, dev_path, "/vol/storage_mlc01/usr/title/00050000");
	if (ret < 0) 
	{
		console_printf(1, "MLC mounting failed");
		sleep(1);
		return 0;
	}

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir ("dev:/")) != NULL) {
	// print all the files and directories within directory
		while ((ent = readdir (dir)) != NULL) {
			mlc_folder_string_count++;
			int meta_null = 0;
			if (mlc_folders == NULL)
				mlc_folders = malloc(sizeof(char *) * mlc_folder_string_count);
			else
				mlc_folders = realloc(mlc_folders, sizeof(char *) * mlc_folder_string_count);
			if (mlc_meta_names == NULL)
			{
				mlc_meta_names = malloc(sizeof(char *) * mlc_folder_string_count);
				meta_null = 1;
			}
			if (mlc_stored_folders == NULL)
				mlc_stored_folders = malloc(sizeof(char *) * mlc_folder_string_count);
			else
				mlc_stored_folders = realloc(mlc_stored_folders, sizeof(char *) * mlc_folder_string_count);
			int single_string_len = strlen(ent->d_name);
			char *single_string = malloc(sizeof(char) * single_string_len + 1);
			strcpy(single_string, ent->d_name);
			mlc_folders[mlc_folder_string_count - 1] = single_string;
			if (meta_null)
				mlc_meta_names[mlc_folder_string_count - 1] = single_string;
		}
		closedir (dir);
		mlc_page_count = (mlc_folder_string_count / 8);
		if ((mlc_folder_string_count % 8) > 0)
			mlc_page_count++;
	} else {
		perror ("");
		console_printf(1, "MLC mounting failed");
		sleep(1);
		return 0;
	}

	for (u32 i=0;i<mlc_folder_string_count;i++) {
		if (mlc_stored_folders[i] == mlc_folders[i])
			getMlcMetaData = 0;
		else
			mlc_stored_folders[i] = mlc_folders[i];
	}

	// Get metadata
	if (getMlcMetaData)
	{
		console_printf(1, "dumpling will now fetch metadata, which may take a while.");
		console_printf(1, "This must be done once every time you start the app.");
		getMlcMetaData = 0;
		mlc_meta_names = (char**)malloc(mlc_folder_string_count*sizeof(char*));
		if (!mlc_meta_names) {
			return 0;
		}
		for (u32 i = 0; i < mlc_folder_string_count; i++) {
			console_printf(1, "Fetching meta for %s [MLC %d/%d]", mlc_folders[i], i + 1, mlc_folder_string_count);

			char xml_path[28];
			snprintf(xml_path, sizeof(xml_path), "dev:/%s/meta/meta.xml", mlc_folders[i]);
			
			FILE *fp;
			mxml_node_t *tree;

			fp = fopen(xml_path, "r");
			tree = mxmlLoadFile(NULL, fp, MXML_OPAQUE_CALLBACK);

			char* meta_xml_getname(mxml_node_t *tree) {
				mxml_node_t* value = mxmlFindPath(tree, "menu/shortname_en");
				const char* str =  mxmlGetOpaque(value);
				char* name = (char*)malloc(strlen(str) + 1);
				strcpy(name, str);
				mxmlDelete(value);
				return name;
			}

			char *get_name = meta_xml_getname(tree);

			if (get_name == NULL)
			{
				mlc_meta_names[i] = mlc_folders[i];
				console_printf(1, "Failed to get meta\n");
			}
			else
			{
				mlc_meta_names[i] = get_name;
				console_printf(1, "%s", get_name);
			}

			mxmlDelete(tree);
			fclose(fp);
		}
	}

	unmount_fs("dev");

	int mlc_checkBox[mlc_folder_string_count];

	for(u32 i = 0; i < mlc_folder_string_count; i++)
		mlc_checkBox[i] = 0;

	// USB

	u32 usb_folder_string_count = 0;
	u32 usb_page_count = 0;
	
	int mlc_only = 0;
	char **usb_folders = NULL;

	res = mount_fs("dev", fsaFd, dev_path, "/vol/storage_usb01/usr/title/00050000");

	if ((dir = opendir ("dev:/")) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			usb_folder_string_count++;
			int meta_null = 0;
			if (usb_folders == NULL)
				usb_folders = malloc(sizeof(char *) * usb_folder_string_count);
			else
				usb_folders = realloc(usb_folders, sizeof(char *) * usb_folder_string_count);
			if (usb_meta_names == NULL)
			{
				usb_meta_names = malloc(sizeof(char *) * usb_folder_string_count);
				meta_null = 1;
			}
			if (usb_stored_folders == NULL)
				usb_stored_folders = malloc(sizeof(char *) * usb_folder_string_count);
			else
				usb_stored_folders = realloc(usb_stored_folders, sizeof(char *) * usb_folder_string_count);
			int single_string_len = strlen(ent->d_name);
			char *single_string = malloc(sizeof(char) * single_string_len + 1);
			strcpy(single_string, ent->d_name);
			usb_folders[usb_folder_string_count - 1] = single_string;
			if (meta_null)
				usb_meta_names[usb_folder_string_count - 1] = single_string;
		}
		closedir (dir);
		usb_page_count = (usb_folder_string_count / 8);
		if ((usb_folder_string_count % 8) > 0)
			usb_page_count++;
	} else {
		perror ("");
		mlc_only = 1;
	}

	for (u32 i=0;i<usb_folder_string_count;i++) {
		if (usb_stored_folders[i] == usb_folders[i])
			getUsbMetaData = 0;
		else
			usb_stored_folders[i] = usb_folders[i];
	}

	int usb_checkBox[usb_folder_string_count];

	if (!mlc_only) 
	{
		for(u32 i = 0; i < usb_folder_string_count; i++)
			mlc_checkBox[i] = 0;
	
		usb_meta_names = (char**)malloc(usb_folder_string_count*sizeof(char*));
		if (!usb_meta_names) {
			return 0;
		}

		// Get metadata
		if (getUsbMetaData)
		{
			getUsbMetaData = 0;
			for (u32 i = 0; i < usb_folder_string_count; i++) {
				console_printf(1, "Fetching meta for %s [USB %d/%d]", usb_folders[i], i + 1, usb_folder_string_count);

				char xml_path[28];
				snprintf(xml_path, sizeof(xml_path), "dev:/%s/meta/meta.xml", usb_folders[i]);
				
				FILE *fp;
				mxml_node_t *tree;

				fp = fopen(xml_path, "r");
				tree = mxmlLoadFile(NULL, fp, MXML_OPAQUE_CALLBACK);

				char* meta_xml_getname(mxml_node_t *tree) {
					mxml_node_t* value = mxmlFindPath(tree, "menu/shortname_en");
					const char* str =  mxmlGetOpaque(value);
					char* name = (char*)malloc(strlen(str) + 1);
					strcpy(name, str);
					mxmlDelete(value);
					return name;
				}

				char *get_name = meta_xml_getname(tree);

				if (get_name == NULL)
				{
					usb_meta_names[i] = usb_folders[i];
					console_printf(1, "Failed to get meta\n");
				}
				else
				{
					usb_meta_names[i] = get_name;
					console_printf(1, "%s", get_name);
				}

				mxmlDelete(tree);
				fclose(fp);
			}
		}

		for(u32 i = 0; i < usb_folder_string_count; i++)
			usb_checkBox[i] = 0;
	}

	unmount_fs("dev");

	IOSUHAX_FSA_Close(fsaFd);
	if(mcp_hook_fd >= 0)
		MCPHookClose();
	else
		IOSUHAX_Close();

	while(1)
	{
		VPADRead(0, &vpad, 1, &vpadError);

		if (mlc_only)
			dump_source = 0;

		// Page checks
		if (page_number < 1)
			page_number = (dump_source) ? usb_page_count : mlc_page_count;
		if (page_number > ((dump_source) ? usb_page_count : mlc_page_count))
		{
			page_number = 1;
			selectedItem = 0;
		}

		// selectedItem checks
		if(selectedItem < 0)
			selectedItem = ((dump_source) ? usb_folder_string_count : mlc_folder_string_count) - 1;

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

			console_print_pos(0, 1, head_string);
			console_print_pos(0, 2, "Source: %s // Target: %s // Page %d of %d", (dump_source) ? "USB" : "System", (dump_target) ? "USB" : "SD", page_number, (dump_source) ? usb_page_count : mlc_page_count);

			console_print_pos(0, 4, "Press A to select and START to begin dumping.");
			if (mlc_only)
            	console_print_pos(0, 5, "Use ZL/ZR to change target.");
			else
            	console_print_pos(0, 5, "Use L/R to change source and ZL/ZR to change target.");

			for(u32 i = 0; i < ((dump_source) ? usb_page_count : mlc_page_count); i++) {
				if (i == page_number - 1)
				{
					u32 onscreen_count = 8;
					if (i == ((dump_source) ? usb_page_count : mlc_page_count) - 1)
						onscreen_count = ((dump_source) ? usb_folder_string_count : mlc_folder_string_count) % 8;
					for(u32 j = 0 + (i*8); j < onscreen_count + (i*8); j++)
					{
						int ypos = j % 8;
						if ((dump_source) ? usb_checkBox[j] : mlc_checkBox[j])
							console_print_pos(0, 7 + ypos, "[x]");
						else
							console_print_pos(0, 7 + ypos, "[ ]");

						if (selectedItem == (int)j)
							console_print_pos(4, 7 + ypos, "> %s", (dump_source) ? usb_meta_names[j] : mlc_meta_names[j]);
						else
							console_print_pos(4, 7 + ypos, "  %s", (dump_source) ? usb_meta_names[j] : mlc_meta_names[j]);
					}
				}
			}

			console_print_pos(0, 16, "Hold B to cancel. Press X to return to main menu.");

			// Flip buffers
			OSScreenFlipBuffersEx(0);
			OSScreenFlipBuffersEx(1);
		}

		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_DOWN))
		{
			// if item is the last entry on page
			if (selectedItem % 8 == 7)
				page_number++;
			// if page is the last page and there are less than 8 entries
			u32 folder_count = (dump_source) ? usb_page_count : mlc_page_count;
			int string_count = (dump_source) ? usb_folder_string_count : mlc_folder_string_count;
			if ((page_number == folder_count) && (string_count % 8 > 0))
				// if item is equal to the amount of strings (aka if it's the last one)
				if (selectedItem == (string_count - 1))
					page_number++;
			selectedItem++;
			initScreen = 1;
			usleep(150000);
		}

		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_UP))
		{
			if (selectedItem % 8 == 0)
				page_number--;
			selectedItem--;
			initScreen = 1;
			usleep(150000);
		}

		if(!mlc_only && vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_R | VPAD_BUTTON_L)))
		{
			u32 i;
			for(i = 0; i < ((dump_source) ? usb_folder_string_count : mlc_folder_string_count); i++) {
				if (dump_source)
					usb_checkBox[i] = 0;
				else
					mlc_checkBox[i] = 0;
			}
			dump_source = !dump_source;
			selectedItem = 0;
			initScreen = 1;
			usleep(150000);
		}

		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_LEFT)))
		{
			int string_count = (dump_source) ? usb_folder_string_count : mlc_folder_string_count;
			if (selectedItem < 8)
			{
				selectedItem = string_count - (string_count % 8) + selectedItem;
				if (selectedItem > string_count - 1)
					selectedItem = string_count - 1;
			}
			else
				selectedItem -= 8;
			page_number--;
			initScreen = 1;
			usleep(150000);
		}

		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_RIGHT)))
		{
			int string_count = (dump_source) ? usb_folder_string_count : mlc_folder_string_count;
			if (selectedItem + 8 > string_count - 1)
				selectedItem = string_count - 1;
			else
				selectedItem += 8;
			page_number++;
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
			if (dump_source)
				usb_checkBox[selectedItem] = !usb_checkBox[selectedItem];
			else
				mlc_checkBox[selectedItem] = !mlc_checkBox[selectedItem];
			initScreen = 1;
			usleep(150000);
		}

		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_X))
		{
			return 1;
			usleep(150000);
		}

		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_PLUS))
		{
			for(u32 i = 0; i < ((dump_source) ? usb_folder_string_count : mlc_folder_string_count); i++)
			{
				if ((dump_source) ? usb_checkBox[i] : mlc_checkBox[i]) {
					char selection_path[255];
					char dir[255];

					// Dump game
					snprintf(selection_path, sizeof(selection_path), "storage_%s01:/usr/title/00050000/%s", (dump_source) ? "usb" : "mlc", ((dump_source) ? usb_folders[i] : mlc_folders[i]));
					snprintf(dir, sizeof(dir), "games/%s", ((dump_source) ? usb_meta_names[i] : mlc_meta_names[i]));
					dumpFunc(selection_path, dir, 0, dump_source, dump_target, 0);
					 
					// Dump update
					snprintf(selection_path, sizeof(selection_path), "storage_%s01:/usr/title/0005000E/%s", (dump_source) ? "usb" : "mlc", ((dump_source) ? usb_folders[i] : mlc_folders[i]));
					snprintf(dir, sizeof(dir), "updates/%s", ((dump_source) ? usb_meta_names[i] : mlc_meta_names[i]));
					dumpFunc(selection_path, dir, 0, dump_source, dump_target, 1);
					 
					// Dump update
					snprintf(selection_path, sizeof(selection_path), "storage_%s01:/usr/title/0005000C/%s", (dump_source) ? "usb" : "mlc", ((dump_source) ? usb_folders[i] : mlc_folders[i]));
					snprintf(dir, sizeof(dir), "dlc/%s", ((dump_source) ? usb_meta_names[i] : mlc_meta_names[i]));
					dumpFunc(selection_path, dir, 0, dump_source, dump_target, 1);
					 
					// Saves
					snprintf(selection_path, sizeof(selection_path), "storage_%s01:/usr/save/00050000/%s", (dump_source) ? "usb" : "mlc", ((dump_source) ? usb_folders[i] : mlc_folders[i]));
					snprintf(dir, sizeof(dir), "saves/%s", ((dump_source) ? usb_meta_names[i] : mlc_meta_names[i]));
					dumpFunc(selection_path, dir, 0, dump_source, dump_target, 1);
				}
				initScreen = 1;
			}
		}

		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_HOME))
		{
			loop = 0;
			return 1;
		}

	usleep(50000);
	}
}

/* Entry point */
int Menu_Main(void)
{
    InitOSFunctionPointers();
    InitSocketFunctionPointers();

    InitFSFunctionPointers();
    InitVPadFunctionPointers();

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
	int mlc_only = 0;

	mcp_hook_fd = -1;
	int res = IOSUHAX_Open(NULL);
	if(res < 0)
        res = MCPHookOpen();
	if(res < 0)
	{
		console_printf(2, "IOSUHAX_open failed");
		console_printf(1, "Ensure you are using CFW");
		sleep(3);
		return 0;
	}

	int fsaFd = IOSUHAX_FSA_Open();
	if(fsaFd < 0)
	{
		console_printf(2, "IOSUHAX_FSA_Open failed");
		console_printf(1, "Run CFW again");
		sleep(3);
		return 0;
	}

	mount_fs("dev", fsaFd, NULL, "/vol/storage_usb01/");

	if (!ifDirExists("dev:/"))
		mlc_only = 1;

	unmount_fs("dev");

	IOSUHAX_FSA_Close(fsaFd);
	if(mcp_hook_fd >= 0)
		MCPHookClose();
	else
		IOSUHAX_Close();

    static const char* mlc_selection_text[] =
    {
        "Account Data",
        "Friends List",
        "Disc dump",
        "All digital games",
        "All digital updates",
        "All digital DLC",
        "All save data",
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

    static const char* mlc_selection_dir[] =
    {
        "online_files/mlc01",
        "friends_list",
        "disc",
        "games",
        "updates",
        "dlc",
        "saves",
        "nand",
    };

    static const char* usb_selection_text[] =
    {
        "All digital games",
        "All digital updates",
        "All digital DLC",
        "All save data",
    };

    static const char* usb_selection_paths[] =
    {
        "storage_usb:/usr/title/00050000",
        "storage_usb:/usr/title/0005000E",
        "storage_usb:/usr/title/0005000C",
        "storage_usb:/usr/save/00050000",
    };

    static const char* usb_selection_dir[] =
    {
        "games",
        "updates",
        "dlc",
        "saves",
    };

    static const char* online_files_paths[] =
    {
		"storage_mlc:/usr/save/system/act",
		"storage_mlc:/sys/title/0005001b/10054000/",
		"storage_mlc:/sys/title/0005001b/10056000",
    };

    int selectedItem = 0;
	int dump_source = 0;
	int dump_target = 0;

	int checkBox[(((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4)];

	u32 i;
	for(i = 0; i < (((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4); i++) {
		checkBox[i] = 0;
	}

    while(loop)
    {
        VPADRead(0, &vpad, 1, &vpadError);

		if (mlc_only)
			dump_source = 0;

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

            console_print_pos(0, 1, head_string);
            console_print_pos(0, 2, "Source: %s // Target: %s", (dump_source) ? "USB" : "System", (dump_target) ? "USB" : "SD");

            console_print_pos(0, 4, "Press A to select and START to begin dumping.");
			if (mlc_only)
            	console_print_pos(0, 5, "Use ZL/ZR to change target.");
			else
            	console_print_pos(0, 5, "Use L/R to change source and ZL/ZR to change target.");

            u32 i;

            for(i = 0; i < (((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4); i++)
            {
				if (checkBox[i])
					console_print_pos(0, 7 + i, "[x]");
				else
					console_print_pos(0, 7 + i, "[ ]");

                if (selectedItem == (int)i)
					console_print_pos(4, 7 + i, "> %s", (dump_source) ? usb_selection_text[i] : mlc_selection_text[i]);
                else
					console_print_pos(4, 7 + i, "  %s", (dump_source) ? usb_selection_text[i] : mlc_selection_text[i]);
            }

            console_print_pos(0, 16, "Hold B to cancel. Press X to dump individual titles.");

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

        if(!mlc_only && vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & (VPAD_BUTTON_R | VPAD_BUTTON_RIGHT | VPAD_BUTTON_L | VPAD_BUTTON_LEFT)))
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
			titles_menu(dump_source, dump_target);
			initScreen = 1;
			usleep(150000);
        }

        if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_PLUS))
        {
			u32 i, j;
			for(i = 0; i < (((dump_source) ? sizeof(usb_selection_text) : sizeof(mlc_selection_text)) / 4); i++)
			{
				int dev = 0;
				if (checkBox[i]) {
					if (!dump_source && (i < 2)) {
						if (i == 0)
							for (j=0;j<sizeof(online_files_paths)/4;j++)
								dumpFunc(online_files_paths[j], (dump_source) ? usb_selection_dir[i] : mlc_selection_dir[i], dev, dump_source, dump_target, 0);
						if (i == 1) 
						{
							for (j=0;j<3;j++)
							{
								int doDump = 0;
								char friends_list_path[29];
								snprintf(friends_list_path, sizeof(friends_list_path), "/sys/title/00050030/10015%d0A", j);

								mcp_hook_fd = -1;
								res = IOSUHAX_Open(NULL);
								if(res < 0)
        							res = MCPHookOpen();
								fsaFd = IOSUHAX_FSA_Open();

								char mountPath[255];
								snprintf(mountPath, sizeof(mountPath), "/vol/storage_mlc01%s", friends_list_path);

								mount_fs("dev", fsaFd, NULL, mountPath);
								VirtualMountDevice("dev:/");

								if (ifDirExists("dev:/"))
									doDump = 1;

								unmount_fs("dev");
								UnmountVirtualPaths();
								IOSUHAX_FSA_Close(fsaFd);
								if(mcp_hook_fd >= 0)
									MCPHookClose();
								else
									IOSUHAX_Close();

								char dumpPath[255];
								snprintf(dumpPath, sizeof(mountPath), "storage_mlc:%s", friends_list_path);

								if (doDump)
								{
									char selection_dir[16];
									if (j == 0) strcpy(selection_dir, "friends_list_jp");
									if (j == 1) strcpy(selection_dir, "friends_list_us");
									if (j == 2) strcpy(selection_dir, "friends_list_eu");
									dumpFunc(dumpPath, selection_dir, dev, dump_source, dump_target, 0);
								}
							}
						}
					}
					else
					{
						if (!dump_source && (i == 2)) dev = 1;
						dumpFunc((dump_source) ? usb_selection_paths[i] : mlc_selection_paths[i], (dump_source) ? usb_selection_dir[i] : mlc_selection_dir[i], dev, dump_source, dump_target, 0);
					}
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
