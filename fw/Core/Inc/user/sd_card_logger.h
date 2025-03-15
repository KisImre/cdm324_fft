#ifndef SD_CARD_LOGGER_H_
#define SD_CARD_LOGGER_H_

#include "fatfs.h"
#include <stdint.h>

struct sd_card_logger {
	FIL file;
	uint32_t last_activity_tick;
};

FRESULT sd_card_logger_init(struct sd_card_logger *logger);
void sd_card_logger_log(struct sd_card_logger *logger, uint16_t speed);
void sd_card_logger_flush(struct sd_card_logger *logger);

#endif /* SD_CARD_LOGGER_H_ */
