/******************************************************************************
 * @file    ble_teleinfo.h 
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
#ifndef BLE_TELEINFO_H_
#define BLE_TELEINFO_H_

/**************************************************************************
 * Include Files
 **************************************************************************/
#include "ac_ble_transceiver.h"
#include "teleinfo.h"
#include <EventManager.h>
#include <timer.h>

class BleTeleinfo :     public ITeleinfoListener,
						public TimerListener,
						public IBleTransceiverListener
{
private :
	enum TeleinfoType : uint8_t
	{
		IINST = 0,
		APP_POWER = 1
	};

private:
	BLETransceiver* _p_bleTransceiver;
	Timer _timer;
	Teleinfo _teleinfo;

public:
	BleTeleinfo(BLETransceiver& arg_p_bleTransceiver);
	~BleTeleinfo(void);
	void start(void);

private:
	/** from ITeleinfoListener */
	void hubAddrChanged(char* arg_hubAddr);
	void optTarChanged(EOptTar arg_e_optTar);
	void baseIndexChanged(uint32_t arg_u32_baseIndex);
	void hcIndexChanged(uint32_t arg_u32_hcIndex);
	void hpIndexChanged(uint32_t arg_u32_hpIndex);
	void ejpHMIChanged(uint32_t arg_u32_ejpHMI);
	void ejpHPMIChanged(uint32_t arg_u32_ejpHPMI);
	void ejpMessChanged(uint8_t arg_u8_ejpMess);
	void gazIndexChanged(uint32_t arg_u32_gazIndex);
	void currTarChanged(EPTEC arg_e_currTar);
	void modEtatChanged(char* arg_modEtat);
	void instIntChanged(uint16_t arg_u16_instInt);
	void maxIntChanged(uint16_t arg_u16_maxInt);
	void souscIntChanged(uint16_t arg_u16_souscInt);
	void appPowerChanged(uint32_t arg_u32_appPower);
	void hhphcChanged(char arg_s8_hhphc);

	/** from TimerListener */
	void timerElapsed(void);

	/** from TimerListener */
	void onDataReceived(uint8_t arg_u8_dataLength, uint8_t arg_au8_data[]);
	void onConnection(void);
	void onDisconnection(void);
	void onRSSIChange(int8_t arg_s8_rssi);
};

#endif /* BLE_TELEINFO_H_ */
