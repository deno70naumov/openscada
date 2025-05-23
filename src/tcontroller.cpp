
//OpenSCADA file: tcontroller.cpp
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
#include "tmess.h"
#include "ttypeparam.h"
#include "tcontroller.h"

#define DEF_messTm	"0"
#define DEF_messSize	"60"

using namespace OSCADA;

//*************************************************
//* TController					  *
//*************************************************
TController::TController( const string &id_c, const string &daq_db, TElem *cfgelem ) :
    TConfig(cfgelem), enSt(false), runSt(false),
    mId(cfg("ID")), mMessLev(cfg("MESS_LEV")), mAEn(cfg("ENABLE").getBd()), mAStart(cfg("START").getBd()),
    mDB(daq_db), mRdSt(dataRes()), mRdUse(true), mRdFirst(true)
{
    mId = id_c;
    mPrm = grpAdd("prm_");

    if(mess_lev() == TMess::Debug) SYS->cntrIter(objName(), 1);
}

TController::~TController( )
{
    nodeDelAll();

    if(mess_lev() == TMess::Debug) SYS->cntrIter(objName(), -1);
}

string TController::objName( )	{ return TCntrNode::objName()+":TController"; }

string TController::DAQPath( )	{ return owner().DAQPath()+"."+id(); }

TCntrNode &TController::operator=( const TCntrNode &node )
{
    const TController *src_n = dynamic_cast<const TController*>(&node);
    if(!src_n) return *this;

    //Individual DB names store - ????[v1.0] Remove
    vector<string> dbNms;
    for(unsigned iTp = 0; iTp < owner().tpPrmSize(); iTp++)
	dbNms.push_back(owner().tpPrmAt(iTp).DB(this));

    //Configuration copy
    exclCopy(*src_n, "ID;");
    setDB(src_n->DB());

    //Individual DB names restore - ????[v1.0] Remove
    for(unsigned iTp = 0; iTp < owner().tpPrmSize() && iTp < dbNms.size(); iTp++)
	owner().tpPrmAt(iTp).setDB(this, dbNms[iTp]);

    //Parameters copy
    if(src_n->enableStat()) {
	if(!enableStat()) enable();
	vector<string> prmLs;
	src_n->list(prmLs);
	for(unsigned iP = 0; iP < prmLs.size(); iP++) {
	    if(!owner().tpPrmPresent(src_n->at(prmLs[iP]).at().type().name)) continue;
	    if(!present(prmLs[iP])) add(prmLs[iP], owner().tpPrmToId(src_n->at(prmLs[iP]).at().type().name));
	    (TCntrNode&)at(prmLs[iP]).at() = (TCntrNode&)src_n->at(prmLs[iP]).at();
	    //if(toEnable() && !enableStat()) enable();
	}
    }

    return *this;
}

void TController::preDisable( int flag )
{
    if(startStat())	stop();
    if(enableStat())	disable();
}

void TController::postDisable( int flag )
{
    if(flag&(NodeRemove|NodeRemoveOnlyStor)) {
	//Delete DB record
	TBDS::dataDel(fullDB(flag&NodeRemoveOnlyStor), owner().nodePath()+"DAQ", *this, TBDS::UseAllKeys);

	//Delete parameter tables
	for(unsigned iTp = 0; iTp < owner().tpPrmSize(); iTp++)
	    TBDS::dataDelTbl(DB(flag&NodeRemoveOnlyStor)+"."+tbl(owner().tpPrmAt(iTp)),
				owner().nodePath()+tbl(owner().tpPrmAt(iTp)));

	if(flag&NodeRemoveOnlyStor) { setStorage(mDB, "", true); return; }
    }
}

TTypeDAQ &TController::owner( ) const	{ return *(TTypeDAQ*)nodePrev(); }

string TController::workId( )	{ return owner().modId()+"."+id(); }

string TController::name( )	{ string nm = cfg("NAME").getS(); return nm.empty() ? id() : nm; }

void TController::setName( const string &nm )		{ cfg("NAME").setS(nm);  }

string TController::descr( )	{ return cfg("DESCR").getS(); }

void TController::setDescr( const string &dscr )	{ cfg("DESCR").setS(dscr); }

int64_t TController::timeStamp( )
{
    int64_t mTimeStamp = 0;

    vector<string> ls;
    list(ls);
    for(unsigned iL = 0; iL < ls.size(); ++iL)
	mTimeStamp = vmax(mTimeStamp, at(ls[iL]).at().timeStamp());

    return mTimeStamp;
}

string TController::tbl( ) const			{ return owner().owner().subId()+"_"+owner().modId(); }

string TController::tbl( const TTypeParam &tP ) const	{ return tP.DB(this); }

string TController::tblStd( const TTypeParam &tP ) const{ return owner().modId()+tP.name+"_"+id(); }

string TController::getStatus( )
{
    string rez, mess;
    if(startStat()) {
	rez = string("0:")+_("Running. ");
	if(owner().redntAllow() && redntUse()) {
	    mess = _("Acquisition data from a remote station: ");
	    string rSt = mRdSt.getVal();
	    if(!rSt.empty()) {
		if(rSt.find(mess) == string::npos) {
		    int rOff = 0;
		    rez.replace(0, 1, TSYS::strSepParse(rSt,0,':',&rOff));
		    mess.append(rSt.substr(rOff));
		}
		else mess = _("Your redundancy settings are incorrect and the controller often enables-disables the redundancy!");
	    }
	    rez += mess;
	}
    }
    else if(enableStat()) rez = string("1:")+_("Enabled. ");
    else rez = string("2:")+_("Disabled. ");

    return rez;
}

void TController::load_( TConfig *icfg )
{
    if(!SYS->chkSelDB(DB())) throw TError();
    mess_sys(TMess::Info, _("Controller configuration loading."));

    bool enSt_prev = enSt, runSt_prev = runSt;

    if(icfg) *(TConfig*)this = *icfg;
    else {
	//cfgViewAll(true);
	TBDS::dataGet(fullDB(), owner().nodePath()+"DAQ", *this);
    }

    mRdUse = owner().redntAllow() && (bool)redntMode();

    LoadParmCfg();

    if(!enSt && enSt_prev)	enable();
    if(!runSt && runSt_prev)	start();
}

void TController::save_( )
{
    mess_sys(TMess::Info, _("Controller configuration saving."));

    //Update type controller bd record
    TBDS::dataSet(fullDB(), owner().nodePath()+"DAQ", *this);
    setDB(DB(), true);
}

void TController::start( )
{
    //Enable if no enabled
    if(runSt)	return;
    if(!enSt)	enable();

    mess_sys(TMess::Info, _("Controller starting."));

    //First archives synchronization
    if(owner().redntAllow() && redntMode()) redntDataUpdate();

    //Start for children
    start_();

    runSt = true;
}

void TController::stop( )
{
    if(!runSt)	return;

    mess_sys(TMess::Info, _("Controller stopping."));

    //Stop for children
    stop_();

    runSt = false;
}

void TController::enable( )
{
    if(!enSt) {
	mess_sys(TMess::Info, _("Controller enabling."));

	//Enable for children
	enable_();
    }

    bool enErr = false;
    //Enable parameters
    vector<string> prm_list;
    list(prm_list);
    for(unsigned iPrm = 0; iPrm < prm_list.size(); iPrm++)
	if(at(prm_list[iPrm]).at().toEnable())
	    try{ at(prm_list[iPrm]).at().enable(); }
	    catch(TError &err) {
		mess_warning(err.cat.c_str(), "%s", err.mess.c_str());
		mess_sys(TMess::Warning, _("Error turning on the parameter '%s'."), prm_list[iPrm].c_str());
		enErr = true;
	    }

    //Set enable stat flag
    enSt = true;

    if(enErr) throw err_sys(_("Error turning on some parameters."));
}

void TController::disable( )
{
    if(!enSt) return;

    //Stop if runed
    if(runSt) stop();

    mess_sys(TMess::Info, _("Controller disabling."));

    //Disable parameters
    vector<string> prm_list;
    list(prm_list);
    for(unsigned iPrm = 0; iPrm < prm_list.size(); iPrm++)
	if(at(prm_list[iPrm]).at().enableStat())
	    try{ at(prm_list[iPrm]).at().disable(); }
	    catch(TError &err) {
		mess_warning(err.cat.c_str(), "%s", err.mess.c_str());
		mess_sys(TMess::Warning, _("Error turning off the parameter '%s'."), prm_list[iPrm].c_str());
	    }

    //Disable for children
    disable_();

    //Clear enable flag
    enSt = false;
}

void TController::LoadParmCfg( )
{
    map<string, bool>	itReg;

    //Search and create new parameters
    for(unsigned iTp = 0; iTp < owner().tpPrmSize(); iTp++) {
	if(tbl(owner().tpPrmAt(iTp)).empty()) continue;
	try {
	    TConfig cEl(&owner().tpPrmAt(iTp));
	    //cEl.cfgViewAll(false);
	    cEl.cfg("OWNER").setS("", TCfg::ForceUse);

	    // Search new one in DB and Config-file
	    for(int fldCnt = 0; TBDS::dataSeek(DB()+"."+tbl(owner().tpPrmAt(iTp)),
					owner().nodePath()+tbl(owner().tpPrmAt(iTp)),fldCnt++,cEl,TBDS::UseCache); )
	    {
		try {
		    string shfr = cEl.cfg("SHIFR").getS();
		    if(!present(shfr))	add(shfr, iTp);
		    at(shfr).at().load(&cEl);
		    itReg[shfr] = true;
		} catch(TError &err) {
		    mess_err(err.cat.c_str(), "%s", err.mess.c_str());
		    mess_sys(TMess::Warning, _("Error adding the parameter '%s'."), cEl.cfg("SHIFR").getS().c_str());
		}
	    }
	} catch(TError &err) {
	    mess_err(err.cat.c_str(),"%s",err.mess.c_str());
	    mess_sys(TMess::Error, _("Error finding and creating new parameters."));
	}
    }

    //Check for remove items removed from DB
    if(SYS->chkSelDB(SYS->selDB(),true)) {
	vector<string> itLs;
	list(itLs);
	for(unsigned iIt = 0; iIt < itLs.size(); iIt++)
	    if(itReg.find(itLs[iIt]) == itReg.end())
		del(itLs[iIt]);
    }

    //Force load present parameters
    vector<string> prmLs;
    list(prmLs);
    for(unsigned iP = 0; iP < prmLs.size(); iP++) {
	at(prmLs[iP]).at().modifG();
	at(prmLs[iP]).at().load();
    }
}

string TController::add( const string &iid, unsigned type )
{
    return chldAdd(mPrm, ParamAttach(TSYS::strEncode(sTrm(iid),TSYS::oscdID),type));
}

TParamContr *TController::ParamAttach( const string &iid, int type)	{ return new TParamContr(iid, &owner().tpPrmAt(type)); }

void TController::redntDataUpdate( )
{
    vector<RedntStkEl> hst;

    //Prepare a group of a hierarchy request to the parameters
    AutoHD<TParamContr> prm, prmC;
    XMLNode req("CntrReqs"); req.setAttr("path", nodePath());
    req.childAdd("get")->setAttr("path", "/%2fcntr%2fst%2fstatus");

    hst.push_back(RedntStkEl());
    list(hst.back().ls);
    string addr;
    while(true) {
	if(hst.back().pos >= hst.back().ls.size()) {
	    if(!hst.back().addr.size()) break;
	    hst.pop_back(); hst.back().pos++;
	    prm = AutoHD<TParamContr>(hst.back().addr.size()?dynamic_cast<TParamContr*>(prm.at().nodePrev(true)):NULL);
	    continue;
	}
	prmC = prm.freeStat() ? at(hst.back().ls[hst.back().pos]) : prm.at().at(hst.back().ls[hst.back().pos]);
	addr = hst.back().addr + "/prm_"+hst.back().ls[hst.back().pos];
	if(prmC.at().enableStat()) {
	    XMLNode *prmNd = req.childAdd("get")->setAttr("path", addr + "/%2fserv%2fattr");

	    // Prepare individual attributes list
	    prmNd->setAttr("sepReq", "1")->setAttr("prcTm", i2s(prmC.at().mRdPrcTm));

	    // Check attributes last presence data time in archives
	    vector<string> listV;
	    prmC.at().vlList(listV);
	    unsigned rC = 0;
	    for(unsigned iV = 0; iV < listV.size(); iV++) {
		AutoHD<TVal> vl = prmC.at().vlAt(listV[iV]);
		if(!vl.at().arch().freeStat())
		    prmNd->childAdd("ael")->setAttr("id",listV[iV])->
					    setAttr("tm",ll2s(vmax(vl.at().arch().at().end(""),
								   TSYS::curTime()-(int64_t)(3.6e9*owner().owner().rdRestDtTm()))));
		if(!vl.at().arch().freeStat() || vl.at().reqFlg()) { prmNd->childAdd("el")->setAttr("id",listV[iV]); rC++; }
	    }
	    if(rC > listV.size()/2) { prmNd->childClear("el"); prmNd->setAttr("sepReq", ""); }
	    //if(s2i(prmNd->attr("sepReq")) && !prmNd->childSize()) req.childDel(prmNd);
	}
	hst.push_back(RedntStkEl(addr));
	prmC.at().list(hst.back().ls);
	prm = prmC;
    }

    //Send request to the first active station for this controller object
    if(owner().owner().rdStRequest(workId(),req,"",!mRdFirst).empty()) return;
    mRdFirst = false;

    //Write the requested data to the parameters
    for(unsigned iP = 0; iP < req.childSize(); iP++) {
	XMLNode *p = req.childGet(iP);
	addr = p->attr("path");
	if(addr == "/%2fcntr%2fst%2fstatus") { mRdSt.setVal(p->text()); continue; }	//???? Move to the synchronous request in getStatus()
	size_t aPos = addr.rfind("/"); addr = (aPos == string::npos) ? "" : addr.substr(0, aPos);
	if((prm=nodeAt(addr,0,0,0,true)).freeStat()) continue;
	prm.at().mRdPrcTm = s2i(p->attr("prcTm"));
	for(unsigned iA = 0; iA < p->childSize(); iA++) {
	    XMLNode *aNd = p->childGet(iA);
	    AutoHD<TVal> vl;
	    if(prm.at().vlPresent(aNd->attr("id"))) vl = prm.at().vlAt(aNd->attr("id"));

	    try {
		if(aNd->name() == "el" && !vl.freeStat()) { vl.at().setS(aNd->text(),atoll(aNd->attr("tm").c_str()),true); vl.at().setReqFlg(false); }
		else if(aNd->name() == "ael" && !vl.freeStat() && !vl.at().arch().freeStat() && aNd->childSize()) {
		    int64_t btm = atoll(aNd->attr("tm").c_str());
		    int64_t per = atoll(aNd->attr("per").c_str());
		    TValBuf buf(vl.at().arch().at().valType(), 0, per, false, true);
		    for(unsigned iV = 0; iV < aNd->childSize(); iV++)
			buf.setS(aNd->childGet(iV)->text(),btm+per*iV);
		    vl.at().arch().at().setVals(buf, buf.begin(), buf.end(), "");
		}
		else if(aNd->name() == "del" && prm.at().dynElCntr()) {
		    MtxAlloc res(prm.at().dynElCntr()->resEl(), true);
		    TFld::Type tp = (TFld::Type)s2i(aNd->attr("type"));
		    unsigned flg = s2i(aNd->attr("flg"));
		    if(vl.freeStat()) prm.at().dynElCntr()->fldAdd(new TFld(aNd->attr("id").c_str(),aNd->attr("name").c_str(),tp,flg,"","",
									    aNd->attr("values"),aNd->attr("selNames")));
		    else {
			unsigned aId = prm.at().dynElCntr()->fldId(aNd->attr("id"), true);
			prm.at().dynElCntr()->fldAt(aId).setDescr(aNd->attr("name"));
			prm.at().dynElCntr()->fldAt(aId).setFlg(prm.at().dynElCntr()->fldAt(aId).flg()^((prm.at().dynElCntr()->fldAt(aId).flg()^flg)&(TFld::Selectable|TFld::SelEdit)));
			prm.at().dynElCntr()->fldAt(aId).setValues(aNd->attr("values"));
			prm.at().dynElCntr()->fldAt(aId).setSelNames(aNd->attr("selNames"));
		    }
		}
	    } catch(TError &err) { mess_err(err.cat.c_str(), "%s", err.mess.c_str()); }
	}
    }
}

string TController::catsPat( )
{
    return "^[a-zA-Z]{2}"+owner().modId()+":"+id()+"(\\.|$)|^"+nodePath();
    //return "^al"+owner().modId()+":"+id()+"(\\.|$)|^"+nodePath();
    //return "/^(al"+owner().modId()+":"+id()+"(\\.|$)|"+nodePath()+")/";
}

void TController::messSet( const string &mess, int lev, const string &type2Code, const string &prm, const string &cat )
{
    string pId = TSYS::strLine(prm, 0);
    string pNm = TSYS::strLine(prm, 1);
    string aCat = type2Code + owner().modId() + ":" + id() + (pId.size()?"."+pId:"") + (cat.size()?":"+cat:"");
    string aMess = (pNm.size()?name()+" > "+pNm+": ":"") + mess;

    message(aCat.c_str(), lev, aMess.c_str());
}

void TController::alarmSet( const string &mess, int lev, const string &prm, bool force )
{
    if(force || !redntUse(TController::Any)) {
	string	pId = TSYS::strLine(prm, 0);
	string	pNm = TSYS::strLine(prm, 1);
	string	aCat = "al" + owner().modId() + ":" + id();
	if(pId.size()) aCat += "." + pId;
	string	aMess = mess;
	if(aMess.size()) {
	    pId = name();
	    if(pNm.size()) pId += " > " + pNm;
	    aMess = pId + ((prm.size() && !pNm.size())?" > ":": ") + aMess;
	}

	//Checking for presence at set and missing at unset
	if(!force) {
	    vector<TMess::SRec> recs;
	    SYS->archive().at().messGet(0, TSYS::curTime()/1000000, recs, aCat, -1, ARCH_ALRM);	//!!!! curTime() and not sysTm() due to it using in the message mark
	    if((lev >= 0 && !recs.size()) || (lev < 0 && recs.size() && aMess == recs[0].mess && lev == recs[0].level))
		return;
	}

	message(aCat.c_str(), lev, aMess.c_str());
    }
}

TVariant TController::objFuncCall( const string &iid, vector<TVariant> &prms, const string &user_lang )
{
    // string name( ) - get controller name.
    if(iid == "name")	return name();
    // string descr( ) - get controller description.
    if(iid == "descr")	return descr();
    // string status( ) - get controller status.
    if(iid == "status")	return getStatus();
    // bool messSet( string mess, int lev, string type2Code = "OP", string prm = "", string cat = "") -
    //		sets of the DAQ-sourced message <mess> with the level <lev>, for the parameter <prm> ({PrmId}),
    //		additional category information <cat> and the type code <type2Code>.
    //		This function forms the messages with the unified DAQ-transparency category "{type2Code}{ModId}:{CntrId}[.{PrmId}][:{cat}]"
    if(iid == "messSet" && prms.size() >= 2) {
	messSet(prms[0].getS(), prms[1].getI(), ((prms.size()>=3)?prms[2].getS():"OP"),
	    ((prms.size()>=4)?prms[3].getS():""), ((prms.size()>=5)?prms[4].getS():""));
	return true;
    }
    // bool alarmSet( string mess, int lev = -5, string prm = "", bool force = false ) -
    //		sets/removes of the violation <mess> with the level <lev> (negative to set otherwise to remove), for the parameter <prm> ({PrmId}\n{PrmNm}).
    //		The function forms the alarms with the category "al{ModId}:{CntrId}[.{PrmId}]" and the text "{CntrNm} > {PrmNm}: {MessText}".
    if(iid == "alarmSet" && prms.size() >= 1) {
	alarmSet(prms[0].getS(), (prms.size() >= 2) ? prms[1].getI() : -TMess::Crit,
	    (prms.size() >= 3) ? prms[2].getS() : "", (prms.size() >= 4) ? prms[3].getB() : false);
	return true;
    }
    // bool enable( bool newSt = EVAL ) - get enable status or change it by argument 'newSt' assign.
    if(iid == "enable") {
	if(prms.size())	{ prms[0].getB() ? enable() : disable(); }
	return enableStat();
    }
    // bool start( bool newSt = EVAL ) - get start status or change it by argument 'newSt' assign.
    if(iid == "start") {
	if(prms.size())	{ prms[0].getB() ? start() : stop(); }
	return startStat();
    }

    //Configuration functions call
    TVariant cfRez = objFunc(iid, prms, user_lang, RWRWR_, "root:" SDAQ_ID);
    if(!cfRez.isNull()) return cfRez;

    return TCntrNode::objFuncCall(iid, prms, user_lang);
}

void TController::cntrCmdProc( XMLNode *opt )
{
    string a_path = opt->attr("path");
    //Service commands process
    if(a_path == "/serv/mess") {
	if(ctrChkNode(opt,"get")) {
	    if(catsPat().empty()) return;
	    vector<TMess::SRec> rez;
	    time_t tm	= strtoul(opt->attr("tm").c_str(), 0, 10);
	    //-1 for waranty all curent date get without doubles and losses
	    if(!tm) {
		tm = redntUse(TController::Any) ? SYS->archive().at().rdTm() : (time_t)(TSYS::curTime()/1000000) - 1;
		opt->setAttr("tm", i2s(tm));
	    }
	    time_t tm_grnd = strtoul(opt->attr("tm_grnd").c_str(), 0, 10);
	    int	lev	= s2i(opt->attr("lev"));
	    SYS->archive().at().messGet(tm_grnd, tm, rez, "/("+catsPat()+")/", lev, "");
	    for(unsigned iR = 0; iR < rez.size(); iR++)
		opt->childAdd("el")->
		    setAttr("time", u2s(rez[iR].time))->
		    setAttr("utime", u2s(rez[iR].utime))->
		    setAttr("cat", rez[iR].categ)->
		    setAttr("lev", i2s(rez[iR].level))->
		    setText(rez[iR].mess);
	    return;
	}
	if(ctrChkNode(opt,"set")) {
	    messSet(opt->text(), s2i(opt->attr("lev")), opt->attr("type2Code"), opt->attr("prm"), opt->attr("cat"));
	    return;
	}
    }

    //Get page info
    if(opt->name() == "info") {
	TCntrNode::cntrCmdProc(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Controller: ")+trD(name()),RWRWR_,"root",SDAQ_ID);
	ctrMkNode("branches",opt,-1,"/br","",R_R_R_);
	if(ctrMkNode("area",opt,-1,"/cntr",_("Controller"),R_R_R_)) {
	    if(ctrMkNode("area",opt,-1,"/cntr/st",_("State"),R_R_R_)) {
		ctrMkNode("fld",opt,-1,"/cntr/st/status",_("Status"),R_R_R_,"","",1, "tp","str");
		ctrMkNode3("fld",opt,-1,"/cntr/st/enSt",_("Enabled"),SEC_RD|SEC_WR, "tp","bool", NULL);
		ctrMkNode3("fld",opt,-1,"/cntr/st/runSt",_("Running"),SEC_RD|SEC_WR, "tp","bool", NULL);
		ctrMkNode3("fld",opt,-1,"/cntr/st/db",_("Storage"),SEC_RD|SEC_WR,
		    "tp","str", "dest","select", "select","/db/list", "help",TMess::labStor().c_str(), NULL);
		if(DB(true).size())
		    ctrMkNode3("comm",opt,-1,"/cntr/st/removeFromDB",TSYS::strMess(_("Remove from '%s'"),
			TMess::labStorFromCode(DB(true)).c_str()).c_str(),SEC_RD|SEC_WR, "help",TMess::labStorRem(mDB).c_str(), NULL);
		ctrMkNode("fld",opt,-1,"/cntr/st/timestamp",_("Date of modification"),R_R_R_,"","",1, "tp","time");
	    }
	    if(ctrMkNode3("area",opt,-1,"/cntr/cfg",_("Configuration"),SEC_RD,NULL)) {
		TConfig::cntrCmdMake(this,opt,"/cntr/cfg",0,"","",SEC_RD|SEC_WR);
		for(unsigned iTpP = 0; iTpP < owner().tpPrmSize(); ++iTpP)	//????[v1.0] Remove
		    if(owner().tpPrmAt(iTpP).mDB.size() && tbl(owner().tpPrmAt(iTpP)) == tblStd(owner().tpPrmAt(iTpP)))
			ctrRemoveNode(opt, ("/cntr/cfg/"+owner().tpPrmAt(iTpP).mDB).c_str());
		ctrMkNode3("fld",opt,-1,"/cntr/cfg/DESCR",EVAL_STR,SEC_RD|SEC_WR, "SnthHgl","1", NULL);
		ctrRemoveNode(opt, "/cntr/cfg/MESS_LEV");
		ctrRemoveNode(opt, "/cntr/cfg/REDNT");
		ctrRemoveNode(opt, "/cntr/cfg/REDNT_RUN");
	    }
	}
	if(owner().tpPrmSize()) {
	    ctrMkNode("grp",opt,-1,"/br/prm_",_("Parameter"),RWRWR_,"","",2, "idm",i2s(limObjNm_SZ).c_str(), "idSz",i2s(limObjID_SZ).c_str());
	    if(ctrMkNode("area",opt,-1,"/prm",_("Parameters"),R_R_R_)) {
		if(owner().tpPrmSize() > 1)
		    ctrMkNode("fld",opt,-1,"/prm/t_prm",_("Add parameters to the type"),RWRW__,"","",3,
			"tp","str", "dest","select", "select","/prm/t_lst");
		ctrMkNode("fld",opt,-1,"/prm/nmb",_("Number"),R_R_R_,"","",1, "tp","str");
		ctrMkNode("list",opt,-1,"/prm/prm",_("Parameters"),RWRWR_,"","",5,
		    "tp","br", "idm",i2s(limObjNm_SZ).c_str(), "s_com","add,del", "br_pref","prm_", "idSz",i2s(limObjID_SZ).c_str());
	    }
	}
	if(ctrMkNode("area",opt,-1,"/mess",_("Diagnostics"),R_R___)) {
	    ctrMkNode("fld",opt,-1,"/mess/tm",_("Time, depth and level"),RWRW__,"","",1, "tp","time");
	    ctrMkNode("fld",opt,-1,"/mess/size","",RWRW__,"","",5, "tp","str", "len","10",
		"dest","sel_ed", "sel_list",TMess::labTimeSel().c_str(), "help",TMess::labTime().c_str());
	    ctrMkNode("fld",opt,-1,"/mess/lvl","",RWRW__,"","",5, "tp","dec", "dest","select",
		"sel_id","0;1;2;3;4;5;6;7",
		"sel_list",_("Debug (0);Information (1[X]);Notice (2[X]);Warning (3[X]);Error (4[X]);Critical (5[X]);Alert (6[X]);Emergency (7[X])"),
		"help",_("Display messages for larger or even levels.\n"
			 "Also affects to specific-diagnostic messages generation by the data sources."));
	    if(ctrMkNode("table",opt,-1,"/mess/mess",_("Messages"),R_R___,"","")) {
		ctrMkNode("list",opt,-1,"/mess/mess/0",_("Time"),R_R___,"","",1, "tp","time");
		ctrMkNode("list",opt,-1,"/mess/mess/0a",_("mcsec"),R_R___,"","",1, "tp","dec");
		ctrMkNode("list",opt,-1,"/mess/mess/1",_("Category"),R_R___,"","",1, "tp","str");
		ctrMkNode("list",opt,-1,"/mess/mess/2",_("Level"),R_R___,"","",1, "tp","dec");
		ctrMkNode("list",opt,-1,"/mess/mess/3",_("Message"),R_R___,"","",1, "tp","str");
	    }
	}
	return;
    }

    //Process command to page
    vector<string> c_list;

    if(a_path == "/cntr/st/status" && ctrChkNode(opt,"get")) opt->setText(getStatus());
    else if(a_path == "/cntr/st/db") {
	if(ctrChkNode2(opt,"get",SEC_RD)) opt->setText(DB());
	if(ctrChkNode2(opt,"set",SEC_WR)) setDB(opt->text());
    }
    else if(a_path == "/cntr/st/enSt") {
	if(ctrChkNode2(opt,"get",SEC_RD)) opt->setText(enSt?"1":"0");
	if(ctrChkNode2(opt,"set",SEC_WR)) s2i(opt->text()) ? enable() : disable();
    }
    else if(a_path == "/cntr/st/runSt") {
	if(ctrChkNode2(opt,"get",SEC_RD)) opt->setText(runSt?"1":"0");
	if(ctrChkNode2(opt,"set",SEC_WR)) s2i(opt->text()) ? start() : stop();
    }
    else if(a_path == "/cntr/st/removeFromDB" && ctrChkNode2(opt,"set",SEC_WR))
	postDisable(NodeRemoveOnlyStor);
    else if(a_path == "/cntr/st/timestamp" && ctrChkNode(opt,"get")) opt->setText(i2s(timeStamp()));
    else if(a_path == "/cntr/cfg/DESCR" && ctrChkNode2(opt,"SnthHgl",SEC_RD)) nodeLoadACLSnthHgl(*opt);
    else if(a_path.find("/cntr/cfg") == 0) {
	TConfig::cntrCmdProc(this, opt, TSYS::pathLev(a_path,2), "", "", -1);
	if(ctrChkNode2(opt,"set",SEC_WR))	//????[v1.0] Remove
	    for(unsigned iT = 0; iT < owner().tpPrmSize(); iT++)
		if(owner().tpPrmAt(iT).mDB == TSYS::pathLev(a_path,2))
		{ modifG(); break; }
    }
    else if(a_path == "/prm/nmb" && ctrChkNode(opt,"get")) {
	list(c_list);
	unsigned eC = 0;
	for(unsigned iA = 0; iA < c_list.size(); iA++)
	    if(at(c_list[iA]).at().enableStat()) eC++;
	opt->setText(TSYS::strMess(_("All: %d; Enabled: %d"),c_list.size(),eC));
    }
    else if(a_path == "/prm/t_prm" && owner().tpPrmSize()) {
	if(ctrChkNode(opt,"get",RWRW__,"","",SEC_RD))
	    opt->setText(TBDS::genPrmGet(owner().nodePath()+"addType",owner().tpPrmAt(0).name,opt->attr("user")));
	if(ctrChkNode(opt,"set",RWRW__,"","",SEC_WR))
	    TBDS::genPrmSet(owner().nodePath()+"addType",opt->text(),opt->attr("user"));
    }
    else if((a_path == "/br/prm_" || a_path == "/prm/prm") && owner().tpPrmSize()) {
	if(ctrChkNode(opt,"get",RWRWR_,"","",SEC_RD)) {
	    list(c_list);
	    for(unsigned iA = 0; iA < c_list.size(); iA++) {
		XMLNode *cN = opt->childAdd("el")->setAttr("id",c_list[iA])->setText(trD(at(c_list[iA]).at().name()));
		if(!s2i(opt->attr("recurs"))) continue;
		cN->setName(opt->name())->setAttr("path",TSYS::strEncode(opt->attr("path"),TSYS::PathEl))->setAttr("recurs","1");
		at(c_list[iA]).at().cntrCmd(cN);
		cN->setName("el")->setAttr("path","")->setAttr("rez","")->setAttr("recurs","")->setText("");
	    }
	}
	if(ctrChkNode(opt,"add",RWRWR_,"","",SEC_WR)) {
	    opt->setAttr("id", add(opt->attr("id"),owner().tpPrmToId(TBDS::genPrmGet(owner().nodePath()+"addType",owner().tpPrmAt(0).name,opt->attr("user")))));
	    at(opt->attr("id")).at().setName(opt->text());
	}
	if(ctrChkNode(opt,"del",RWRWR_,"","",SEC_WR))	del(opt->attr("id"), NodeRemove);
    }
    else if(a_path == "/prm/t_lst" && owner().tpPrmSize() && ctrChkNode(opt,"get",R_R_R_)) {
	for(unsigned iA = 0; iA < owner().tpPrmSize(); iA++)
	    opt->childAdd("el")->setAttr("id",owner().tpPrmAt(iA).name)->setText(owner().tpPrmAt(iA).descr);
    }
    else if(a_path == "/mess/tm") {
	if(ctrChkNode(opt,"get",RWRW__,"","",SEC_RD)) {
	    opt->setText(TBDS::genPrmGet(SYS->daq().at().nodePath()+"messTm",DEF_messTm,opt->attr("user")));
	    if(!s2i(opt->text())) opt->setText(i2s(TSYS::curTime()/1000000));
	}
	if(ctrChkNode(opt,"set",RWRW__,"","",SEC_WR))
	    TBDS::genPrmSet(SYS->daq().at().nodePath()+"messTm",(s2i(opt->text())>=TSYS::curTime()/1000000)?"0":opt->text(),opt->attr("user"));
    }
    else if(a_path == "/mess/size") {
	if(ctrChkNode(opt,"get",RWRWR_,"","",SEC_RD))
	    opt->setText(TSYS::time2str(s2i(TBDS::genPrmGet(SYS->daq().at().nodePath()+"messSize",DEF_messSize,opt->attr("user"))),false));
	if(ctrChkNode(opt,"set",RWRWR_,"","",SEC_WR))
	    TBDS::genPrmSet(SYS->daq().at().nodePath()+"messSize",i2s(TSYS::str2time(opt->text(),false)),opt->attr("user"));
    }
    else if(a_path == "/mess/lvl") {
	if(ctrChkNode(opt,"get",RWRW__,"","",SEC_RD))	opt->setText(mMessLev.getS());
	if(ctrChkNode(opt,"set",RWRW__,"","",SEC_WR))	mMessLev = opt->text();
    }
    else if(a_path == "/mess/mess" && ctrChkNode(opt,"get",R_R___,"","")) {
	vector<TMess::SRec> rec;
	time_t gtm = s2i(TBDS::genPrmGet(SYS->daq().at().nodePath()+"messTm",DEF_messTm,opt->attr("user")));
	if(!gtm) gtm = (time_t)(TSYS::curTime()/1000000);
	int gsz = s2i(TBDS::genPrmGet(SYS->daq().at().nodePath()+"messSize",DEF_messSize,opt->attr("user")));
	SYS->archive().at().messGet(gtm-gsz, gtm, rec, "/("+catsPat()+")/", messLev(), "");

	XMLNode *n_tm   = ctrMkNode("list",opt,-1,"/mess/mess/0","",R_R___,"","");
	XMLNode *n_tmu  = ctrMkNode("list",opt,-1,"/mess/mess/0a","",R_R___,"","");
	XMLNode *n_cat  = ctrMkNode("list",opt,-1,"/mess/mess/1","",R_R___,"","");
	XMLNode *n_lvl  = ctrMkNode("list",opt,-1,"/mess/mess/2","",R_R___,"","");
	XMLNode *n_mess = ctrMkNode("list",opt,-1,"/mess/mess/3","",R_R___,"","");
	for(int iRec = rec.size()-1; iRec >= 0; iRec--) {
	    if(n_tm)	n_tm->childAdd("el")->setText(i2s(rec[iRec].time));
	    if(n_tmu)	n_tmu->childAdd("el")->setText(i2s(rec[iRec].utime));
	    if(n_cat)	n_cat->childAdd("el")->setText(rec[iRec].categ);
	    if(n_lvl)	n_lvl->childAdd("el")->setText(i2s(rec[iRec].level));
	    if(n_mess)	n_mess->childAdd("el")->setText(rec[iRec].mess);
	}
    }
    else TCntrNode::cntrCmdProc(opt);
}

bool TController::cfgChange( TCfg &co, const TVariant &pc )
{
    if(co.getS() != pc.getS()) {
	if(co.name() == "ENABLE" && !co.getB())    cfg("START").setB(false);
	else if(co.name() == "START" && co.getB()) cfg("ENABLE").setB(true);
	else if(co.name() == "DESCR") nodeLoadACL(co.getS());

	modif();
    }
    return true;
}
