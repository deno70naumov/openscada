
//OpenSCADA module Archive.DBArch file: mess.cpp
/***************************************************************************
 *   Copyright (C) 2007-2023 by Roman Savochenko, <roman@oscada.org>       *
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

#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <tsys.h>
#include "arch.h"
#include "mess.h"

using namespace DBArch;

//************************************************
//* DBArch::ModMArch - Messages archivator       *
//************************************************
ModMArch::ModMArch( const string &iid, const string &idb, TElem *cf_el ) :
    TMArchivator(iid, idb, cf_el), tmProc(0), tmProcMax(0), mBeg(0), mEnd(0), mMaxSize(0),
    mTmAsStr(false), mKeyTmCat(true), needMeta(true)
{
    setAddr(DB_GEN);
}

ModMArch::~ModMArch( )
{
    try { stop(); } catch(...) { }
}

TCntrNode &ModMArch::operator=( const TCntrNode &node )
{
    const TMArchivator *src_n = dynamic_cast<const TMArchivator*>(&node);
    if(!src_n) return *this;

    //Configuration copy
    exclCopy(*src_n, "ID;START;");
    cfg("MODUL").setS(owner().modId());
    setDB(src_n->DB());

    //TMArchivator::operator=(node);
    load_();

    return *this;
}

void ModMArch::postDisable( int flag )
{
    TMArchivator::postDisable(flag);

    if(flag&NodeRemove) {
	//Remove info record
	TConfig cfg(&mod->archEl());
	cfg.cfg("TBL").setS(archTbl(),true);
	TBDS::dataDel(addr()+"."+mod->mainTbl(), "", cfg);

	//Removing the archive DB table
	TBDS::dataDelTbl(addr()+"."+archTbl());
    }
}

void ModMArch::load_( )
{
    //TMArchivator::load_();

    //Init address to DB
    if(addr().empty()) setAddr(DB_GEN);

    try {
	XMLNode prmNd;
	string  vl;
	prmNd.load(cfg("A_PRMS").getS());
	if(!(vl=prmNd.attr("Size")).empty())	setMaxSize(s2r(vl));
	if(!(vl=prmNd.attr("TmAsStr")).empty())	setTmAsStr(s2i(vl));
	if(!(vl=prmNd.attr("KeyTmCat")).empty()) setKeyTmCat(s2i(vl));
    } catch(...) { }

    needMeta = !readMeta();
}

void ModMArch::save_( )
{
    XMLNode prmNd("prms");
    prmNd.setAttr("Size", r2s(maxSize()));
    prmNd.setAttr("TmAsStr", i2s(tmAsStr()));
    prmNd.setAttr("KeyTmCat", i2s(keyTmCat()));
    cfg("A_PRMS").setS(prmNd.save(XMLNode::BrAllPast));

    TMArchivator::save_();
}

void ModMArch::start( )
{
    if(!runSt) {
	reqEl.fldClear();
	reqEl.fldAdd(new TFld("MIN",trS("In minutes"),TFld::Integer,TCfg::Key,"8"));	//Mostly for fast reading next, by minutes
	reqEl.fldAdd(new TFld("TM",trS("Time, seconds"),TFld::Integer,TCfg::Key|(tmAsStr()?TFld::DateTimeDec:0),(tmAsStr()?"20":"10")));
	reqEl.fldAdd(new TFld("TMU",trS("Time, microseconds"),TFld::Integer,TCfg::Key,"6","0"));
	reqEl.fldAdd(new TFld("CATEG",trS("Category"),TFld::String,TCfg::Key,"200"));
	reqEl.fldAdd(new TFld("MESS",trS("Message"),TFld::String,(keyTmCat()?(int)TFld::NoFlag:(int)TCfg::Key),(keyTmCat()?"100000":"255")));
	reqEl.fldAdd(new TFld("LEV",trS("Level"),TFld::Integer,TFld::NoFlag,"2"));
    }

    //Connection to DB and enable status check
    string wdb = TBDS::realDBName(addr());
    AutoHD<TBD> db = SYS->db().at().nodeAt(wdb, 0, '.');
    try { if(!db.at().enableStat()) db.at().enable(); }
    catch(TError &err) { mess_warning(nodePath().c_str(), _("Error enabling the target DB: %s"), err.mess.c_str()); }

    TMArchivator::start();
}

void ModMArch::stop( )
{
    TMArchivator::stop();

    reqEl.fldClear();
}

time_t ModMArch::begin( )
{
    return mBeg;
}

time_t ModMArch::end( )
{
    return mEnd;
}

bool ModMArch::put( vector<TMess::SRec> &mess, bool force )
{
    if(needMeta && (needMeta=!readMeta()))	return false;

    TMArchivator::put(mess, force);	//Allow redundancy

    if(!runSt) throw TError(nodePath(), _("The archive is not started!"));

    AutoHD<TTable> tbl = TBDS::tblOpen(addr()+"."+archTbl(), true);
    if(tbl.freeStat()) return false;

    TConfig cfg(&reqEl);
    int64_t t_cnt = TSYS::curTime();
    for(unsigned i_m = 0; i_m < mess.size(); i_m++) {
	if(!chkMessOK(mess[i_m].categ,mess[i_m].level)) continue;

	//Put record to DB
	cfg.cfg("MIN").setI(mess[i_m].time/60);
	cfg.cfg("TM").setI(mess[i_m].time);
	cfg.cfg("TMU").setI(mess[i_m].utime);
	cfg.cfg("CATEG").setS(mess[i_m].categ);
	cfg.cfg("MESS").setS(mess[i_m].mess);
	cfg.cfg("LEV").setI(mess[i_m].level);
	tbl.at().fieldSet(cfg);
	//Archive time border update
	mBeg = mBeg ? vmin(mBeg,mess[i_m].time) : mess[i_m].time;
	mEnd = mEnd ? vmax(mEnd,mess[i_m].time) : mess[i_m].time;
    }

    //Archive size limit process
    if(maxSize() && (mEnd-mBeg) > (time_t)(maxSize()*86400)) {
	time_t nEnd = mEnd - (time_t)(maxSize()*86400);
	cfg.cfg("TM").setKeyUse(false);
	for(int tC = mBeg/60; tC < nEnd/60; tC++) {
	    cfg.cfg("MIN").setI(tC, true);
	    tbl.at().fieldDel(cfg);
	}
	mBeg = nEnd;
    }
    tbl.free();
    //SYS->db().at().close(addr()+"."+archTbl());	//!!! No close the table manually

    //Update archive info
    cfg.setElem(&mod->archEl());
    cfg.cfgViewAll(false);
    cfg.cfg("TBL").setS(archTbl(),true);
    cfg.cfg("BEGIN").setS(i2s(mBeg),true);
    cfg.cfg("END").setS(i2s(mEnd),true);
    bool rez = TBDS::dataSet(addr()+"."+mod->mainTbl(), "", cfg, TBDS::NoException);

    tmProc = TSYS::curTime() - t_cnt; tmProcMax = vmax(tmProcMax, tmProc);

    return rez;
}

time_t ModMArch::get( time_t bTm, time_t eTm, vector<TMess::SRec> &mess, const string &category, char level, time_t upTo )
{
    if(!runSt) throw TError(nodePath(), _("The archive is not started!"));
    if(needMeta && (needMeta=!readMeta())) return eTm;
    if(!upTo) upTo = SYS->sysTm() + prmInterf_TM;

    bTm = vmax(bTm, begin());
    eTm = vmin(eTm, end());
    if(eTm < bTm) return eTm;

    TConfig cfg(&reqEl);
    TRegExp re(category, "p");

    //Get values from DB
    cfg.cfg("TM").setKeyUse(false);
    time_t result = bTm;
    for(time_t tC = bTm; tC/60 <= eTm/60 && SYS->sysTm() < upTo; ) {
	tC = (tC/60)*60;
	cfg.cfg("MIN").setI(tC/60, true);
	int eC = 0;
	for( ; TBDS::dataSeek(addr()+"."+archTbl(),"",eC++,cfg,TBDS::UseCache) && SYS->sysTm() < upTo; ) {
	    TMess::SRec rc(cfg.cfg("TM").getI(), cfg.cfg("TMU").getI(), cfg.cfg("CATEG").getS(),
			    (TMess::Type)cfg.cfg("LEV").getI(), cfg.cfg("MESS").getS());
	    if(rc.time >= bTm && rc.time <= eTm && TMess::messLevelTest(level,rc.level) && re.test(rc.categ)) {
		bool equal = false;
		int i_p = mess.size();
		for(int i_m = mess.size()-1; i_m >= 0; i_m--) {
		    if(FTM(mess[i_m]) > FTM(rc)) i_p = i_m;
		    else if(FTM(mess[i_m]) == FTM(rc) && rc.level == mess[i_m].level && rc.mess == mess[i_m].mess)
		    { equal = true; break; }
		    else if(FTM(mess[i_m]) < FTM(rc)) break;
		}
		if(!equal) {
		    mess.insert(mess.begin()+i_p, rc);
		    if(SYS->sysTm() >= upTo) return result;
		}
	    }
	}
	tC += 60;
	if(SYS->sysTm() < upTo) result = vmax(bTm, vmin(eTm,tC-1));
    }

    return result;
}

bool ModMArch::readMeta( )
{
    bool rez = true;

    //Load message archive parameters
    TConfig wcfg(&mod->archEl());
    wcfg.cfg("TBL").setS(archTbl());
    if(TBDS::dataGet(addr()+"."+mod->mainTbl(),"",wcfg,TBDS::NoException)) {
	mBeg = s2i(wcfg.cfg("BEGIN").getS());
	mEnd = s2i(wcfg.cfg("END").getS());
	// Check for delete archivator table
	if(maxSize() && mEnd <= (time(NULL)-(time_t)(maxSize()*86400))) {
	    TBDS::dataDelTbl(addr()+"."+archTbl());
	    mBeg = mEnd = 0;
	}
    } else rez = false;

    //Check for target DB enabled (disabled by the connection loss)
    if(!rez) {
	string wDB = TBDS::realDBName(addr());
	rez = (TSYS::strParse(wDB,0,".") == DB_CFG ||
	    SYS->db().at().at(TSYS::strParse(wDB,0,".")).at().at(TSYS::strParse(wDB,1,".")).at().enableStat());
    }

    return rez;
}

void ModMArch::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if(opt->name() == "info") {
	TMArchivator::cntrCmdProc(opt);
	ctrRemoveNode(opt,"/prm/cfg/A_PRMS");
	ctrMkNode("fld",opt,-1,"/prm/st/tarch",_("Archiving time"),R_R_R_,"root",SARH_ID,1,"tp","str");
	ctrMkNode("fld",opt,-1,"/prm/cfg/ADDR",EVAL_STR,startStat()?R_R_R_:RWRWR_,"root",SARH_ID,3,
	    "dest","select","select","/db/list:onlydb","help",TMess::labStor().c_str());
	if(ctrMkNode("area",opt,-1,"/prm/add",_("Additional options"),R_R_R_,"root",SARH_ID)) {
	    ctrMkNode("fld",opt,-1,"/prm/add/sz",_("Archive size, days"),RWRWR_,"root",SARH_ID,2,
		"tp","real", "help",_("Set to 0 to disable this limit and to rise some the performance."));
	    ctrMkNode("fld",opt,-1,"/prm/add/tmAsStr",_("To form time as a string"),startStat()?R_R_R_:RWRWR_,"root",SARH_ID,2,
		"tp","bool", "help",_("Only for databases that support such by means of specific data types like \"datetime\" in MySQL."));
	    ctrMkNode("fld",opt,-1,"/prm/add/keyTmCat",_("Unique and non duple messages for time and category only"),startStat()?R_R_R_:RWRWR_,"root",SARH_ID,2,
		"tp","bool", "help",_("Otherwise the message field is included to the primary key and is limited in 255 symbols."));
	}
	return;
    }

    //Process command to page
    string a_path = opt->attr("path");
    if(a_path == "/prm/st/tarch" && ctrChkNode(opt))	opt->setText(tm2s(1e-6*tmProc) + "[" + tm2s(1e-6*tmProcMax) + "]");
    else if(a_path == "/prm/add/sz") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(r2s(maxSize()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setMaxSize(s2r(opt->text()));
    }
    else if(a_path == "/prm/add/tmAsStr") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(tmAsStr()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setTmAsStr(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/keyTmCat") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(keyTmCat()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setKeyTmCat(s2i(opt->text()));
    }
    else TMArchivator::cntrCmdProc(opt);
}
