/**
 * @file lv_fs_fatfs.cpp
 * LVGL 9.x filesystem driver for FatFS on ESP32
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_fatfs.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define DRIVE_LETTER 'S'

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_fs_drv_t fs_drv_instance;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

extern "C" {

// Static function prototypes (LVGL 9.x API)
static void* fs_open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t* drv, void* file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br);
static lv_fs_res_t fs_write(lv_fs_drv_t* drv, void* file_p, const void* buf, uint32_t btw, uint32_t* bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t fs_tell(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p);
static void* fs_dir_open(lv_fs_drv_t* drv, const char* path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t* drv, void* dir_p, char* fn, uint32_t len);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t* drv, void* dir_p);

void lv_fs_if_init(void)
{
	memset(&fs_drv_instance, 0, sizeof(lv_fs_drv_t));
	lv_fs_drv_t* fs_drv = &fs_drv_instance;

	fs_drv->letter = DRIVE_LETTER;
	fs_drv->open_cb = fs_open;
	fs_drv->close_cb = fs_close;
	fs_drv->read_cb = fs_read;
	fs_drv->write_cb = fs_write;
	fs_drv->seek_cb = fs_seek;
	fs_drv->tell_cb = fs_tell;
	fs_drv->dir_open_cb = fs_dir_open;
	fs_drv->dir_read_cb = fs_dir_read;
	fs_drv->dir_close_cb = fs_dir_close;

	lv_fs_drv_register(fs_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void* fs_open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode)
{
	(void)drv;
	uint8_t flags = 0;

	if (mode == LV_FS_MODE_WR) flags = FA_WRITE | FA_OPEN_ALWAYS;
	else if (mode == LV_FS_MODE_RD) flags = FA_READ;
	else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;

	FIL* f = (FIL*)lv_malloc(sizeof(FIL));
	if (f == NULL) return NULL;

	FRESULT res = f_open(f, path, flags);
	if (res == FR_OK) {
		return f;
	}
	else {
		lv_free(f);
		return NULL;
	}
}

static lv_fs_res_t fs_close(lv_fs_drv_t* drv, void* file_p)
{
	(void)drv;
	f_close((FIL*)file_p);
	lv_free(file_p);
	return LV_FS_RES_OK;
}

static lv_fs_res_t fs_read(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br)
{
	(void)drv;
	FRESULT res = f_read((FIL*)file_p, buf, btr, (UINT*)br);
	return (res == FR_OK) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_write(lv_fs_drv_t* drv, void* file_p, const void* buf, uint32_t btw, uint32_t* bw)
{
	(void)drv;
	FRESULT res = f_write((FIL*)file_p, buf, btw, (UINT*)bw);
	return (res == FR_OK) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_seek(lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence)
{
	(void)drv;
	FIL* fp = (FIL*)file_p;
	FSIZE_t new_pos;
	
	switch (whence) {
		case LV_FS_SEEK_SET:
			new_pos = pos;
			break;
		case LV_FS_SEEK_CUR:
			new_pos = f_tell(fp) + pos;
			break;
		case LV_FS_SEEK_END:
			new_pos = f_size(fp) + pos;
			break;
		default:
			return LV_FS_RES_INV_PARAM;
	}
	
	FRESULT res = f_lseek(fp, new_pos);
	return (res == FR_OK) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_tell(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p)
{
	(void)drv;
	*pos_p = f_tell((FIL*)file_p);
	return LV_FS_RES_OK;
}

static void* fs_dir_open(lv_fs_drv_t* drv, const char* path)
{
	(void)drv;
	FF_DIR* d = (FF_DIR*)lv_malloc(sizeof(FF_DIR));
	if (d == NULL) return NULL;
	
	FRESULT res = f_opendir(d, path);
	if (res == FR_OK) {
		return d;
	}
	else {
		lv_free(d);
		return NULL;
	}
}

static lv_fs_res_t fs_dir_read(lv_fs_drv_t* drv, void* dir_p, char* fn, uint32_t len)
{
	(void)drv;
	(void)len;
	FRESULT res;
	FILINFO fno;
	fn[0] = '\0';

	do {
		res = f_readdir((FF_DIR*)dir_p, &fno);
		if (res != FR_OK) return LV_FS_RES_UNKNOWN;

		if (fno.fattrib & AM_DIR) {
			fn[0] = '/';
			strcpy(&fn[1], fno.fname);
		}
		else {
			strcpy(fn, fno.fname);
		}
	} while (strcmp(fn, "/.") == 0 || strcmp(fn, "/..") == 0);

	return LV_FS_RES_OK;
}

static lv_fs_res_t fs_dir_close(lv_fs_drv_t* drv, void* dir_p)
{
	(void)drv;
	f_closedir((FF_DIR*)dir_p);
	lv_free(dir_p);
	return LV_FS_RES_OK;
}

} // extern "C"
