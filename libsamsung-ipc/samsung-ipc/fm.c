/**
 * This file is part of libsamsung-ipc.
 *
 * Copyright (C) 2011-2012 KB <kbjetdroid@gmail.com>
 *
 * Implemented as per the Mocha AP-CP protocol analysis done by Dominik Marszk
 *
 * libsamsung-ipc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsamsung-ipc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libsamsung-ipc.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * TODO: Optimize this file
 * 		 Implement following functions properly:
 * 		 	FmMoveFile
 * 		 	FmGetFileAttrFile
 * 		 	FmFGetFileAttrFile
 * 		 	FmSetFileAttrFile
 * 		 	FmTruncateFile
 * 		 	FmReadDirFile
 * 		 	FmRemoveDirFile
 * 		 	FmGetQuotaSpaceFile
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <getopt.h>

#include <fm.h>
#include <radio.h>
#include <dirent.h>
#include <errno.h>

#define LOG_TAG "RIL_FM"
#include <utils/Log.h>

#define MAX_OPEN_DIRS 	10

DIR* dirArray[MAX_OPEN_DIRS];
uint32_t dirIndex = 1;

#if defined(DEVICE_JET)
char *mochaRoot = "/KFAT0";
#elif defined(DEVICE_WAVE)
char *mochaRoot = "/bada_usr/modem/";
#endif

int32_t (*fileOps[MAX_FILE_OPS])(struct fmRequest *, struct fmResponse *) =
{
	&FmOpenFile,
	&FmCloseFile,
	&FmCreateFile,
	&FmReadFile,
	&FmWriteFile,
	&FmFlushFile,
	&FmSeekFile,
	&FmTellFile,
	&FmRemoveFile,
	&FmMoveFile,
	&FmGetFileAttrFile,
	&FmFGetFileAttrFile,
	&FmSetFileAttrFile,
	&FmTruncateFile,
	&FmOpenDirFile,
	&FmCloseDirFile,
	&FmReadDirFile,
	&FmCreateDirFile,
	&FmRemoveDirFile,
	&FmGetQuotaSpaceFile
};

int32_t FmOpenFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
//	DEBUG_I("Inside FmOpenFile");
	int32_t retval = 0;
	char *fName;
	int32_t mode;
	uint32_t flags = O_RDONLY;

	mode = *(int32_t *)(rx_packet->reqBuf);
	fName = (char *)malloc(sizeof(*mochaRoot) + strlen((char *)(rx_packet->reqBuf) + sizeof(mode)));
	strcpy(fName, mochaRoot);
	strcat(fName, (const char *)(rx_packet->reqBuf + sizeof(mode)));
//	DEBUG_I("fName %s, mode = 0x%x", fName, mode);

	/*if (!strcmp(fName, "/KFAT0/nvm/num/87_19"))
		lastOpen = 1;
*/
	if(mode & FM_CREATE)
		flags |= O_CREAT;
	if(mode & FM_WRITE)
		flags |= O_RDWR;
	if(mode & FM_TRUNCATE)
		flags |= O_TRUNC;
	if(mode & FM_APPEND)
		flags |= O_APPEND;
#if 0
	else if(mode & FM_NOSHARE)
		flags |= O_RDWR;
	else if(mode & FM_DIRECTIO)
		flags |= O_RDWR;
	else if(mode & FM_CONCRETE_WRITE)
		flags |= O_RDWR;
	else if(mode & FM_NOUPDATE_TIME)
		flags |= O_RDWR;
#endif
	retval = open(fName, flags); //0777);
	free(fName);

	tx_packet->funcRet = retval; //-1; //retval;
	tx_packet->errorVal = (retval < 0 ? errno : 0); //-1; //retval;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet); //0x08; //0x100;
	tx_packet->respBuf = NULL;

//	DEBUG_I("Leaving FmOpenFile fd = %d", retval);
	return 0;
}

int32_t FmCloseFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
//	DEBUG_I("Inside FmCloseFile");
	int32_t retval = 0;
	int32_t fd;

	fd = *(int32_t *)(rx_packet->reqBuf);

	retval = close(fd);

	tx_packet->errorVal = (retval < 0 ? errno : 0); //0; //retval;
	tx_packet->funcRet = retval; //0; //retval;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet);
	tx_packet->respBuf = NULL;

//	DEBUG_I("Leaving FmCloseFile fd = %d", fd);
	return 0;
}

int32_t FmCreateFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmCreateFile");
	int32_t retval = 0;
	struct stat sb;
	char *fName;

	fName = (char *)malloc(sizeof(*mochaRoot) + strlen((char *)rx_packet->reqBuf));
	strcpy(fName, mochaRoot);
	strcat(fName, (const char *)(rx_packet->reqBuf));
	DEBUG_I("fName %s", fName);

	retval = creat(fName, 0777);
	free(fName);

	tx_packet->errorVal = (retval < 0 ? errno : 0); //0; //retval;
	tx_packet->funcRet = retval; //0; //retval;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet); //0x08; //0x100;
	tx_packet->respBuf = NULL;

	DEBUG_I("Leaving FmCreateFile");
	return 0;
}

int32_t FmReadFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
//	DEBUG_I("Inside FmReadFile");
	int32_t retval = 0;
	int32_t fd;
	uint32_t size, numRead;
	uint8_t *readBuf;

	fd = *(int32_t *)(rx_packet->reqBuf);
	size = *(int32_t *)((rx_packet->reqBuf) + sizeof(fd));

	readBuf = (uint8_t *)malloc(size + sizeof(numRead));

	numRead = read(fd, (readBuf+ sizeof(numRead)), size);

	memcpy(readBuf, &numRead, sizeof(numRead));

	tx_packet->errorVal = ((int32_t)numRead < 0 ? errno : 0); //0; //retval;
	tx_packet->funcRet = numRead; //0; //retval;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet) + numRead; //0x08; //0x100;
	tx_packet->respBuf = readBuf;

//	DEBUG_I("Leaving FmReadFile");
	return 0;
}

int32_t FmWriteFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
//	DEBUG_I("Inside FmWriteFile");
	int32_t retval = 0;
	int32_t fd;
	uint32_t size, numWrite;
	uint8_t *writeBuf;

	fd = *(int32_t *)(rx_packet->reqBuf);
	size = *(int32_t *)((rx_packet->reqBuf) + sizeof(fd));

	writeBuf = (uint8_t *)((rx_packet->reqBuf) + sizeof(fd) + sizeof(size));

	numWrite = write(fd, writeBuf, size);

	tx_packet->errorVal = ((int32_t) numWrite < 0 ? errno : 0); //0; //retval;
	tx_packet->funcRet = numWrite; //0; //retval;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet); //0x08; //0x100;
	tx_packet->respBuf = NULL;

//	DEBUG_I("Leaving FmWriteFile");
	return 0;
}

int32_t FmFlushFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmFlushFile");
	int32_t retval = 0;
	int32_t fd;

	fd = *(int32_t *)(rx_packet->reqBuf);

	retval = fsync(fd);

	tx_packet->errorVal = 0; //retval;
	tx_packet->funcRet = retval; //0; //retval;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet);
	tx_packet->respBuf = NULL;

	DEBUG_I("Leaving FmFlushFile");
	return 0;
}

int32_t FmSeekFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmSeekFile");
	int32_t retval = 0;
	int32_t fd;
	uint32_t offset, origin;

	fd = *(int32_t *)(rx_packet->reqBuf);
	offset = *(int32_t *)((rx_packet->reqBuf) + sizeof(fd));
	origin = *(int32_t *)((rx_packet->reqBuf) + sizeof(fd) + sizeof(offset));

	DEBUG_I("Inside FmSeekFile fd = %d, offset = 0x%x, origin = 0x%x", fd, offset, origin);

	retval = lseek(fd, offset, origin);

	tx_packet->errorVal = 0; //retval;
	tx_packet->funcRet = 0; //retval;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet); //0x08; //0x100;
	tx_packet->respBuf = NULL;

	DEBUG_I("Leaving FmSeekFile fd = %d", fd);
	return 0;
}

int32_t FmTellFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmTellFile");
	int32_t retval = 0;
	int32_t fd;

	fd = *(int32_t *)(rx_packet->reqBuf);

	retval = lseek(fd, 0, SEEK_CUR); //ftell(fd);

	tx_packet->errorVal = 0; //retval;
	tx_packet->funcRet = retval; //0; //retval;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet);
	tx_packet->respBuf = NULL;

	DEBUG_I("Leaving FmTellFile fd = %d", fd);
	return 0;
}

int32_t FmRemoveFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	//DEBUG_I("Inside FmRemoveFile");
	int32_t retval = 0;
	char *fName;

	fName = (char *)malloc(sizeof(*mochaRoot) + strlen((char *)rx_packet->reqBuf));
	strcpy(fName, mochaRoot);
	strcat(fName, (const char *)(rx_packet->reqBuf));
//	DEBUG_I("fName %s", fName);

	retval = remove(fName);
	free(fName);

	tx_packet->errorVal = 0; //retval;
	tx_packet->funcRet = 0; //retval;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet); //0x08; //0x100;
	tx_packet->respBuf = NULL;

//	DEBUG_I("Leaving FmRemoveFile");
	return retval;

}

int32_t FmMoveFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmMoveFile");

	DEBUG_I("Leaving FmMoveFile");
	return 0;
}

/*
 * FIXME: Put proper timestamp in FileAttribute structure
 */
FmFileAttribute *fAttr;
TmDateTime fmTime = {
		.year = 2011,
		.month = 12,
		.day = 29,
		.hour = 10,
		.minute = 45,
		.second = 45,
};

int32_t FmGetFileAttrFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
//	DEBUG_I("Inside FmGetFileAttrFile");
	int32_t retval = 0;
	struct stat sb;
	char *fName;

	fName = (char *)malloc(sizeof(*mochaRoot) + strlen((char *)rx_packet->reqBuf));
	strcpy(fName, mochaRoot);
	strcat(fName, (const char *)(rx_packet->reqBuf));
//	DEBUG_I("fName %s", fName);

	retval = stat(fName, &sb);
	free(fName);

	fAttr = (FmFileAttribute *)malloc(sizeof(FmFileAttribute));

	tx_packet->funcRet = retval; //(retval < 0 ? 0 : 1); //0;
	tx_packet->errorVal = (retval < 0 ? errno : 0); // ENOENT; retval; //0;
	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet) + sizeof(FmFileAttribute); //0x08; //0x100;

#if 1
	if(retval >= 0)
	{
#if 1
		fAttr->oldFileSize = sb.st_size;
		fAttr->startAddr = 0;
		fAttr->attribute = sb.st_mode;
		fAttr->iVol = sb.st_dev;
		fAttr->dt.year = fmTime.year;
		fAttr->dt.month = fmTime.month;
		fAttr->dt.day = fmTime.day;
		fAttr->dt.hour = fmTime.hour;
		fAttr->dt.minute = fmTime.minute;
		fAttr->dt.second = fmTime.second;
		fAttr->oldAllocatedSize = sb.st_size;
		fAttr->stModifiedDataTime.year = fmTime.year;
		fAttr->stModifiedDataTime.month = fmTime.month;
		fAttr->stModifiedDataTime.day = fmTime.day;
		fAttr->stModifiedDataTime.hour = fmTime.hour;
		fAttr->stModifiedDataTime.minute = fmTime.minute;
		fAttr->stModifiedDataTime.second = fmTime.second;
		fAttr->u64EntryUniqID = sb.st_ino;
		fAttr->uReservedField = 0;
#endif
		fAttr->fileSize = sb.st_size;
		fAttr->allocatedSize = sb.st_size;
	}
	else
	{
#if 1
		fAttr->oldFileSize = 0;
		fAttr->startAddr = 0;
		fAttr->attribute = 0;
		fAttr->iVol = 0;
		fAttr->dt.year = fmTime.year;
		fAttr->dt.month = fmTime.month;
		fAttr->dt.day = fmTime.day;
		fAttr->dt.hour = fmTime.hour;
		fAttr->dt.minute = fmTime.minute;
		fAttr->dt.second = fmTime.second;
		fAttr->oldAllocatedSize = 0;
		fAttr->stModifiedDataTime.year = fmTime.year;
		fAttr->stModifiedDataTime.month = fmTime.month;
		fAttr->stModifiedDataTime.day = fmTime.day;
		fAttr->stModifiedDataTime.hour = fmTime.hour;
		fAttr->stModifiedDataTime.minute = fmTime.minute;
		fAttr->stModifiedDataTime.second = fmTime.second;
		fAttr->u64EntryUniqID = 0;
		fAttr->uReservedField = 0;
#endif
		fAttr->fileSize = 0;
		fAttr->allocatedSize = 0;
	}
#endif

	//memcpy(tx_packet->respBuf, &fAttr, sizeof(FmFileAttribute));
	tx_packet->respBuf = (uint8_t *)fAttr;

//	DEBUG_I("Leaving FmGetFileAttrFile");
	return 0;
}

int32_t FmFGetFileAttrFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmFGetFileAttrFile");
	int32_t retval = 0;
	struct stat sb;
	int32_t fd;

	fd = *(int32_t *)(rx_packet->reqBuf);

	retval = fstat(fd, &sb);

	fAttr = malloc(sizeof(FmFileAttribute));

	tx_packet->funcRet = (retval < 0 ? 0 : 1); //0;
	tx_packet->errorVal = (retval < 0 ? errno : 0); // ENOENT; retval; //0;
	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet) + sizeof(FmFileAttribute); //0x08; //0x100;

	if(retval >= 0)
	{
		fAttr->oldFileSize = sb.st_size;
		fAttr->startAddr = 0;
		fAttr->attribute = sb.st_mode;
		fAttr->iVol = sb.st_dev;
		fAttr->dt.year = fmTime.year;
		fAttr->dt.month = fmTime.month;
		fAttr->dt.day = fmTime.day;
		fAttr->dt.hour = fmTime.hour;
		fAttr->dt.minute = fmTime.minute;
		fAttr->dt.second = fmTime.second;
		fAttr->oldAllocatedSize = sb.st_size;
		fAttr->stModifiedDataTime.year = fmTime.year;
		fAttr->stModifiedDataTime.month = fmTime.month;
		fAttr->stModifiedDataTime.day = fmTime.day;
		fAttr->stModifiedDataTime.hour = fmTime.hour;
		fAttr->stModifiedDataTime.minute = fmTime.minute;
		fAttr->stModifiedDataTime.second = fmTime.second;
		fAttr->u64EntryUniqID = sb.st_ino;
		fAttr->uReservedField = 0;
		fAttr->fileSize = sb.st_size;
		fAttr->allocatedSize = sb.st_size;
	}
	else
	{
		fAttr->oldFileSize = 0;
		fAttr->startAddr = 0;
		fAttr->attribute = 0;
		fAttr->iVol = 0;
		fAttr->dt.year = fmTime.year;
		fAttr->dt.month = fmTime.month;
		fAttr->dt.day = fmTime.day;
		fAttr->dt.hour = fmTime.hour;
		fAttr->dt.minute = fmTime.minute;
		fAttr->dt.second = fmTime.second;
		fAttr->oldAllocatedSize = 0;
		fAttr->stModifiedDataTime.year = fmTime.year;
		fAttr->stModifiedDataTime.month = fmTime.month;
		fAttr->stModifiedDataTime.day = fmTime.day;
		fAttr->stModifiedDataTime.hour = fmTime.hour;
		fAttr->stModifiedDataTime.minute = fmTime.minute;
		fAttr->stModifiedDataTime.second = fmTime.second;
		fAttr->u64EntryUniqID = 0;
		fAttr->uReservedField = 0;
		fAttr->fileSize = 0;
		fAttr->allocatedSize = 0;
	}

	//memcpy(tx_packet->respBuf, &fAttr, sizeof(FmFileAttribute));
	tx_packet->respBuf = (uint8_t *)fAttr;

	return retval;
}

int32_t FmSetFileAttrFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmSetFileAttrFile - TBD");

	DEBUG_I("Leaving FmSetFileAttrFile");
	return 0;
}

int32_t FmTruncateFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmTruncateFile - TBD");

	DEBUG_I("Leaving FmTruncateFile");
	return 0;
}

int32_t FmOpenDirFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmOpenDirFile");
	int32_t retval = 0;
	DIR * dir;
	uint8_t *payload;
	char *fName;

	fName = (char *)malloc(sizeof(*mochaRoot) + strlen((char *)rx_packet->reqBuf));
	strcpy(fName, mochaRoot);
	strcat(fName, (const char *)(rx_packet->reqBuf));
	DEBUG_I("fName %s", fName);
	dir = opendir(fName);
	free(fName);

	dirArray[dirIndex++] = dir;

	tx_packet->errorVal = 0;
	tx_packet->funcRet = 1;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet); //0x08; //0x100;
	tx_packet->respBuf = NULL;

	DEBUG_I("Leaving FmOpenDirFile");
	return 0;
}

int32_t FmCloseDirFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmCloseDirFile");

	int32_t retval = 0;
	struct dirent * dir;
	uint8_t *payload;
	int32_t fd;

	fd = *(int32_t *)(rx_packet->reqBuf);
//	dir = closedir((DIR *)dirArray[fd]);

	tx_packet->errorVal = 0;
	tx_packet->funcRet = 0;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet); //0x08; //0x100;
	tx_packet->respBuf = NULL;

	DEBUG_I("Leaving FmCloseDirFile");
	return 0;
}

int32_t FmReadDirFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmReadDirFile");
	int32_t retval = 0;
	struct dirent * dir;
	int32_t fd;

	fd = *(int32_t *)(rx_packet->reqBuf);
	//dir = readdir((DIR *)dirArray[fd]);

	// TODO :
	// What to return if the directory is not empty?

	tx_packet->errorVal = 0;
	tx_packet->funcRet = 0;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet); //0x08; //0x100;
	tx_packet->respBuf = NULL;

	DEBUG_I("Leaving FmReadDirFile");
	return 0;
}

int32_t FmCreateDirFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmCreateDirFile");
	int32_t retval = 0;
	char *fName;

	fName = (char *)malloc(sizeof(*mochaRoot) + strlen((char *)rx_packet->reqBuf));
	strcpy(fName, mochaRoot);
	strcat(fName, (const char *)(rx_packet->reqBuf));
	DEBUG_I("fName %s", fName);

	retval = mkdir(fName, 0777);

	if(!retval)
		DEBUG_I("error creating directory %s", fName);
	free(fName);
	tx_packet->errorVal = 0;
	tx_packet->funcRet = 0;

	tx_packet->header->packetLen = sizeof(tx_packet->errorVal) + sizeof(tx_packet->funcRet); //0x08; //0x100;
	tx_packet->respBuf = NULL;

	DEBUG_I("Leaving FmCreateDirFile");
	return retval;
}

int32_t FmRemoveDirFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmRemoveDirFile");

	DEBUG_I("Leaving FmRemoveDirFile");
	return 0;
}

int32_t FmGetQuotaSpaceFile(struct fmRequest *rx_packet, struct fmResponse *tx_packet)
{
	DEBUG_I("Inside FmGetQuotaSpaceFile");

	DEBUG_I("Leaving FmGetQuotaSpaceFile");
	return 0;
}

int32_t get_request_packet(void *data, struct fmRequest *rx_packet)
{
	struct fmPacketHeader *header;

	header = (struct fmPacketHeader *)data;
	rx_packet->header = header;
	rx_packet->reqBuf = (uint8_t *)((uint8_t *)data + sizeof(struct fmPacketHeader));

//	DEBUG_I("Packet type = 0x%x", header->fmPacketType);
//	DEBUG_I("Packet Length = 0x%x", header->packetLen);
	DEBUG_I("Packet counter = 0x%x", header->reqCounter);
//	DEBUG_I("File name = %s", (char *)(rx_packet->reqBuf));

	return 0;

}

int32_t ipc_parse_fm(struct ipc_client* client, struct modem_io *ipc_frame)
{
	//DEBUG_I("Entering ipc_parse_fm");

	int32_t retval;
	struct fmRequest rx_packet;
	struct fmResponse tx_packet;
	struct fmArgs args;
	struct modem_io request;
	uint8_t *frame;
	uint8_t *payload;
	int32_t frame_length;

	get_request_packet(ipc_frame->data, &rx_packet);

	tx_packet.header = rx_packet.header;
	retval = fileOps[(tx_packet.header->fmPacketType + 0xEFFFFFFF)](&rx_packet, &tx_packet);

    frame_length = (sizeof(struct fmPacketHeader) + tx_packet.header->packetLen);

	request.magic = ipc_frame->magic;
	request.cmd = ipc_frame->cmd;
	request.datasize = frame_length;


    frame = (uint8_t*)malloc(frame_length);

    memcpy(frame, tx_packet.header, sizeof(struct fmPacketHeader));

    payload = (frame + sizeof(struct fmPacketHeader));

    memcpy(payload, &tx_packet.funcRet, sizeof(tx_packet.funcRet));

    payload = (payload + sizeof(tx_packet.funcRet));

    memcpy(payload, &tx_packet.errorVal, sizeof(tx_packet.errorVal));

    payload = (payload + sizeof(tx_packet.errorVal));

    memcpy(payload, tx_packet.respBuf, tx_packet.header->packetLen - (sizeof(tx_packet.errorVal) + sizeof(tx_packet.funcRet)));

	request.data = frame;

	ipc_send(&request);

	if(tx_packet.respBuf != NULL)
        free(tx_packet.respBuf);

    if(frame != NULL)
        free(frame);

    return 0;
}