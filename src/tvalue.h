
//OpenSCADA file: tvalue.h
/***************************************************************************
 *   Copyright (C) 2003-2025 by Roman Savochenko, <roman@oscada.org>       *
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

#ifndef TVALUE_H
#define TVALUE_H

#include <string>
#include <vector>

#include "terror.h"
#include "tvariant.h"
#include "tconfig.h"

using std::string;
using std::vector;

namespace OSCADA
{

//*************************************************
//* TVal                                          *
//*************************************************
class TValue;
class TVArchive;

class TVal: public TCntrNode
{
    public:
	//Data
	enum AttrFlg {
	    DirRead  = 0x100,
	    DirWrite = 0x200,
	    Dynamic  = 0x400,	//Created and can be changed in dynamic way by a procedure,
				//mostly that is provided by the logical sources like DAQ.{LogicLev,ModBus}
	    NoSave   = 0x800	//The attribute can be changed but cannot be saved, that is do not mark its modified
	};

	//Methods
	TVal( );
	TVal( TFld &fld );
	TVal( TCfg &cfg );
	~TVal( );

	string objName( );

	string DAQPath( );

	void setFld( TFld &fld );
	void setCfg( TCfg &cfg );

	string	name( );
	int64_t	time( )		{ return mTime; }
	void	setTime( int64_t vl );
	bool	isCfg( )	{ return mCfg; }
	bool	dataActive( );
	bool	isTransl( ) const { return (fld().type() == TFld::String && fld().flg()&TFld::TransltText && !noTransl()); }
	bool	noTransl( ) const { return mNoTransl; }
	void	setNoTransl( bool vl );

	// Read current value (direct)
	TVariant get( int64_t *tm = NULL, bool sys = false );
	string	getS( int64_t *tm = NULL, bool sys = false );
	double	getR( int64_t *tm = NULL, bool sys = false );
	int64_t	getI( int64_t *tm = NULL, bool sys = false );
	char	getB( int64_t *tm = NULL, bool sys = false );
	AutoHD<TVarObj> getO( int64_t *tm = NULL, bool sys = false );

	// Set current value
	void set( const TVariant &value, int64_t tm = 0, bool sys = false );
	void setS( const string &value, int64_t tm = 0, bool sys = false );
	void setR( double value, int64_t tm = 0, bool sys = false );
	void setI( int64_t value, int64_t tm = 0, bool sys = false );
	void setB( char value, int64_t tm = 0, bool sys = false );
	void setO( AutoHD<TVarObj> value, int64_t tm = 0, bool sys = false );

	AutoHD<TVArchive> arch( );
	void setArch( const AutoHD<TVArchive> &vl );
	string setArch( const string &nm = "" );

	bool reqFlg( )			{ return mReqFlg; }
	void setReqFlg( bool vl )	{ mReqFlg = vl; }

	TValue &owner( ) const;
	TFld &fld( ) const;

    protected:
	//Methods
	void preDisable( int flag );
	void cntrCmdProc( XMLNode *opt );

	TVariant objFuncCall( const string &id, vector<TVariant> &prms, const string &user_lang );

    private:
	//Methods
	const char *nodeName( ) const;

	//Attributes
	union {
	    string	*s;		//String value
	    double	r;		//Real value
	    int64_t	i;		//Integer value
	    char	b;		//Boolean value
	    AutoHD<TVarObj> *o;		//Object value
	} val;

	unsigned char mCfg	: 1;	//Flag of linking the configuration
	unsigned char mNoTransl	: 1;	//No text translation, mostly to disable the dynamic translation and at change from calculation
	unsigned char mReqFlg	: 1;	//Flag of the requesting
	union {
	    TFld *fld;
	    TCfg *cfg;
	} src;
	int64_t mTime;			//Last value's time (usec)
	AutoHD<TVArchive>	mArch;
};

//*************************************************
//* TValue                                        *
//*************************************************
class TValue: public TCntrNode, public TValElem
{
    friend class TVal;

    public:
	TValue( );
	virtual ~TValue( );

	virtual bool dataActive( )	{ return false; }

	string objName( );

	virtual string DAQPath( );

	// Atributes
	void vlList( vector<string> &list ) const	{ chldList(mVl, list); }
	bool vlPresent( const string &name ) const	{ return chldPresent(mVl, name); }
	AutoHD<TVal> vlAt( const string &name ) const	{ return chldAt(mVl, name); }

    protected:
	//Methods
	string chldAdd( int8_t igr, TCntrNode *node, int pos = -1, bool noExp = false );
	void cntrCmdProc( XMLNode *opt );	//Control interface command process

	// Manipulation for configuration element
	TConfig *vlCfg( )			{ return mCfg; }
	void setVlCfg( TConfig *cfg );		//Set configs. NULL - clear configurations.

	// Manipulation for elements of value
	bool vlElemPresent( TElem *ValEl );
	void vlElemAtt( TElem *ValEl );
	void vlElemDet( TElem *ValEl );
	TElem &vlElem( const string &name );

	virtual TVal* vlNew( );
	virtual void vlGet( TVal &vo )		{ };
	virtual void vlSet( TVal &vo, const TVariant &vl, const TVariant &pvl )		{ };
	virtual void vlArchMake( TVal &val )	{ };

	void setNoTransl( bool vl );

    private:
	//Methods
	// TElem commands
	void detElem( TElem *el );
	void addFld( TElem *el, unsigned id_val );
	void delFld( TElem *el, unsigned id_val );

	//Attributes
	char		mVl;
	vector<TElem*>	elem;		// Elements (dinamic parts)

	short int	lCfg;		// Configuration len
	TConfig*	mCfg;		// Configurations (static parts)
};

}

#endif // TVALUE_H
