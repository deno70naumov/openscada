
//OpenSCADA module DAQ.System file: da_sensors.h
/***************************************************************************
 *   Copyright (C) 2005-2024 by Roman Savochenko, <roman@oscada.org>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef DA_SENSORS_H
#define DA_SENSORS_H

#include "da.h"

#define DIR_VDEV "/sys/devices/virtual/"

namespace SystemCntr
{

//*************************************************
//* Sensors                                       *
//*************************************************
class Sensors: public DA
{
    public:
	//Methods
	Sensors( );
	~Sensors( );

	bool hasSubTypes( )	{ return false; }

	string id( )	{ return "sensors"; }
	string name( )	{ return _("Sensors"); }

	void getVal( TMdPrm *prm );

	void makeActiveDA( TMdContr *aCntr, const string &dIdPref = "", const string &dNmPref = "" );

    private:
	//Attributes
	static const char *mbmon_cmd;
	bool libsensor_ok;

	//Methods
	bool devChkAccess( const string &file, const string &acs = "r" );
	string devRead( const string &file );
};

} //End namespace

#endif //DA_SENSORS_H
