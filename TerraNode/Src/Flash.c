#include "Flash.h"
#include "Flash_impl.h"
#include <string.h>

#define VALID_FLAG 0xA5

static bool is_address_range_valid(uint32_t address, uint16_t length)
{
	return (address >= FLASH_START_ADDR) &&
		   ((address + length) <= FLASH_END_ADDR);
}

static bool is_flag_valid(uint32_t flagAddress)
{
	uint8_t flag;
	memcpy(&flag, (uint8_t *)(flagAddress), 1);
	return flag == VALID_FLAG;
}

HAL_StatusTypeDef Flash_read(uint32_t address, uint8_t *data, uint16_t length)
{
	if (!is_address_range_valid(address, length))
		return HAL_ERROR;

	memcpy(data, (uint8_t *)address, length);
	return HAL_OK;
}

HAL_StatusTypeDef Flash_read_check_flag(uint32_t address, uint8_t *data, uint16_t length)
{
	if (!is_address_range_valid(address, length))
	{
		return HAL_ERROR;
	}

	if (!is_flag_valid(address + length))
	{
		return HAL_ERROR;
	}

	memcpy(data, (uint8_t *)address, length);
	return HAL_OK;
}

HAL_StatusTypeDef Flash_write(uint32_t address, const uint8_t *data,
							  uint16_t length)
{
	if (!is_address_range_valid(address, length))
		return HAL_ERROR;

	uint32_t startOffset = address % FLASH_PAGE_SIZE;
	uint16_t dataOffset = 0;
	uint8_t pageCount = (startOffset + length + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
	HAL_StatusTypeDef status = HAL_OK;
	HAL_FLASH_Unlock();

	for (uint8_t pageIndex = 0; pageIndex < pageCount; pageIndex++)
	{
		uint32_t currentPageAddr = (address / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE + pageIndex * FLASH_PAGE_SIZE;
		uint8_t pageBuffer[FLASH_PAGE_SIZE];
		uint16_t bytesToCopyToCurrentPage = 0;
		uint16_t offsetInPage = 0;

		if (pageIndex == 0)
		{
			offsetInPage = address % FLASH_PAGE_SIZE;
			bytesToCopyToCurrentPage = FLASH_PAGE_SIZE - offsetInPage;
			if (bytesToCopyToCurrentPage > length)
				bytesToCopyToCurrentPage = length;
		}
		else if (pageIndex == pageCount - 1)
		{
			bytesToCopyToCurrentPage = (startOffset + length) % FLASH_PAGE_SIZE;
			if (bytesToCopyToCurrentPage == 0)
				bytesToCopyToCurrentPage = FLASH_PAGE_SIZE;
		}
		else
		{
			bytesToCopyToCurrentPage = FLASH_PAGE_SIZE;
		}

		memcpy(pageBuffer, (uint8_t *)currentPageAddr, FLASH_PAGE_SIZE);
		memcpy(&pageBuffer[offsetInPage], &data[dataOffset], bytesToCopyToCurrentPage);
		dataOffset += bytesToCopyToCurrentPage;

		uint32_t pageError;
		FLASH_EraseInitTypeDef EraseInit = {.TypeErase = FLASH_TYPEERASE_PAGES,
											.PageAddress = currentPageAddr,
											.NbPages = 1};
		status = HAL_FLASHEx_Erase(&EraseInit, &pageError);
		if (status != HAL_OK)
		{
			HAL_FLASH_Lock();
			return status;
		}

		// STM32L0 uses FLASH_TYPEPROGRAM_WORD (4 bytes) instead of DOUBLEWORD (8 bytes)
		for (uint32_t offset = 0; offset < FLASH_PAGE_SIZE; offset += 4)
		{
			uint32_t word;
			memcpy(&word, &pageBuffer[offset], sizeof(word));
			status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
									   currentPageAddr + offset,
									   word);
			if (status != HAL_OK)
			{
				HAL_FLASH_Lock();
				return status;
			}
		}
	}

	HAL_FLASH_Lock();
	return status;
}

