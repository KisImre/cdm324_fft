#include "user/sd_card_logger.h"
#include "fatfs.h"
#include <stdio.h>

#define LOG_DIR "cdm324"
#define LOG_FILE_FMT "%03lu.csv"
#define LOG_FLUSH_TICK (10 * 1000) /* 10 sec */

/*! \fn     get_highest_file_index(uint32_t *index)
*   \brief  List the files of the log directory and find the log file with the highest index.
*   \param  index Pointer to the index
*   \return FR_OK on success
*/
static FRESULT get_highest_file_index(uint32_t *index)
{
	DIR dir = { 0 };
	FRESULT result = FR_DENIED;
	FILINFO file_info = { 0 };
	uint32_t index_from_filename = 0;

	*index = 0;

	result = f_opendir(&dir, LOG_DIR);
	if (result != FR_OK)
	{
		return result;
	}

	while (1) {
		result = f_readdir(&dir, &file_info);

		if (result != FR_OK || file_info.fname[0] == 0)
		{
			break;
		}

		if (file_info.fattrib & AM_DIR)
		{
			/* Skip subdirectories */
			continue;
		}

		if (sscanf(file_info.fname, LOG_FILE_FMT, &index_from_filename) != 1)
		{
			/* The filename does not follow the expected log file format. */
			continue;
		}

		if (*index < index_from_filename)
		{
			/* Store larger index */
			*index = index_from_filename;
		}

	}

	return f_closedir(&dir);
}

/*! \fn     sd_card_logger_init(struct sd_card_logger *logger)
*   \brief  Determines the next log file name and opens the new file.
*   \param  logger Pointer to the logger instance
*   \return FR_OK on success
*/
FRESULT sd_card_logger_init(struct sd_card_logger *logger)
{
	FRESULT result = FR_DENIED;
	TCHAR filename[32] = { 0 };
	uint32_t index = 0;

	result = f_mkdir(LOG_DIR);
	/* Ignore if the directory already exists */
	if (result != FR_OK && result != FR_EXIST)
	{
		return result;
	}

	/* Construct next log file name */
	result = get_highest_file_index(&index);
	if (result != FR_OK)
	{
		return result;
	}

	snprintf(filename, sizeof(filename), LOG_DIR "/" LOG_FILE_FMT, index + 1);

	/* Open log file */
	result = f_open(&logger->file, filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (result != FR_OK)
	{
		return result;
	}

	/* Write header */
	f_printf(&logger->file, "timestamp;speed\n");
	return f_sync(&logger->file);
}

/*! \fn     sd_card_logger_log(struct sd_card_logger *logger, uint16_t speed)
*   \brief  Logs speed
*   \param  logger Pointer to the logger instance
*   \param  speed  Speed in tenth of km/h or mph
*   \return FR_OK on success
*/
void sd_card_logger_log(struct sd_card_logger *logger, uint16_t speed) {
	uint32_t tick = HAL_GetTick();
	uint32_t tick_integer = tick / 1000;
	uint32_t tick_fraction = (tick / 10) % 100;
	uint16_t speed_integer = speed / 10;
	uint16_t speed_fraction = speed % 10;

	/* Adding log entry in CSV format: [time in seconds];[speed in xxx.x]\n  */
	f_printf(&logger->file, "%u.%02u;%u.%u\n", tick_integer, tick_fraction, speed_integer, speed_fraction);
	logger->last_activity_tick = tick;
}

/*! \fn     sd_card_logger_flush(struct sd_card_logger *logger)
*   \brief  Flushes the log data to the SD card after a period of inactivity.
*   \return FR_OK on success
*/
void sd_card_logger_flush(struct sd_card_logger *logger)
{
	uint32_t tick = HAL_GetTick();

	if ((tick - logger->last_activity_tick) > LOG_FLUSH_TICK) {
		f_sync(&logger->file);
		logger->last_activity_tick = tick;
	}
}
