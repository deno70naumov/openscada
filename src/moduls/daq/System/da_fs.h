
//OpenSCADA module DAQ.System file: da_fs.h
/***************************************************************************
 *   Copyright (C) 2016-2023 by Roman Savochenko, <roman@oscada.org>       *
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

#ifndef DA_FS_H
#define DA_FS_H

#include "da.h"

namespace SystemCntr
{

//*************************************************
//* FS                                            *
//*************************************************
class FS: public DA
{
    public:
	//Methods
	FS( );
	~FS( );

	string id( )	{ return "fs"; }
	string name( )	{ return _("File System"); }

	void getVal( TMdPrm *prm );

	void makeActiveDA( TMdContr *aCntr, const string &dIdPref = "", const string &dNmPref = "" )
	{ DA::makeActiveDA(aCntr, "FS", name()); }

    private:
	//Methods
	void dList( vector<string> &list, TMdPrm *prm = NULL );
};

} //End namespace

#endif //DA_FS_H
