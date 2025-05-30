
//OpenSCADA file: tconfig.cpp
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

#include "tsys.h"

using namespace OSCADA;

//*************************************************
//* TConfig                                       *
//*************************************************
TConfig::TConfig( TElem *Elements ) : mRes(true), mElem(NULL), mIncmplTblStrct(false), mReqKeys(false), mTrcSet(false), mNoTransl(false)
{
    setElem(Elements, true);
}

TConfig::TConfig( const TConfig &src ) : mRes(true), mElem(NULL), mIncmplTblStrct(false), mReqKeys(false), mTrcSet(false), mNoTransl(false)
{
    setElem(NULL, true);
    operator=(src);
}

TConfig::~TConfig( )
{
    //Deinit value
    TCfgMap::iterator p;
    while((p=value.begin()) != value.end()) {
	delete p->second;
	value.erase(p);
    }

    mElem->valDet(this);
    if(single) delete mElem;
}

TConfig &TConfig::operator=( const TConfig &config )	{ return exclCopy(config, "", true); }

TConfig &TConfig::exclCopy( const TConfig &config, const string &passCpLs, bool cpElsToSingle )
{
    vector<string> listEl;

    //Copy elements for single/builtin elements structure
    if(cpElsToSingle && single && mElem) {
	//?!?!
    }

    cfgList(listEl);
    for(unsigned iEl = 0; iEl < listEl.size(); iEl++) {
	if(!config.cfgPresent(listEl[iEl]) || passCpLs.find(listEl[iEl]+";") != string::npos) continue;
	TCfg &s_cfg = const_cast<TConfig*>(&config)->cfg(listEl[iEl]);
	if(config.trcSet() && !s_cfg.isSet())	continue;
	TCfg &d_cfg = cfg(listEl[iEl]);
	switch(d_cfg.fld().type()) {
	    case TFld::String:
		d_cfg.setExtVal(s_cfg.extVal());
		if(s_cfg.extVal()) d_cfg = s_cfg;
		else d_cfg.setS(s_cfg.getS());
		break;
	    case TFld::Real:	d_cfg.setR(s_cfg.getR());break;
	    case TFld::Integer:	d_cfg.setI(s_cfg.getI());break;
	    case TFld::Boolean:	d_cfg.setB(s_cfg.getB());break;
	    default: break;
	}
    }

    return *this;
}

void TConfig::detElem( TElem *el )
{
    if(el == mElem)	setElem(NULL);
}

void TConfig::addFld( TElem *el, unsigned id )
{
    value.insert(std::pair<string,TCfg*>(mElem->fldAt(id).name(),new TCfg(mElem->fldAt(id),*this)));
}

void TConfig::delFld( TElem *el, unsigned id )
{
    TCfgMap::iterator p = value.find(mElem->fldAt(id).name());
    if(p == value.end()) return;
    delete p->second;
    value.erase(p);
}

void TConfig::reqKeysUpdate( )
{
    vector<string> ls;
    cfgList(ls);
    mReqKeys = false;
    for(unsigned i_c = 0; i_c < ls.size() && !mReqKeys; i_c++) mReqKeys = cfg(ls[i_c]).reqKey();
}

TCfg &TConfig::cfg( const string &n_val ) const
{
    TCfgMap::const_iterator p = value.find(n_val);
    if(p == value.end()) throw TError("TConfig", _("Attribute '%s' is not present!"), n_val.c_str());

    return *p->second;
}

TCfg *TConfig::at( const string &n_val, bool noExpt ) const
{
    TCfgMap::const_iterator p = value.find(n_val);
    if(p != value.end()) return p->second;
    if(noExpt) return NULL;
    throw TError("TConfig", _("Attribute '%s' is not present!"), n_val.c_str());
}

void TConfig::cfgList( vector<string> &list ) const
{
    list.clear();
    if(mElem)	mElem->fldList(list);
}

bool TConfig::cfgPresent( const string &n_val ) const	{ return (value.find(n_val) != value.end()); }

void TConfig::cfgViewAll( bool val )
{
    for(TCfgMap::iterator p = value.begin(); p != value.end(); ++p)
	p->second->setView(val);
}

void TConfig::cfgKeyUseAll( bool val )
{
    for(TCfgMap::iterator p = value.begin(); p != value.end(); ++p)
	if(p->second->fld().flg()&TCfg::Key)
	    p->second->setKeyUse(val);
}

void TConfig::cfgToDefault( )
{
    for(TCfgMap::iterator p = value.begin(); p != value.end(); ++p)
	if(!(p->second->fld().flg()&TCfg::Key) && !p->second->reqKey() && p->second->view())
	    p->second->toDefault(true);
}

void TConfig::setElem( TElem *Elements, bool first )
{
    if(mElem == Elements && !first) return;

    //Clear previos setting
    if(mElem) {
	TCfgMap::iterator p;
	while((p=value.begin()) != value.end()) {
	    delete p->second;
	    value.erase(p);
	}
	mElem->valDet(this);
	if(single) delete mElem;
    }

    //Set new setting
    if(!Elements) {
	mElem = new TElem("single");
	single = true;
    }
    else {
	mElem = Elements;
	single = false;
    }

    mElem->valAtt(this);
    for(unsigned i = 0; i < mElem->fldSize(); i++)
	value.insert(std::pair<string,TCfg*>(mElem->fldAt(i).name(),new TCfg(mElem->fldAt(i),*this)));
}

void TConfig::cntrCmdMake( TCntrNode *cntrO, XMLNode *opt, const string &path, int pos, const string &owner, const string &group, int perm )
{
    vector<string> list_c;
    cfgList(list_c);
    for(unsigned iEl = 0; iEl < list_c.size(); iEl++)
	if(cfg(list_c[iEl]).view())
	    cfg(list_c[iEl]).fld().cntrCmdMake(cntrO, opt, path, (pos<0)?pos:pos++, owner, group, perm);
}

void TConfig::cntrCmdProc( TCntrNode *cntrO, XMLNode *opt, const string &elem, const string &owner, const string &group, int perm )
{
    TCfg &cel = cfg(elem);
    if(cntrO->ctrChkNode(opt,"get",(cel.fld().flg()&TFld::NoWrite)?(perm&~_W_W_W):perm,owner.c_str(),group.c_str(),SEC_RD)) {
	if(cel.isTransl()) opt->setText(trD(cel.getS()));
	else opt->setText(cel.getS());
    }
    if(cntrO->ctrChkNode(opt,"set",(cel.fld().flg()&TFld::NoWrite)?(perm&~_W_W_W):perm,owner.c_str(),group.c_str(),SEC_WR)) {
	if(cel.isTransl()) cel.setS(trDSet(cel.getS(),opt->text()));
	else cel.setS(opt->text());
    }
}

void TConfig::setTrcSet( bool vl )
{
    mTrcSet = vl;

    for(TCfgMap::iterator p = value.begin(); trcSet() && p != value.end(); ++p)
	if(!(p->second->fld().flg()&TCfg::Key) && !p->second->reqKey() && p->second->view())
	    p->second->setIsSet(false);
}

void TConfig::setNoTransl( bool vl )
{
    mNoTransl = vl;

    for(TCfgMap::iterator p = value.begin(); p != value.end(); ++p)
	p->second->setNoTransl(vl);
}

TVariant TConfig::objFunc( const string &iid, vector<TVariant> &prms,
    const string &user_lang, int perm, const string &owner )
{
    // ElTp cfg( string nm ) - config variable 'nm' get.
    //  nm - config variable name.
    if(iid == "cfg" && prms.size() >= 1 &&
	    SYS->security().at().access(TSYS::strLine(user_lang,0),SEC_RD,TSYS::strParse(owner,0,":"),TSYS::strParse(owner,1,":"),perm)) {
	TCfg *cf = at(prms[0].getS(), true);
	if(!cf) return EVAL_REAL;
	if(cf->fld().type() == TFld::String && cf->fld().flg()&TFld::TransltText)
	    return Mess->I18N(cf->getS(), TSYS::strLine(user_lang,1).c_str());
	return *cf;
    }
    // bool cfgSet( string nm, ElTp val ) - set config variable 'nm' to 'val'.
    //  nm - config variable name;
    //  val - variable value.
    if(iid == "cfgSet" && prms.size() >= 2 &&
	    SYS->security().at().access(TSYS::strLine(user_lang,0),SEC_WR,TSYS::strParse(owner,0,":"),TSYS::strParse(owner,1,":"),perm)) {
	TCfg *cf = at(prms[0].getS(), true);
	if(!cf || (cf->fld().flg()&TFld::NoWrite)) return false;
	*(TVariant*)cf = prms[1];
	return true;
    }

    return TVariant();
}

//*************************************************
//* TCfg                                          *
//*************************************************
TCfg::TCfg( TFld &fld, TConfig &iowner ) :
    mView(true), mKeyUse(false), mNoTransl(false), mReqKey(false), mExtVal(false), mInCfgCh(false), mIsSet(false), mOwner(iowner)
{
    //Chek for self field for dinamic elements
    if(fld.flg()&TFld::SelfFld) {
	mFld = new TFld();
	*mFld = fld;
    } else mFld = &fld;

    toDefault();

    if(fld.flg()&TCfg::Hide)	mView = false;
    mNoTransl = owner().noTransl();
}

TCfg::TCfg( const TCfg &src ) :
    mView(true), mKeyUse(false), mNoTransl(false), mReqKey(false), mExtVal(false), mInCfgCh(false), mIsSet(false), mOwner(src.mOwner)
{
    //Chek for self field for dinamic elements
    if(src.mFld->flg()&TFld::SelfFld) {
	mFld = new TFld();
	*mFld = *src.mFld;
    }
    else mFld = src.mFld;

    toDefault();

    if(src.mFld->flg()&TCfg::Hide)	mView = false;
    mNoTransl = owner().noTransl();

    operator=(src);
}

TCfg::~TCfg( )
{
    if(mFld->flg()&TFld::SelfFld)	delete mFld;
}

TCfg &TCfg::operator=( const TCfg &cfg )
{
    switch(type()) {
	case TVariant::String:	TVariant::setS(cfg.TVariant::getS());	break;
	case TVariant::Integer:	TVariant::setI(cfg.TVariant::getI());	break;
	case TVariant::Real:	TVariant::setR(cfg.TVariant::getR());	break;
	case TVariant::Boolean:	TVariant::setB(cfg.TVariant::getB());	break;
	default: break;
    }

    return *this;
}

const string &TCfg::name( )	{ return mFld->name(); }

bool TCfg::isKey( ) const	{ return owner().reqKeys() ? reqKey() : fld().flg()&TCfg::Key; }

void TCfg::setReqKey( bool vl, bool treatDep )
{
    mReqKey = vl;
    if(treatDep) mKeyUse = vl;
    mOwner.reqKeysUpdate();
}

void TCfg::setExtVal( bool vw, bool toOne )
{
    if(!vw) {
	string fVl = toOne ? getS(TCfg::ExtValOne) : getS();
	mExtVal = vw;
	TVariant::setS(fVl);
    } else mExtVal = vw;
}

void TCfg::toDefault( bool notSetType )
{
    if(!mFld)	return;

    switch(mFld->type()) {
	case TFld::String:
	    if(!notSetType) setType(TVariant::String, true, (mFld->flg()&TCfg::Key));
	    TVariant::setS(mFld->def());
	    break;
	case TFld::Integer:
	    if(!notSetType) setType(TVariant::Integer, true);
	    TVariant::setI(s2ll(mFld->def()));
	    break;
	case TFld::Real:
	    if(!notSetType) setType(TVariant::Real, true);
	    TVariant::setR(s2r(mFld->def()));
	    break;
	case TFld::Boolean:
	    if(!notSetType) setType(TVariant::Boolean, true);
	    TVariant::setB((bool)s2i(mFld->def()));
	    break;
	default: break;
    }
}

string TCfg::getS( ) const
{
    mOwner.mRes.lock();
    string rez = TVariant::getS();
    mOwner.mRes.unlock();
    if(!extVal()) return rez;
    else {
	if(isTransl()) {
	    string rezT = TSYS::strSepParse(rez, 1, 0), rezSrc = TSYS::strSepParse(rez, 2, 0);
	    rez = TSYS::strSepParse(rez, 0, 0);
	    if(rez.size() && rezSrc.size()) Mess->translReg(rez, rezSrc);	//!!!! Can be very busy
	    return rezT.size() ? rezT : rez;
	}
	else return TSYS::strSepParse(rez, 0, 0);
    }
}

string TCfg::getS( uint8_t RqFlg )
{
    if(extVal() && RqFlg&(ExtValOne|ExtValTwo|ExtValThree)) {
	mOwner.mRes.lock();
	string rez = TVariant::getS();
	mOwner.mRes.unlock();

	return TSYS::strSepParse(rez, ((RqFlg&ExtValTwo)?1:((RqFlg&ExtValThree)?2:0)), 0);
    }
    return getS();
}

const char *TCfg::getSd( )
{
    if(type() != TVariant::String)	throw TError("Cfg", _("Element type is not String!"));
    if(mStdString)	return val.s->c_str();
    if(mSize < sizeof(val.sMini)) return val.sMini;
    return val.sPtr;
}

double &TCfg::getRd( )
{
    if(type() != TVariant::Real)	throw TError("Cfg", _("Element type is not Real!"));
    return val.r;
}

int64_t &TCfg::getId( )
{
    if(type() != TVariant::Integer)	throw TError("Cfg", _("Element type is not Integer!"));
    return val.i;
}

char &TCfg::getBd( )
{
    if(type() != TVariant::Boolean)	throw TError("Cfg", _("Element type is not Boolean!"));
    return val.b;
}

void TCfg::setS( const string &ival )
{
    switch(type()) {
	case TVariant::Integer:	setI((ival==EVAL_STR) ? EVAL_INT : s2ll(ival));	break;
	case TVariant::Real:	setR((ival==EVAL_STR) ? EVAL_REAL : s2r(ival));	break;
	case TVariant::Boolean:	setB((ival==EVAL_STR) ? EVAL_BOOL : (bool)s2i(ival));	break;
	case TVariant::String: {
	    mOwner.mRes.lock();
	    string tVal = TVariant::getS();
	    if(extVal() && isTransl() && ival.find(char(0)) == string::npos) {
		if(Mess->langCode() == Mess->langCodeBase()) TVariant::setS(ival+string(2,0)+getS(ExtValThree));
		else TVariant::setS(getS(ExtValOne)+string(1,0)+ival+string(1,0)+getS(ExtValThree));
	    }
	    else if(fld().flg()&TCfg::Key) TVariant::setS(TSYS::strEncode(ival,TSYS::Limit,i2s(fld().len())));
	    else TVariant::setS(ival);
	    mOwner.mRes.unlock();
	    bool mInCfgCh_ = mInCfgCh;
	    try {
		if(!mInCfgCh && (mInCfgCh=true) && !mOwner.cfgChange(*this,tVal)) {
		    mOwner.mRes.lock();
		    TVariant::setS(tVal);
		    mOwner.mRes.unlock();
		}
		if(!mInCfgCh_)	mInCfgCh = false;
		if(mOwner.trcSet())	setIsSet(true);
	    } catch(TError &err) {
		if(!mInCfgCh_)	mInCfgCh = false;
		mOwner.mRes.lock();
		TVariant::setS(tVal);
		mOwner.mRes.unlock();
		throw;
	    }
	    break;
	}
	default: break;
    }
}

void TCfg::setR( double ival )
{
    switch(type()) {
	case TVariant::String:	setS((ival==EVAL_REAL) ? EVAL_STR : r2s(ival));		break;
	case TVariant::Integer:	setI((ival==EVAL_REAL) ? EVAL_INT : (int64_t)ival);	break;
	case TVariant::Boolean:	setB((ival==EVAL_REAL) ? EVAL_BOOL : (bool)ival);	break;
	case TVariant::Real: {
	    double tMinV = 0, tMaxV = 0;
	    if(!(mFld->flg()&TFld::Selectable) && mFld->values().size() &&
		    (tMinV=s2r(TSYS::strParse(mFld->values(),0,";"))) < (tMaxV=s2r(TSYS::strParse(mFld->values(),1,";"))))
		ival = vmin(tMaxV, vmax(tMinV,ival));
	    double tVal = TVariant::getR();
	    TVariant::setR(ival);
	    bool mInCfgCh_ = mInCfgCh;
	    try {
		if(!mInCfgCh && (mInCfgCh=true) && !mOwner.cfgChange(*this,tVal)) TVariant::setR(tVal);
		if(!mInCfgCh_)	mInCfgCh = false;
		if(mOwner.trcSet())	setIsSet(true);
	    }
	    catch(TError &err) {
		if(!mInCfgCh_)	mInCfgCh = false;
		TVariant::setR(tVal);
		throw;
	    }
	    break;
	}
	default: break;
    }
}

void TCfg::setI( int64_t ival )
{
    switch(type()) {
	case TVariant::String:	setS((ival==EVAL_INT) ? EVAL_STR : ll2s(ival));	break;
	case TVariant::Real:	setR((ival==EVAL_INT) ? EVAL_REAL : ival);	break;
	case TVariant::Boolean:	setB((ival==EVAL_INT) ? EVAL_BOOL : (bool)ival);break;
	case TVariant::Integer: {
	    int64_t tMinV = 0, tMaxV = 0;
	    if(!(mFld->flg()&TFld::Selectable) && mFld->values().size() &&
		    (tMinV=s2ll(TSYS::strParse(mFld->values(),0,";"))) < (tMaxV=s2ll(TSYS::strParse(mFld->values(),1,";"))))
		ival = vmin(tMaxV, vmax(tMinV,ival));
	    int64_t tVal = TVariant::getI();
	    TVariant::setI(ival);
	    bool mInCfgCh_ = mInCfgCh;
	    try {
		if(!mInCfgCh && (mInCfgCh=true) && !mOwner.cfgChange(*this,tVal)) TVariant::setI(tVal);
		if(!mInCfgCh_)	mInCfgCh = false;
		if(mOwner.trcSet())	setIsSet(true);
	    }
	    catch(TError &err) {
		if(!mInCfgCh_)	mInCfgCh = false;
		TVariant::setI(tVal);
		throw;
	    }
	    break;
	}
	default: break;
    }
}

void TCfg::setB( char ival )
{
    switch(type()) {
	case TVariant::String:	setS((ival==EVAL_BOOL) ? EVAL_STR : i2s(ival));	break;
	case TVariant::Integer:	setI((ival==EVAL_BOOL) ? EVAL_INT : ival);	break;
	case TVariant::Real:	setR((ival==EVAL_BOOL) ? EVAL_REAL : ival);	break;
	case TVariant::Boolean: {
	    bool tVal = TVariant::getB();
	    TVariant::setB(ival);
	    bool mInCfgCh_ = mInCfgCh;
	    try {
		if(!mInCfgCh && (mInCfgCh=true) && !mOwner.cfgChange(*this,tVal)) TVariant::setB(tVal);
		if(!mInCfgCh_)	mInCfgCh = false;
		if(mOwner.trcSet())	setIsSet(true);
	    }
	    catch(TError &err) {
		if(!mInCfgCh_)	mInCfgCh = false;
		TVariant::setB(tVal);
		throw;
	    }
	    break;
	}
	default: break;
    }
}

void TCfg::setS( const string &ival, uint8_t RqFlg )
{
    if(!extVal() && (RqFlg&(ExtValTwo|ExtValOne|ExtValThree))) {
	mExtVal = true;
	mOwner.mRes.lock();
	setType(TVariant::String);
	mOwner.mRes.unlock();
    }
    if(!extVal()) setS(ival);
    else {
	mOwner.mRes.lock();
	string tVal = TVariant::getS();
	TVariant::setS(((RqFlg&ExtValOne || !(RqFlg&(ExtValTwo|ExtValThree)))?ival:getS(ExtValOne))+string(1,0)+
		((RqFlg&ExtValTwo)?ival:getS(ExtValTwo))+string(1,0)+
		((RqFlg&ExtValThree)?ival:getS(ExtValThree)));
	mOwner.mRes.unlock();

	if(RqFlg&(ExtValOne|ExtValTwo)) {	//!!!! Notify only on the data stages and without the returning back
	    bool mInCfgCh_ = mInCfgCh;
	    try {
		if(!mInCfgCh && (mInCfgCh=true) && !mOwner.cfgChange(*this,tVal)) /* return back */ ;
	    } catch(TError &err) { /* return back */ }
	    if(!mInCfgCh_)	mInCfgCh = false;
	}

	if(mOwner.trcSet())	setIsSet(true);
    }
    if(RqFlg&TCfg::ForceUse)	{ setView(true); setKeyUse(true); }
}

void TCfg::setR( double ival, uint8_t RqFlg )
{
    setR(ival);
    if(RqFlg&TCfg::ForceUse)	{ setView(true); setKeyUse(true); }
}

void TCfg::setI( int64_t ival, uint8_t RqFlg )
{
    setI(ival);
    if(RqFlg&TCfg::ForceUse)	{ setView(true); setKeyUse(true); }
}

void TCfg::setB( char ival, uint8_t RqFlg )
{
    setB(ival);
    if(RqFlg&TCfg::ForceUse)	{ setView(true); setKeyUse(true); }
}

bool TCfg::operator==( TCfg &cfg )
{
    if(fld().type() == cfg.fld().type())
	switch(fld().type()) {
	    case TFld::String:	return (getS() == cfg.getS());
	    case TFld::Integer:	return (getI() == cfg.getI());
	    case TFld::Real:	return (getR() == cfg.getR());
	    case TFld::Boolean:	return (getB() == cfg.getB());
	    default: break;
	}
    return false;
}
