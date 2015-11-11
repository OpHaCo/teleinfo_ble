/******************************************************************************
 * @file    teleinfo.cpp 
 * @author  Rémi Pincent - INRIA
 * @date    10 nov. 2015   
 *
 * @brief TODO_one_line_description_of_file
 * 
 * Project : teleinfo_ble
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 * 
 * Revision History:
 * TODO_revision history
 * 
 * LICENSE :
 * teleinfo_ble (c) by Rémi Pincent
 * teleinfo_ble is licensed under a
 * Creative Commons Attribution-NonCommercial 3.0 Unported License.
 *
 * You should have received a copy of the license along with this
 * work.  If not, see <http://creativecommons.org/licenses/by-nc/3.0/>.
 *****************************************************************************/
#include <teleinfo.h>
#include <delay.h>
#include <logger.h>


const char* Teleinfo::ADC0 = "ADC0";

Teleinfo::Teleinfo(Stream* arg_p_stream) :
	_p_infoStream(arg_p_stream),
	_continueRead(false)
{
	memset(_currLine, 0, MAX_LINE_LENGTH);

}

Teleinfo::~Teleinfo() {
	// TODO Auto-generated destructor stub
}

void Teleinfo::startRead(void)
{
	_continueRead = true;
	uint8_t loc_u8_buff[1] = {0};
	Teleinfo::EError loc_e_error = NO_ERROR;

	while(_continueRead)
	{
		loc_e_error = readBytes(loc_u8_buff, 1);
		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read byte - err = %d", loc_e_error);
			continue;
		}

		if(loc_u8_buff[0] == START_TEXT)
		{
			loc_e_error = readInfoGroups();
			if(loc_e_error != NO_ERROR)
			{
				LOG_ERROR("Cannot read information groups - err = %d", loc_e_error);
			}
			else
			{
				LOG_INFO_LN("Info groups successfully read");
			}
		}
		else if(loc_u8_buff[0] == END_TEXT)
		{
			LOG_INFO_LN("End of frame");
		}
		else if(loc_u8_buff[0] == END_OF_TEXT)
		{
			LOG_INFO_LN("End of text");
		}
		else
		{
			LOG_DEBUG("Cannot handle byte %x in frame reading", loc_u8_buff[0]);
		}
	}
}

Teleinfo::EError Teleinfo::readInfoGroups(void)
{
	uint8_t loc_u8_buff[1] = {0};
	char loc_s8_label[LABEL_MAX_LENGTH] = {0};
	uint8_t loc_u8_value[VALUE_MAX_LENGTH] = {0};
	uint8_t loc_u8_valueLength = 0;

	Teleinfo::EError loc_e_error = readBytes(loc_u8_buff, 1);

	if(loc_e_error < NO_ERROR)
	{
		LOG_ERROR("Cannot read byte - err = %d", loc_e_error);
		return INVALID_READ;
	}

	if(loc_u8_buff[0] == LINE_FEED)
	{
		loc_e_error = readGroupLabel(loc_s8_label);
		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read group label - err = %d", loc_e_error);
			return INVALID_READ;
		}
		else
		{
			LOG_INFO_LN("read label = %s", loc_s8_label);
		}

		loc_e_error = readGroupValue(loc_u8_value, &loc_u8_valueLength);
		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read group value - err = %d", loc_e_error);
			return INVALID_READ;
		}

		/** now check if read group has a valid CRC */
		loc_e_error = readBytes(loc_u8_buff, 1);

		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read CRC - err = %d", loc_e_error);
			return INVALID_READ;
		}

		if(!isCRCOK(loc_s8_label, loc_u8_value, loc_u8_valueLength, loc_u8_buff[0]))
		{
			LOG_ERROR("invalid CRC");
			return INVALID_CRC;
		}

		/** now check if read group has a valid CRC */
		loc_e_error = readBytes(loc_u8_buff, 1);

		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read last group byte - err = %d", loc_e_error);
			return INVALID_READ;
		}

		if(loc_u8_buff[0] != CARRIAGE_RET)
		{
			LOG_ERROR("Invalid byte %x received, %x expected", loc_u8_buff[0], CARRIAGE_RET);
			return INVALID_READ;
		}
		else
		{
			return NO_ERROR;
		}
	}
	else
	{
		LOG_ERROR("Invalid byte %x received, %x expected", loc_u8_buff[0], LINE_FEED);
		return INVALID_READ;
	}
}



Teleinfo::EError Teleinfo::readGroupLabel(char arg_as8_label[LABEL_MAX_LENGTH])
{
	uint8_t loc_u8_buff[1] = {0};
	Teleinfo::EError loc_e_error = readBytes(loc_u8_buff, 1);
	uint8_t loc_u8_index = 0;

	while(loc_u8_buff[0] != SPACE)
	{
		arg_as8_label[loc_u8_index++] = loc_u8_buff[0];
		loc_e_error = readBytes(loc_u8_buff, 1);
		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read byte - err = %d", loc_e_error);
			return INVALID_READ;
		}
	}
	arg_as8_label[loc_u8_index] = '\0';
	return NO_ERROR;
}

Teleinfo::EError Teleinfo::readGroupValue(uint8_t arg_au8_value[VALUE_MAX_LENGTH], uint8_t* arg_u8_value_length)
{
	uint8_t loc_u8_buff[1] = {0};
	Teleinfo::EError loc_e_error = readBytes(loc_u8_buff, 1);
	*arg_u8_value_length = 0;

	while(loc_u8_buff[0] != SPACE)
	{
		arg_au8_value[(*arg_u8_value_length)++] = loc_u8_buff[0];
		loc_e_error = readBytes(loc_u8_buff, 1);
		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read byte - err = %d", loc_e_error);
			return INVALID_READ;
		}
	}
	return NO_ERROR;
}

Teleinfo::EError parseValue()
{

}


void Teleinfo::stopRead(void)
{
	_continueRead = false;
}

Teleinfo::EError Teleinfo::readBytes(uint8_t arg_u8_readByte[], uint8_t arg_u8_nbBytes)
{
	unsigned long loc_u32_startMillis = millis();
	const unsigned long READ_TIMEOUT_MS = 100;
	const uint8_t READ_WAIT_MS = 10;

	uint8_t loc_u8_index = 0;

	while(loc_u8_index < arg_u8_nbBytes)
	{
		if(_p_infoStream->available())
		{
			/** transmission on 7 bits - LSB*/
			arg_u8_readByte[loc_u8_index++] = _p_infoStream->read() & 0x7F;
			LOG_INFO_LN("  %x", arg_u8_readByte[loc_u8_index - 1]);
			loc_u32_startMillis = millis();
		}
		else if(millis() - loc_u32_startMillis >= READ_TIMEOUT_MS)
		{
			return READ_TIMEOUT;
		}
		else
		{
			delay(READ_WAIT_MS);
		}

	}
	return NO_ERROR;
}

bool Teleinfo::isCRCOK(char * arg_s8_label, uint8_t * arg_u8_value, uint8_t arg_u8_valueLen, uint8_t arg_u8_crc)
{
  uint8_t loc_u8_index = 0;

  /** 1 space char to add - p11 - http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf */
  uint8_t loc_u8_sum = SPACE;

  while(arg_s8_label[loc_u8_index] != '\0')
  {
	  loc_u8_sum += arg_s8_label[loc_u8_index++];
  }

  for(loc_u8_index = 0; loc_u8_index < arg_u8_valueLen; )
  {
	  loc_u8_sum += arg_u8_value[loc_u8_index++];
  }

  loc_u8_sum = (loc_u8_sum & 0x3F) + 0x20;
  return (loc_u8_sum == arg_u8_crc);
}


