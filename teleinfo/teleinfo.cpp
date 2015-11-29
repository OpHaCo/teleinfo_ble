/******************************************************************************
 * @file    teleinfo.cpp 
 * @author  Rémi Pincent - INRIA
 * @date    10 nov. 2015   
 *
 * @brief TODO_one_line_description_of_file
 * 
 * Project : teleinfo_lib
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
#include "teleinfo.h"
#include "teleinfo_fields.h"
#include <delay.h>
#include <logger.h>


/*************************************
 * TYPES only used in this source file
 *************************************/

struct PTECMapping{
	const char* _teleinfoField;
	EPTEC _e_Val;
};

struct OPTarMapping{
	const char* _teleinfoField;
	EOptTar _e_Val;
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
		{"TH.."  , TH},
		{"HC.."  , HC},
		{"HP.."  , HP},
		{"HN.."  , HN},
		{"PM.."  , PM},
};


/*************************************
 * Method definitions
 *************************************/

Teleinfo::Teleinfo(Stream* arg_p_stream) :
	_p_infoStream(arg_p_stream),
	_continueRead(false),
	_p_teleinfoListener(NULL)
{
	_telereportHubAddr._fieldValue = (char*)malloc(TELEREPORT_HUB_ADDR_LENGTH + 1);
	_modEtat._fieldValue             = (char*)malloc(MOD_ETAT_LENGTH + 1);
}

Teleinfo::~Teleinfo() {
	if(_telereportHubAddr._fieldValue)
	{
		free(_telereportHubAddr._fieldValue);
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
	if(strcmp(_telereportHubAddr._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _telereportHubAddr._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}

		if(strcmp(_telereportHubAddr._fieldValue, (char*) arg_u8_value) != 0)
		{
			/** +1 for null terminated string */
			strncpy(_telereportHubAddr._fieldValue, (const char*) arg_u8_value, TELEREPORT_HUB_ADDR_LENGTH + 1);
			if(_p_teleinfoListener) {_p_teleinfoListener->hubAddrChanged(_telereportHubAddr._fieldValue);}
			LOG_DEBUG_LN("%s = %s", _telereportHubAddr._as8_name, _telereportHubAddr._fieldValue);
		}
		return NO_ERROR;
	}
	/** Power meter state - MODETAT */
	else if(strcmp(_modEtat._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _modEtat._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		/** +1 for null char */
		if(strcmp(_modEtat._fieldValue, (char*) arg_u8_value) != 0)
		{
			strncpy(_modEtat._fieldValue, (const char*) arg_u8_value, arg_u8_valueLen + 1);
			LOG_DEBUG_LN("%s = %s", _modEtat._as8_name, _modEtat._fieldValue);
			if(_p_teleinfoListener) {_p_teleinfoListener->modEtatChanged(_modEtat._fieldValue);}
		}
		return NO_ERROR;
	}
	/** Price option selected - OPTRARIF */
	else if(strcmp(_optTar._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _optTar._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}

		for(uint8_t loc_u8_optIndex = 0; loc_u8_optIndex < NB_OPT_TAR; loc_u8_optIndex++)
		{
			if(strcmp(OPT_TAR[loc_u8_optIndex]._teleinfoField, (const char*) arg_u8_value) == 0)
			{
				if(_optTar._fieldValue != OPT_TAR[loc_u8_optIndex]._e_Val)
				{
					_optTar._fieldValue = OPT_TAR[loc_u8_optIndex]._e_Val;
					if(_p_teleinfoListener) {_p_teleinfoListener->optTarChanged(_optTar._fieldValue);}
					LOG_INFO_LN("%s = %d", _optTar._as8_name, _optTar._fieldValue);
				}
				return NO_ERROR;
			}
		}
		return INVALID_VALUE;
	}
	/** Subscribed intensity -ISOUSC */
	else if(strcmp(_souscInt._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _souscInt._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		if(_souscInt._fieldValue != atoi((const char*)arg_u8_value))
		{
			_souscInt._fieldValue = atoi((const char*)arg_u8_value);
			if(_p_teleinfoListener) {_p_teleinfoListener->souscIntChanged(_souscInt._fieldValue);}
			LOG_DEBUG_LN("%s = %dA", _souscInt._as8_name, _souscInt._fieldValue);
		}
		return NO_ERROR;
	}
	/** Current intensity -IINST */
	else if(strcmp(_instInt._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _instInt._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		if(_instInt._fieldValue != atoi((const char*)arg_u8_value))
		{
			_instInt._fieldValue = atoi((const char*)arg_u8_value);
			if(_p_teleinfoListener) {_p_teleinfoListener->souscIntChanged(_instInt._fieldValue);}
			LOG_DEBUG_LN("%s = %dA", _instInt._as8_name, _instInt._fieldValue);
		}
		return NO_ERROR;
	}
	/** Max called Int -IMAX */
	else if(strcmp(_maxInt._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _maxInt._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		if(_maxInt._fieldValue != atoi((const char*)arg_u8_value))
		{
			_maxInt._fieldValue = atoi((const char*)arg_u8_value);
			if(_p_teleinfoListener) {_p_teleinfoListener->maxIntChanged(_maxInt._fieldValue);}
			LOG_DEBUG_LN("%s = %dA", _maxInt._as8_name, _maxInt._fieldValue);
		}
		return NO_ERROR;
	}
	/** HCHC index - HCHC */
	else if(strcmp(_hcIndex._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _hcIndex._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		if(_hcIndex._fieldValue != (uint32_t)atoi((const char*) arg_u8_value))
		{
			_hcIndex._fieldValue = atoi((const char*) arg_u8_value);
			if(_p_teleinfoListener) {_p_teleinfoListener->hcIndexChanged(_hcIndex._fieldValue);}
			LOG_DEBUG_LN("%s = %dWh", _hcIndex._as8_name, _hcIndex._fieldValue);
		}
		return NO_ERROR;
	}
	/** Full hour index - HPHP*/
	else if(strcmp(_hpIndex._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _hpIndex._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		if(_hpIndex._fieldValue != (uint32_t)atoi((const char*) arg_u8_value))
		{
			_hpIndex._fieldValue = atoi((const char*) arg_u8_value);
			if(_p_teleinfoListener) {_p_teleinfoListener->hpIndexChanged(_hcIndex._fieldValue);}
			LOG_DEBUG_LN("%s = %dWh", _hpIndex._as8_name, _hpIndex._fieldValue);
		}
		return NO_ERROR;
	}
	/** Base index - BASE */
	else if(strcmp(_baseIndex._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _baseIndex._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		if(_baseIndex._fieldValue != (uint32_t)atoi((const char*) arg_u8_value))
		{
			_baseIndex._fieldValue = atoi((const char*) arg_u8_value);
			if(_p_teleinfoListener) {_p_teleinfoListener->baseIndexChanged(_baseIndex._fieldValue);}
			LOG_DEBUG_LN("%s = %dWh", _baseIndex._as8_name, _baseIndex._fieldValue);
		}
		return NO_ERROR;
	}
	/** Message indicating EJP - PTEC */
	else if(strcmp(_ejpMess._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _ejpMess._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		if(_ejpMess._fieldValue == atoi((const char*) arg_u8_value))
		{
			_ejpMess._fieldValue = atoi((const char*) arg_u8_value);
			if(_p_teleinfoListener) {_p_teleinfoListener->ejpMessChanged(_ejpMess._fieldValue);}
			LOG_DEBUG_LN("%s = %dmin", _ejpMess._as8_name, _ejpMess._fieldValue);
		}
		return NO_ERROR;
	}
	/** Current pricing option - PTEC*/
	else if(strcmp(_currTar._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _currTar._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}

		for(uint8_t loc_u8_tarIndex = 0; loc_u8_tarIndex < NB_PTEC; loc_u8_tarIndex++)
		{
			if(strcmp(PTEC[loc_u8_tarIndex]._teleinfoField, (const char*) arg_u8_value) == 0)
			{
			   if(_currTar._fieldValue != PTEC[loc_u8_tarIndex]._e_Val)
			   {
				   _currTar._fieldValue = PTEC[loc_u8_tarIndex]._e_Val;
				   if(_p_teleinfoListener) {_p_teleinfoListener->currTarChanged(_currTar._fieldValue);}
				   LOG_DEBUG_LN("%s = %d", _currTar._as8_name, _currTar._fieldValue);
			   }
				return NO_ERROR;
			}
		}
		return INVALID_VALUE;
	}
	/** Apparent power - PAPP */
	else if(strcmp(_appPower._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _appPower._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		if(_appPower._fieldValue != (uint32_t)atoi((const char*)arg_u8_value))
		{
			_appPower._fieldValue = atoi((const char*)arg_u8_value);
			if(_p_teleinfoListener) {_p_teleinfoListener->appPowerChanged(_appPower._fieldValue);}
			LOG_DEBUG_LN("%s = %dVA", _appPower._as8_name, _appPower._fieldValue);
		}
		return NO_ERROR;
	}
	/** HHPHC */
	else if(strcmp(_hhphc._as8_name, arg_s8_label) == 0)
	{
		if(arg_u8_valueLen != _hhphc._u8_nbBytes)
		{
			return INVALID_LENGTH;
		}
		if(_hhphc._fieldValue != (char)arg_u8_value[0])
		{
			_hhphc._fieldValue = (char)arg_u8_value[0];
			if(_p_teleinfoListener) {_p_teleinfoListener->hhphcChanged(_hhphc._fieldValue);}
			LOG_DEBUG_LN("%s = %c", _hhphc._as8_name, _hhphc._fieldValue);
		}
		return NO_ERROR;
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

void Teleinfo::registerListener(ITeleinfoListener& arg_listener)
{
	/** only 1 listener */
	if(_p_teleinfoListener != NULL)
	{
		ASSERT(false);
	}
	else
	{
		 _p_teleinfoListener = &arg_listener;
	}
}
void Teleinfo::unRegisterListener(ITeleinfoListener& arg_listener)
{
	_p_teleinfoListener = NULL;
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


