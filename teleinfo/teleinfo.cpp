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

/*************************************
 * TYPES only used in this source file
 *************************************/

struct PTECMapping{
	const char* _teleinfoField;
	Teleinfo::EPTEC _e_Val;
};

struct OPTarMapping{
	const char* _teleinfoField;
	Teleinfo::EOptTar _e_Val;
};

/*************************************
 * Static definitions
 *************************************/
const char* Teleinfo::ADC0 = "ADC0";


const struct OPTarMapping Teleinfo::OPT_TAR[NB_OPT_TAR] =
{
		{"BASE"  , BASE_TAR},
		{"HC.."  , HC_TAR},
		{"EJP."  , EJP_TAR},
};

const struct PTECMapping Teleinfo::PTEC[NB_PTEC] =
{
		{"TH.."  , Teleinfo::TH},
		{"HC.."  , Teleinfo::HC},
		{"HP.."  , Teleinfo::HP},
		{"HN.."  , Teleinfo::HN},
		{"PM.."  , Teleinfo::PM},
};

/*************************************
 * Method definitions
 *************************************/

Teleinfo::Teleinfo(Stream* arg_p_stream) :
	_p_infoStream(arg_p_stream),
	_continueRead(false)
{
	_telereport_hub_addr._fieldValue = (char*)malloc(TELEREPORT_HUB_ADDR_LENGTH + 1);
	_modEtat._fieldValue             = (char*)malloc(MOD_ETAT_LENGTH + 1);
}

Teleinfo::~Teleinfo() {
	if(_telereport_hub_addr._fieldValue)
	{
		free(_telereport_hub_addr._fieldValue);
	}

	if(_modEtat._fieldValue)
	{
		free(_modEtat._fieldValue);
	}
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

				LOG_DEBUG_LN("Info groups successfully read");
			}
		}
		else if(loc_u8_buff[0] == END_TEXT)
		{
			LOG_DEBUG_LN("End of frame");
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
	Teleinfo::EError loc_e_error = NO_ERROR;

	while(_continueRead)
	{
		loc_e_error = readBytes(loc_u8_buff, 1);
		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read byte - err = %d", loc_e_error);
			return INVALID_READ;
		}

		if(loc_u8_buff[0] == LINE_FEED)
		{
			loc_e_error = readInfoGroup();
			if(loc_e_error < NO_ERROR)
			{
				LOG_ERROR("Cannot read info group - err = %d", loc_e_error);
				return INVALID_READ;
			}
		}
		else if(loc_u8_buff[0] == END_TEXT)
		{
			LOG_DEBUG_LN("End of frame - no more info groups to read");
			return NO_ERROR;
		}
		else if(loc_u8_buff[0] == END_OF_TEXT)
		{
			LOG_INFO_LN("End of text - no more info groups to read");
			return NO_ERROR;
		}
		else
		{
			//LOG_ERROR("Invalid byte %x received, %x or %x expected", loc_u8_buff[0], LINE_FEED, END_OF_TEXT);
			return INVALID_READ;
		}
	}

	return NO_ERROR;
}

Teleinfo::EError Teleinfo::readInfoGroup(void)
{
	uint8_t loc_u8_buff[1] = {0};
	char loc_s8_label[LABEL_MAX_LENGTH] = {0};
	uint8_t loc_au8_value[VALUE_MAX_LENGTH] = {0};
	uint8_t loc_u8_valueLength = 0;
	Teleinfo::EError loc_e_error = NO_ERROR;

	loc_e_error = readGroupLabel(loc_s8_label);
	if(loc_e_error < NO_ERROR)
	{
		LOG_ERROR("Cannot read group label - err = %d", loc_e_error);
		return INVALID_READ;
	}

	loc_e_error = readGroupValue(loc_au8_value, &loc_u8_valueLength);
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

	if(!isCRCOK(loc_s8_label, loc_au8_value, loc_u8_valueLength, loc_u8_buff[0]))
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
		loc_e_error = parseGroup(loc_s8_label, loc_au8_value, loc_u8_valueLength);
		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot parse group %s - err = %d", loc_s8_label, loc_e_error);
		}
		return loc_e_error;
	}
}


Teleinfo::EError Teleinfo::readGroupLabel(char arg_as8_label[LABEL_MAX_LENGTH])
{
	uint8_t loc_u8_buff[1] = {0};
	Teleinfo::EError loc_e_error = NO_ERROR;
	uint8_t loc_u8_index = 0;

	for(loc_u8_index = 0; loc_u8_index < LABEL_MAX_LENGTH; loc_u8_index++)
	{
		loc_e_error = readBytes(loc_u8_buff, 1);
		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read byte - err = %d", loc_e_error);
			return INVALID_READ;
		}
		if(loc_u8_buff[0] != SPACE)
		{
			arg_as8_label[loc_u8_index] = loc_u8_buff[0];
		}
		else
		{
			arg_as8_label[loc_u8_index] = '\0';
			return NO_ERROR;
		}
	}

	return INVALID_LENGTH;
}

Teleinfo::EError Teleinfo::readGroupValue(uint8_t arg_au8_value[VALUE_MAX_LENGTH], uint8_t* arg_u8_value_length)
{
	uint8_t loc_u8_buff[1] = {0};
	Teleinfo::EError loc_e_error = NO_ERROR;
	*arg_u8_value_length = 0;

	for(*arg_u8_value_length = 0; *arg_u8_value_length < VALUE_MAX_LENGTH; (*arg_u8_value_length)++)
	{
		loc_e_error = readBytes(loc_u8_buff, 1);
		if(loc_e_error < NO_ERROR)
		{
			LOG_ERROR("Cannot read byte - err = %d", loc_e_error);
			return INVALID_READ;
		}

		if(loc_u8_buff[0] != SPACE)
		{
			arg_au8_value[*arg_u8_value_length] = loc_u8_buff[0];
		}
		else
		{
			return NO_ERROR;
		}
	}

	return INVALID_LENGTH;
}

Teleinfo::EError Teleinfo::parseGroup(char * arg_s8_label, uint8_t * arg_u8_value, uint8_t arg_u8_valueLen)
{
	/** Teleinfo HUB address -ADC0 */
	if(strcmp(_telereport_hub_addr._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _telereport_hub_addr._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		/** +1 for null terminated string */
		strncpy(_telereport_hub_addr._fieldValue, (const char*) arg_u8_value, TELEREPORT_HUB_ADDR_LENGTH + 1);
		LOG_INFO_LN("%s = %s", _telereport_hub_addr._as8_name, _telereport_hub_addr._fieldValue);
		return NO_ERROR;
	}
	/** Power meter state - MODETAT */
	else if(strcmp(_modEtat._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _modEtat._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		memset(_modEtat._fieldValue, 0, MOD_ETAT_LENGTH + 1);
		strncpy(_modEtat._fieldValue, (const char*) arg_u8_value, arg_u8_valueLen);
		LOG_INFO_LN("%s = %s", _modEtat._as8_name, _modEtat._fieldValue);
		return NO_ERROR;
	}
	/** Price option selected - OPTRARIF */
	else if(strcmp(_optTar._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _optTar._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}

		for(uint8_t loc_u8_optIndex = 0; loc_u8_optIndex < Teleinfo::NB_OPT_TAR; loc_u8_optIndex++)
		{
			if(strcmp(OPT_TAR[loc_u8_optIndex]._teleinfoField, (const char*) arg_u8_value) == 0)
			{
				_optTar._fieldValue = OPT_TAR[loc_u8_optIndex]._e_Val;
				LOG_INFO_LN("%s = %d", _optTar._as8_name, _optTar._fieldValue);
				return NO_ERROR;
			}
		}
		return INVALID_VALUE;
	}
	/** Subscribed intensity -ISOUSC */
	else if(strcmp(_intSousc._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _intSousc._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		_intSousc._fieldValue = atoi((const char*)arg_u8_value);
		LOG_INFO_LN("%s = %dA", _intSousc._as8_name, _intSousc._fieldValue);
		return NO_ERROR;
	}
	/** HCHC index - HCHC */
	else if(strcmp(_hcIndex._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _hcIndex._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		_hcIndex._fieldValue = atoi((const char*) arg_u8_value);
		LOG_INFO_LN("%s = %dWh", _hcIndex._as8_name, _hcIndex._fieldValue);
		return NO_ERROR;
	}
	/** Full hour index - HPHP*/
	else if(strcmp(_hpIndex._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _hpIndex._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		_hpIndex._fieldValue = atoi((const char*) arg_u8_value);
		LOG_INFO_LN("%s = %dWh", _hpIndex._as8_name, _hpIndex._fieldValue);
		return NO_ERROR;
	}
	/** Base index - BASE */
	else if(strcmp(_baseIndex._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _baseIndex._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		_baseIndex._fieldValue = atoi((const char*) arg_u8_value);
		LOG_INFO_LN("%s = %dWh", _baseIndex._as8_name, _baseIndex._fieldValue);
		return NO_ERROR;
	}
	/** Message indicating EJP - PTEC */
	else if(strcmp(_ejpMess._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _ejpMess._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		_ejpMess._fieldValue = atoi((const char*) arg_u8_value);
		LOG_INFO_LN("%s = %dmin", _ejpMess._as8_name, _ejpMess._fieldValue);
		return NO_ERROR;
	}
	/** Current pricing option - PTEC*/
	else if(strcmp(_currTar._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _currTar._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}

		for(uint8_t loc_u8_tarIndex = 0; loc_u8_tarIndex < Teleinfo::NB_PTEC; loc_u8_tarIndex++)
		{
			if(strcmp(PTEC[loc_u8_tarIndex]._teleinfoField, (const char*) arg_u8_value) == 0)
			{
				_currTar._fieldValue = PTEC[loc_u8_tarIndex]._e_Val;
				LOG_INFO_LN("%s = %d", _currTar._as8_name, _currTar._fieldValue);
				return NO_ERROR;
			}
		}
		return INVALID_VALUE;
	}
	else
	{
		LOG_ERROR("%s group not handled", arg_s8_label);
		return NOT_HANDLED_GROUP;
	}
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
			//LOG_INFO_LN("%x", arg_u8_readByte[loc_u8_index - 1]);
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


