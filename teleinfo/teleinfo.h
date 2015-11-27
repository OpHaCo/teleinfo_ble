/******************************************************************************
 * @file    teleinfo.h 
 * @author  Rémi Pincent - INRIA
 * @date    10 nov. 2015   
 *
 * @brief EDF teleinformation class - refer http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf
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
	/** Teleinfo errors */
	typedef enum{
		INVALID_VALUE  = -6,
		NOT_HANDLED_GROUP  = -5,
		INVALID_LENGTH  = -4,
		INVALID_CRC  = -3,
		INVALID_READ = -2,
		READ_TIMEOUT = -1,
		NO_ERROR = 0,
		FRAME_AVAILABLE = 1,
	}EError;

	/** Pricing option - OPTAR */
	typedef enum{
		BASE_TAR = 0,
		HC_TAR   = 1,
		EJP_TAR  = 2,
		OPT_TAR_OUT_OF_ENUM,
		NB_OPT_TAR = OPT_TAR_OUT_OF_ENUM
	}EOptTar;

	/** Current pricing option - PTEC */
	typedef enum{
		TH   = 0,
		HC   = 1,
		HP   = 2,
		HN   = 3,
		PM   = 4,
		PTEC_OUT_OF_ENUM,
		NB_PTEC =  PTEC_OUT_OF_ENUM
	}EPTEC;

private:

	/**
	 * Teleinfo frame fied defined according to
	 * http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf
	 */
	template <class T>
	struct FrameField{
		/** field name */
		const char* _as8_name;
		/** number of field data bytes */
		const uint8_t _u8_nbBytes;
		T _fieldValue;
	};

	/*****************************
	 * Teleinfo special chars
	 *****************************/
	static const uint8_t START_TEXT        = 0x02;
	static const uint8_t END_TEXT          = 0x03;
	static const uint8_t END_OF_TEXT       = 0x04;
	static const uint8_t LINE_FEED         = 0x0A;
	static const uint8_t SPACE             = 0x20;
	/** carriage return */
	static const uint8_t CARRIAGE_RET      = 0x0D;
	static const uint8_t LABEL_MAX_LENGTH  = 7 + 1;
	static const uint8_t VALUE_MAX_LENGTH  = 15;

	/** Max teleinfo line length */
	static const uint8_t MAX_LINE_LENGTH   = 21;

	/*****************************
	 * Teleinfo fields size
	 *****************************/
	static const uint8_t TELEREPORT_HUB_ADDR_LENGTH     = 12;
	static const uint8_t OP_TAR_LENGTH                  = 4;
	static const uint8_t INDEX_LENGTH                   = 9;
	static const uint8_t GAZ_INDEX_LENGTH               = 7;
	static const uint8_t PTEC_LENGTH                    = 4;
	static const uint8_t MOD_ETAT_LENGTH                = 6;
	static const uint8_t CURR_INT_LENGTH                = 3;
	static const uint8_t INT_SOUSC_LENGTH               = 2;
	static const uint8_t PTEC_MESS_LENGTH               = 2;

	/*********************************************
	 * teleinfo fields on client power meter side
	 ********************************************/
	struct FrameField<char*>     _telereport_hub_addr   = {"ADCO",       TELEREPORT_HUB_ADDR_LENGTH,  NULL};
	struct FrameField<EOptTar>   _optTar                = {"OPTARIF",    OP_TAR_LENGTH,               OPT_TAR_OUT_OF_ENUM};
	struct FrameField<uint32_t>  _baseIndex             = {"BASE",       INDEX_LENGTH,                0};
	struct FrameField<uint32_t>  _hcIndex               = {"HCHC",       INDEX_LENGTH,                0};
	struct FrameField<uint32_t>  _hpIndex               = {"HCHP",       INDEX_LENGTH,                0};
	struct FrameField<uint32_t>  _ejpHMIndex            = {"EJPHN",      INDEX_LENGTH,                0};
	struct FrameField<uint32_t>  _ejpHPMIndex           = {"EJPHPM",     INDEX_LENGTH,                0};
	struct FrameField<uint8_t>   _ejpMess               = {"PEJP",       PTEC_MESS_LENGTH,            0};
	struct FrameField<uint32_t>  _gazIndex              = {"GAZ",        GAZ_INDEX_LENGTH,            0};
	struct FrameField<EPTEC>     _currTar               = {"PTEC",       PTEC_LENGTH,                 PTEC_OUT_OF_ENUM};
	struct FrameField<char*>     _modEtat               = {"MOTDETAT",   MOD_ETAT_LENGTH,             NULL};
	struct FrameField<uint16_t>  _instInt               = {"IINST",      CURR_INT_LENGTH,             0};
	struct FrameField<uint16_t>  _intSousc              = {"ISOUSC",     INT_SOUSC_LENGTH,            0};



	/** telereport hub address field */
	static const char* ADC0;
	/** OPTAR mapping from teleinfo raw field to EOptTar*/
	static const struct OPTarMapping OPT_TAR[NB_OPT_TAR];
	/** PTEC mapping from teleinfo raw field to EPTEC*/
	static const struct PTECMapping PTEC[NB_PTEC];

	/** teleinfo stream */
	Stream* _p_infoStream;
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
	 * Parse given group and updates related teleinfo values
	 * @param arg_s8_label
	 * @param arg_u8_value
	 * @param arg_u8_valueLen
	 * @return
	 */
	EError parseGroup(char * arg_s8_label, uint8_t * arg_u8_value, uint8_t arg_u8_valueLen);

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
	 * Read an info group
	 * @return
	 */
	EError readInfoGroup(void);

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
