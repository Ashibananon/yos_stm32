/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

/* Example: Declarations of the platform and disk functions in the project */
//#include "platform.h"
//#include "storage.h"
#include <stdio.h>
#include "../../../src/ydevice/spi_sdcard/yspi_sdcard.h"

/* Example: Mapping of physical drive number for each drive */
#define DEV_FLASH	0	/* Map FTL to physical drive 0 */
#define DEV_MMC		1	/* Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Map USB MSD to physical drive 2 */

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = STA_NOINIT;
	int result;

	switch (pdrv) {
	case DEV_FLASH :
		result = ysdcard_is_ready();
		if (result) {
			stat = 0;
		}

		break;
	case DEV_MMC :
	case DEV_USB :
	default :
		break;
	}

	return stat;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = STA_NOINIT;
	int result;

	switch (pdrv) {
	case DEV_FLASH :
		result = ysdcard_init();
		if (result == 0) {
			stat = 0;
		}

		break;
	case DEV_MMC :
	case DEV_USB :
	default :
		break;
	}

	return stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res = RES_PARERR;
	int result;
	struct ysdcard_info _ysdinfo;

	switch (pdrv) {
	case DEV_FLASH :
		// translate the arguments here
		if (ysdcard_get_sdcard_info(&_ysdinfo) == 0) {
			result = ysdcard_read(sector, 0, buff, _ysdinfo.sector_size * count);
			if (result == 0) {
				res = RES_OK;
			} else {
				res = RES_ERROR;
			}
		} else {
			res = RES_NOTRDY;
		}

		break;
	case DEV_MMC :
	case DEV_USB :
	default :
		break;
	}

	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res = RES_PARERR;
	int result;
	struct ysdcard_info _ysdinfo;

	switch (pdrv) {
	case DEV_FLASH :
		// translate the arguments here
		if (ysdcard_get_sdcard_info(&_ysdinfo) == 0) {
			result = ysdcard_write(sector, 0, buff, _ysdinfo.sector_size * count);
			if (result == 0) {
				res = RES_OK;
			} else {
				res = RES_ERROR;
			}
		} else {
			res = RES_NOTRDY;
		}

		break;
	case DEV_MMC :
	case DEV_USB :
		break;
	}

	return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_PARERR;
	int result;
	struct ysdcard_info _ysdinfo;

	switch (pdrv) {
	case DEV_FLASH :
		switch (cmd) {
		case CTRL_SYNC:
			res = RES_OK;
			break;
		case GET_SECTOR_COUNT:
			result = ysdcard_get_sdcard_info(&_ysdinfo);
			if (result == 0) {
				if (buff != NULL) {
					*((LBA_t *)buff) = _ysdinfo.sector_num;
					res = RES_OK;
				}
			} else {
				res = RES_NOTRDY;
			}

			break;
		case GET_SECTOR_SIZE:
			result = ysdcard_get_sdcard_info(&_ysdinfo);
			if (result == 0) {
				if (buff != NULL) {
					*((WORD *)buff) = _ysdinfo.sector_size;
					res = RES_OK;
				}
			} else {
				res = RES_NOTRDY;
			}

			break;
		case GET_BLOCK_SIZE:
			result = ysdcard_get_sdcard_info(&_ysdinfo);
			if (result == 0) {
				if (buff != NULL) {
					*((DWORD *)buff) = _ysdinfo.erase_block_size;
					res = RES_OK;
				}
			} else {
				res = RES_NOTRDY;
			}

			break;
		case CTRL_TRIM:
			break;
		default:
			result = -1;
			break;
		}

		break;
	case DEV_MMC :
	case DEV_USB :
	default :
		break;
	}

	return res;
}

