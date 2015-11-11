/******************************************************************************
 * @file    teleinfo.h 
 * @author  Rémi Pincent - INRIA
 * @date    10 nov. 2015   
 *
 * @brief EDF teleinformation object - refer http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf
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
#ifndef TELEINFO_TELEINFO_H_
#define TELEINFO_TELEINFO_H_

#include <stdint.h>
#include <Stream.h>

class Teleinfo {
public:
	typedef enum{
		INVALID_CRC  = -3,
		INVALID_READ = -2,
		READ_TIMEOUT = -1,
		NO_ERROR = 0,
		FRAME_AVAILABLE = 1,
	}EError;
private:
	template <class T>
	class FrameField{
		/** field name */
		const char* name;
		/** number of field data bytes */
		const uint8_t nbBytes;
		T fieldValue;
	};

	static const uint8_t START_TEXT        = 0x02;
	static const uint8_t END_TEXT          = 0x03;
	static const uint8_t END_OF_TEXT       = 0x04;
	static const uint8_t LINE_FEED         = 0x0A;
	static const uint8_t SPACE             = 0x20;
	/** carriage return */
	static const uint8_t CARRIAGE_RET      = 0x0D;
	static const uint8_t LABEL_MAX_LENGTH  = 7 + 1;
	static const uint8_t VALUE_MAX_LENGTH  = 15;

	/** telereport hub address field */
	static const char* ADC0;

	static const uint8_t MAX_LINE_LENGTH = 21;

	Stream* _p_infoStream;
	uint8_t _currLine[MAX_LINE_LENGTH];
	bool _continueRead;

public:
	Teleinfo(Stream* arg_p_stream);

	/**
	 * Start teleinfo reading on Stream
	 */
	void startRead(void);

	/**
	 * Stop teleinfo reading on Stream
	 */
	void stopRead(void);

	virtual ~Teleinfo();

private:
	/**
	 * Read a given number of bytes
	 * @param arg_u8_readByte
	 * @param arg_u8_nbBytes
	 * @return
	 */
	EError readBytes(uint8_t arg_u8_readByte[], uint8_t arg_u8_nbBytes);

	/**
	 * Read info groups in a frame
	 * @return
	 */
	EError readInfoGroups(void);

	/**
	 * @param label read
	 * @return
	 */
	EError readGroupLabel(char arg_as8_label[LABEL_MAX_LENGTH]);

	/**
	 *
	 * @param arg_au8_value
	 * @param arg_u8_value_length
	 * @return
	 */
	EError readGroupValue(uint8_t arg_au8_value[VALUE_MAX_LENGTH], uint8_t* arg_u8_value_length);

	/**
	 * Check Info Group CRC on a valid group label and value
	 * @return
	 */
	static bool isCRCOK(char * arg_s8_label, uint8_t * arg_u8_value, uint8_t arg_u8_valueLen, uint8_t arg_u8_crc);
};

#endif /* TELEINFO_TELEINFO_H_ */
