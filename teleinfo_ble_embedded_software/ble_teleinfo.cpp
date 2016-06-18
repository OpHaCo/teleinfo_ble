/******************************************************************************
 * @file    ble_teleinfo.cpp 
 * @author  Rémi Pincent - INRIA
 * @date    18 juin 2016   
 *
 * @brief Class that get teleinfo data and sends it over ble
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
#include <ble_teleinfo.h>
#include "logger.h"

BleTeleinfo::BleTeleinfo(BLETransceiver& arg_p_bleTransceiver) : _p_bleTransceiver(&arg_p_bleTransceiver),
_timer(this),
_teleinfo(&Serial)

{
	_teleinfo.registerListener(*this);
	_p_bleTransceiver->registerListener(this);
};

BleTeleinfo::~BleTeleinfo(void)
{
};

void BleTeleinfo::start(void)
{
	_timer.notifyAfter(2000);
}

void BleTeleinfo::hubAddrChanged(char* arg_hubAddr){LOG_INFO_LN("hubAddr = %s", arg_hubAddr);};

void BleTeleinfo::optTarChanged(EOptTar arg_e_optTar){LOG_INFO_LN("optTar = %d", arg_e_optTar);};

void BleTeleinfo::baseIndexChanged(uint32_t arg_u32_baseIndex){LOG_INFO_LN("baseIndex = %dWh", arg_u32_baseIndex);};

void BleTeleinfo::hcIndexChanged(uint32_t arg_u32_hcIndex){LOG_INFO_LN("hcIndex = %dWh", arg_u32_hcIndex);};

void BleTeleinfo::hpIndexChanged(uint32_t arg_u32_hpIndex){LOG_INFO_LN("hpIndex = %dWh", arg_u32_hpIndex);};

void BleTeleinfo::ejpHMIChanged(uint32_t arg_u32_ejpHMI){LOG_INFO_LN("ejpHMI = %d", arg_u32_ejpHMI);};

void BleTeleinfo::ejpHPMIChanged(uint32_t arg_u32_ejpHPMI){LOG_INFO_LN("ejpHPMI = %d", arg_u32_ejpHPMI);};

void BleTeleinfo::ejpMessChanged(uint8_t arg_u8_ejpMess){LOG_INFO_LN("ejpMess = %d", arg_u8_ejpMess);};

void BleTeleinfo::gazIndexChanged(uint32_t arg_u32_gazIndex){LOG_INFO_LN("gazIndex = %ddal", arg_u32_gazIndex);};

void BleTeleinfo::currTarChanged(EPTEC arg_e_currTar){LOG_INFO_LN("currTar = %d", arg_e_currTar);};

void BleTeleinfo::modEtatChanged(char* arg_modEtat){LOG_INFO_LN("modEtat = %s", arg_modEtat);};

void BleTeleinfo::instIntChanged(uint16_t arg_u16_instInt)
{
	if(_p_bleTransceiver->isConnected())
	{
		uint8_t loc_u8_length = sizeof(arg_u16_instInt);
		uint8_t loc_u8_dataToSend[sizeof(arg_u16_instInt)] = {(uint8_t) ((arg_u16_instInt >> 8) & 0xFF),
				(uint8_t)(arg_u16_instInt & 0xFF)
		};
		BLETransceiver::Error loc_e_err = _p_bleTransceiver->send(loc_u8_length, loc_u8_dataToSend);
		if(loc_e_err < BLETransceiver::NO_ERROR || loc_u8_length < sizeof(arg_u16_instInt))
		{
			LOG_ERROR("Cannot send iinst - err = %d", loc_e_err);
		}
	}
	_teleinfo.stopRead();
};

void BleTeleinfo::maxIntChanged(uint16_t arg_u16_maxInt)
{
};

void BleTeleinfo::souscIntChanged(uint16_t arg_u16_souscInt){LOG_INFO_LN("souscInt = %dA", arg_u16_souscInt);};

void BleTeleinfo::appPowerChanged(uint16_t arg_u32_appPower){LOG_INFO_LN("appPower = %dVA", arg_u32_appPower);};

void BleTeleinfo::hhphcChanged(char arg_s8_hhphc){LOG_INFO_LN("hhphc = %c", arg_s8_hhphc);};

/** from TimerListener */
void BleTeleinfo::timerElapsed(void)
{
	/** read teleinfo periodically */
	if(_p_bleTransceiver->isConnected())
	{
		_teleinfo.readFrame(20000);
	}
	_timer.notifyAfter(2000);
};

/** from TimerListener */
void BleTeleinfo::onDataReceived(uint8_t arg_u8_dataLength, uint8_t arg_au8_data[])
{

};

void BleTeleinfo::onConnection(void)
{

};

void BleTeleinfo::onDisconnection(void)

{

};

void BleTeleinfo::onRSSIChange(int8_t arg_s8_rssi)

{

};
