/******************************************************************************
 * @file    teleinfo_listener.h 
 * @author  Rémi Pincent - INRIA
 * @date    29 nov. 2015   
 *
 * @brief Listener on teleinfo
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
#ifndef TELEINFO_TELEINFO_LISTENER_H_
#define TELEINFO_TELEINFO_LISTENER_H_

#include "teleinfo_fields.h"

class ITeleinfoListener {
	public :
	virtual ~ITeleinfoListener(void){};
	virtual void hubAddrChanged(char* arg_hubAddr) = 0;
	virtual void optTarChanged(EOptTar arg_e_optTar) = 0;
	virtual void baseIndexChanged(uint32_t arg_u32_baseIndex) = 0;
	virtual void hcIndexChanged(uint32_t arg_u32_hcIndex) = 0;
	virtual void hpIndexChanged(uint32_t arg_u32_hpIndex) = 0;
	virtual void ejpHMIChanged(uint32_t arg_u32_ejpHMI) = 0;
	virtual void ejpHPMIChanged(uint32_t arg_u32_ejpHPMI) = 0;
	virtual void ejpMessChanged(uint8_t arg_u8_ejpMess) = 0;
	virtual void gazIndexChanged(uint32_t arg_u32_gazIndex) = 0;
	virtual void currTarChanged(EPTEC arg_e_currTar) = 0;
	virtual void modEtatChanged(char* arg_modEtat) = 0;
	virtual void instIntChanged(uint16_t arg_u16_instInt) = 0;
	virtual void maxIntChanged(uint16_t arg_u16_maxInt) = 0;
	virtual void souscIntChanged(uint16_t arg_u16_souscInt) = 0;
	virtual void appPowerChanged(uint16_t arg_u32_appPower) = 0;
	virtual void hhphcChanged(char arg_s8_hhphc) = 0;;
};

#endif /* TELEINFO_TELEINFO_LISTENER_H_ */
