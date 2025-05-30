
//OpenSCADA module Archive.FSArch file: mess.cpp
/***************************************************************************
 *   Copyright (C) 2003-2023 by Roman Savochenko, <roman@oscada.org>       *
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
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <tsys.h>
#include "base.h"
#include "mess.h"

using namespace FSArch;

//************************************************
//* FSArch::ModMArch - Messages archivator       *
//************************************************
ModMArch::ModMArch( const string &iid, const string &idb, TElem *cf_el ) :
    TMArchivator(iid,idb,cf_el), chkANow(false), infoTbl(dataRes()),
    mUseXml(false), mMaxSize(1024), mNumbFiles(30), mTimeSize(30), mChkTm(60), mPackTm(10),
    mPackInfoFiles(false), mPrevDbl(false), mPrevDblTmCatLev(false), tmProc(0), tmProcMax(0), mLstCheck(0)
{
    if(SYS->prjNm().size()) setAddr("ARCHIVES/MESS/"+iid);
}

ModMArch::~ModMArch( )
{
    try { stop(); } catch(...) { }
}

TCntrNode &ModMArch::operator=( const TCntrNode &node )
{
    TMArchivator::operator=(node);
    load_();

    return *this;
}

void ModMArch::load_( )
{
    //TMArchivator::load_();

    try {
	XMLNode prmNd;
	string  vl;
	prmNd.load(cfg("A_PRMS").getS());
	vl = prmNd.attr("XML");		if(!vl.empty()) setUseXML(s2i(vl));
	vl = prmNd.attr("MSize");	if(!vl.empty()) setMaxSize(s2i(vl));
	vl = prmNd.attr("NFiles");	if(!vl.empty()) setNumbFiles(s2i(vl));
	vl = prmNd.attr("TmSize");	if(!vl.empty()) setTimeSize(s2i(vl));
	vl = prmNd.attr("PackTm");	if(!vl.empty()) setPackTm(s2i(vl));
	vl = prmNd.attr("CheckTm");	if(!vl.empty()) setCheckTm(s2i(vl));
	vl = prmNd.attr("PackInfoFiles"); if(!vl.empty()) setPackInfoFiles(s2i(vl));
	vl = prmNd.attr("PrevDbl");	if(!vl.empty()) setPrevDbl(s2i(vl));
	vl = prmNd.attr("PrevDblTmCatLev"); if(!vl.empty()) setPrevDblTmCatLev(s2i(vl));
    } catch(...) { }
}

void ModMArch::save_( )
{
    XMLNode prmNd("prms");
    prmNd.setAttr("XML", i2s(useXML()));
    prmNd.setAttr("MSize", i2s(maxSize()));
    prmNd.setAttr("NFiles", i2s(numbFiles()));
    prmNd.setAttr("TmSize", i2s(timeSize()));
    prmNd.setAttr("PackTm", i2s(packTm()));
    prmNd.setAttr("CheckTm", i2s(checkTm()));
    prmNd.setAttr("PackInfoFiles", i2s(packInfoFiles()));
    prmNd.setAttr("PrevDbl", i2s(prevDbl()));
    prmNd.setAttr("PrevDblTmCatLev", i2s(prevDblTmCatLev()));
    cfg("A_PRMS").setS(prmNd.save(XMLNode::BrAllPast));

    TMArchivator::save_();
}

bool ModMArch::cfgChange( TCfg &co, const TVariant &pc )
{
    if(co.name() == "ADDR" && startStat())	return false;

    return TMArchivator::cfgChange(co, pc);
}

void ModMArch::start( )
{
    if(!startStat()) {
	//Open/create the archive directory
	DIR *IdDir = opendir(addr().c_str());
	if(IdDir == NULL && mkdir(addr().c_str(),SYS->permCrtFiles(true)))
	    throw err_sys(_("Cannot create the archive directory '%s'."), addr().c_str());
	if(IdDir) closedir(IdDir);

	//Checking the archiver folders for duplicates
	string dbl = "";
	MtxAlloc res(mod->enRes(), true);
	const char *fLock = "fsArchLock";
	int hd = open((addr()+"/"+fLock).c_str(), O_CREAT|O_TRUNC|O_WRONLY, SYS->permCrtFiles());
	if(hd >= 0) {
	    write(hd, "1", 1);
	    vector<string> ls;
	    mod->messList(ls);
	    for(unsigned iL = 0; iL < ls.size() && dbl.empty(); iL++) {
		AutoHD<TMArchivator> mAt = mod->messAt(ls[iL]);
		if(mAt.at().id() == id() || !mAt.at().startStat())	continue;
		int hd1 = open((mAt.at().addr()+"/"+fLock).c_str(), O_RDONLY);
		if(hd1 >= 0 || mAt.at().addr() == addr()) {
		    dbl = mAt.at().addr();
		    if(hd1 >= 0 && close(hd1) != 0)
			mess_warning(nodePath().c_str(), _("Closing the file %d error '%s (%d)'!"), hd1, strerror(errno), errno);
		}
	    }
	    if(close(hd) != 0)
		mess_warning(nodePath().c_str(), _("Closing the file %d error '%s (%d)'!"), hd, strerror(errno), errno);
	    remove((addr()+"/"+fLock).c_str());
	}
	res.unlock();
	if(dbl.size()) { stop(); throw err_sys(_("The value archiver '%s' uses the same folder '%s' as an other archiver."), id().c_str(), addr().c_str()); }
    }

    //Create and/or update the SQLite info file, special for the archivator and placed with main files of the archivator
    if(!startStat() && packInfoFiles()) {
	infoTbl = "";
	try {
	    AutoHD<TTypeBD> SQLite = SYS->db().at().at("SQLite");
	    // Search for the info table
	    vector<string> ls;
	    SQLite.at().list(ls);
	    for(unsigned iL = 0; iL < ls.size() && infoTbl.empty(); iL++)
		if(SQLite.at().at(ls[iL]).at().addr() == addr()+"/info.db")
		    infoTbl = "SQLite."+ls[iL]+"."+TSYS::strParse(mod->filesDB(),2,".");
	    if(infoTbl.empty()) {
		string iDBnm = MOD_ID "_Mess_"+id()+"_info";
		while(true) {
		    AutoHD<TBD> infoDB = SQLite.at().at((iDBnm=SQLite.at().open(iDBnm)));
		    if(infoDB.at().addr().size()) { iDBnm = TSYS::strLabEnum(iDBnm); continue; }
		    infoDB.at().setName(TSYS::strMess(_("%s: Mess: %s: information"),MOD_ID,id().c_str()));
		    infoDB.at().setDscr(TSYS::strMess(_("Local information DB for the message archiver '%s'. "
			"Created automatically then don't modify, save and remove it!"),id().c_str()));
		    infoDB.at().setAddr(addr()+"/info.db");
		    infoDB.at().enable();
		    infoDB.at().modifClr();
		    infoTbl = "SQLite."+iDBnm+"."+TSYS::strParse(mod->filesDB(),2,".");
		    break;
		}
	    }
	} catch(TError&) { }
    }

    //First scan dir
    checkArchivator();
    TMArchivator::start();
}

void ModMArch::stop( )
{
    bool curSt = startStat();

    ResAlloc res(mRes, true);

    TMArchivator::stop();

    //Clear archive files list
    while(files.size()) { delete files[0]; files.pop_front(); }

    if(curSt)	infoTbl = "";
    mLstCheck = 0;
}

time_t ModMArch::begin( )
{
    ResAlloc res(mRes, false);
    for(int iF = files.size()-1; iF >= 0; iF--)
	if(!files[iF]->err()) return files[iF]->begin();

    return 0;
}

time_t ModMArch::end( )
{
    ResAlloc res(mRes, false);
    for(unsigned iF = 0; iF < files.size(); iF++)
	if(!files[iF]->err()) return files[iF]->end();

    return 0;
}

bool ModMArch::put( vector<TMess::SRec> &mess, bool force )
{
    TMArchivator::put(mess, force);	//For allow redundancy

    int64_t t_cnt = TSYS::curTime();

    ResAlloc res(mRes, true);	//!!!! WR to prevent of multiple files creation on busy systems

    if(!runSt) throw err_sys(_("Archive is not started!"));
    if(!mLstCheck)	return false;

    bool wrOK = true;
    for(unsigned iM = 0; iM < mess.size(); iM++) {
	if(!chkMessOK(mess[iM].categ,mess[iM].level)) continue;
	int iF;
	for(iF = 0; iF < (int)files.size(); iF++)
	    if(!files[iF]->err() && mess[iM].time >= files[iF]->begin()) {
		if(mess[iM].time > files[iF]->end() &&
		    ((mMaxSize && iF == 0 && files[iF]->size() > mMaxSize*1024) ||
		    (mess[iM].time >= files[iF]->begin()+mTimeSize*24*60*60))) break;
		try {
		    wrOK = files[iF]->put(mess[iM]) && wrOK;
		} catch(TError &err) { mess_err(err.cat.c_str(), "%s", err.mess.c_str()); continue; }
		iF = -1;
		break;
	    }
	//At comming new data we create new file
	if(iF >= 0) {
	    //res.request(true);
	    time_t f_beg = mess[iM].time;
	    if(iF < (int)files.size() && f_beg > files[iF]->end() && (f_beg-files[iF]->end()) < (mTimeSize*24*60*60/3))
		f_beg = files[iF]->end()+1;
	    if(iF && files[iF-1]->begin() > f_beg && (files[iF-1]->begin()-f_beg) < (mTimeSize*24*60*60*2/3))
		f_beg = files[iF-1]->begin()-mTimeSize*24*60*60;
	    // Create new Archive
	    string f_name = atm2s(f_beg, "/%F %H.%M.%S.msg");
	    try {
		MFileArch *f_obj = new MFileArch(addr()+f_name, f_beg, this, Mess->charset(), useXML());
		//Remove new error created file mostly by store space lack
		if(f_obj->err()) {
		    f_obj->delFile();
		    delete f_obj;
		    return false;
		}

		if(iF == (int)files.size()) files.push_back(f_obj);
		else if(iF < (int)files.size()) files.insert(files.begin()+iF, f_obj);
		else { delete f_obj; return true; }
	    } catch(TError &err) {
		mess_sys(TMess::Crit, _("Error creating a new archive file '%s'!"), (addr()+f_name).c_str());
		return false;
	    }
	    // Allow parallel read access
	    //res.request(false);	//!!!! That is bad to relock in the cycle and for the direct index
	    wrOK = files[iF]->put(mess[iM]) && wrOK;
	}
    }

    tmProc = TSYS::curTime() - t_cnt; tmProcMax = vmax(tmProcMax, tmProc);

    return wrOK;
}

time_t ModMArch::get( time_t bTm, time_t eTm, vector<TMess::SRec> &mess, const string &category, char level, time_t upTo )
{
    ResAlloc res(mRes, false);

    bTm = vmax(bTm, begin());
    eTm = vmin(eTm, end());
    if(eTm < bTm) return eTm;
    if(!runSt) throw err_sys(_("Archive is not started!"));
    if(!upTo) upTo = SYS->sysTm() + prmInterf_TM;

    time_t result = bTm;
    for(int iF = files.size()-1; iF >= 0 && SYS->sysTm() < upTo; iF--) {
	if(!files[iF]->err() &&
		!((bTm < files[iF]->begin() && eTm < files[iF]->begin()) || (bTm > files[iF]->end() && eTm > files[iF]->end())))
	    result = files[iF]->get(bTm, eTm, mess, category, level, upTo);
    }

    return result;
}

void ModMArch::checkArchivator( bool now )
{
    if(chkANow)	return;

    chkANow = true;
    TError err;

    try {

    if(now || time(NULL) > mLstCheck + checkTm()*60) {
	DIR *IdDir = opendir(addr().c_str());
	if(IdDir == NULL) throw err_sys(_("The archive directory '%s' is not present."), addr().c_str());

	//Clean scan flag
	ResAlloc res(mRes, false);
	for(unsigned iF = 0; iF < files.size(); iF++) files[iF]->scan = false;
	res.release();

	struct stat file_stat;
	dirent	*scan_rez = NULL,
		*scan_dirent = (dirent*)malloc(offsetof(dirent,d_name) + NAME_MAX + 1);

	while(readdir_r(IdDir,scan_dirent,&scan_rez) == 0 && scan_rez) {
	    if(strcmp(scan_rez->d_name,"..") == 0 || strcmp(scan_rez->d_name,".") == 0 || strcmp(scan_rez->d_name,"info.db") == 0) continue;
	    string NameArhFile = addr() + "/" + scan_rez->d_name;

	    stat(NameArhFile.c_str(), &file_stat);
	    if((file_stat.st_mode&S_IFMT) != S_IFREG || access(NameArhFile.c_str(),F_OK|R_OK) != 0) continue;

	    // Remove for empty files mostly after wrong or limited FSs
	    if(file_stat.st_size == 0) { remove(NameArhFile.c_str()); remove((NameArhFile+".info").c_str()); continue; }

	    // Pass info and other improper files
	    if(NameArhFile.compare(NameArhFile.size()-4,4,".msg") != 0 && NameArhFile.compare(NameArhFile.size()-7,7,".msg.gz") != 0) continue;

	    // Check all files
	    unsigned iF;
	    res.request(false);
	    for(iF = 0; iF < files.size(); iF++)
		if(files[iF]->name() == NameArhFile) {
		    files[iF]->scan = true;
		    res.release();
		    break;
		}
	    // Registre new archive file
	    if(iF >= files.size()) {
		res.release();

		MFileArch *f_arh = new MFileArch(this);
		f_arh->attach(NameArhFile, false);
		f_arh->scan = true;

		res.request(true);
		//  Oldest and broken archives to the down
		if(f_arh->err()) files.push_back(f_arh);
		else {
		    for(iF = 0; iF < files.size(); iF++)
			if(files[iF]->err() || f_arh->begin() >= files[iF]->begin()) {
			    files.insert(files.begin()+iF, f_arh);
			    break;
			}
		    if(iF >= files.size()) files.push_back(f_arh);
		}
		res.release();
	    }
	}

	free(scan_dirent);
	closedir(IdDir);

	//Check for archives deletion
	res.request(true);
	for(unsigned iF = 0; iF < files.size(); )
	    if(!files[iF]->scan) {
		delete files[iF];
		files.erase(files.begin() + iF);
	    }
	    else iF++;
	//res.release();

	//Check file count and delete odd files
	if(mNumbFiles && !mod->noArchLimit) {
	    int f_cnt = 0;	//Work files number
	    //res.request(false);
	    for(unsigned iF = 0; iF < files.size(); iF++)
		if(!files[iF]->err()) f_cnt++;
	    if(f_cnt > mNumbFiles) {
		// Delete oldest files
		//res.request(true);
		for(int iF = files.size()-1; iF >= 0; iF--)
		    if(f_cnt <= mNumbFiles)	break;
		    else if(!files[iF]->err()) {
			files[iF]->delFile();
			delete files[iF];
			files.erase(files.begin()+iF);
			f_cnt--;
		    }
	    }
	}
	res.release();

	//Check for not presented files in the info table
	if(infoTbl.size() && !now) {
	    TConfig cEl(&mod->packFE());
	    cEl.cfgViewAll(false);

	    for(int fldCnt = 0; TBDS::dataSeek(infoTbl,mod->nodePath()+"Pack",fldCnt++,cEl); )
		if(stat(cEl.cfg("FILE").getS().c_str(),&file_stat) != 0 || (file_stat.st_mode&S_IFMT) != S_IFREG) {
		    if(!TBDS::dataDel(infoTbl,mod->nodePath()+"Pack",cEl,TBDS::UseAllKeys|TBDS::NoException)) break;
		    fldCnt--;
	    }
	}

	mLstCheck = time(NULL);
    }

    //Call files checking
    ResAlloc res(mRes, false);
    for(unsigned iF = 0; iF < files.size(); iF++)
	files[iF]->check();

    } catch(TError &ierr) { err = ierr; }

    chkANow = false;

    if(err.mess.size()) throw err;
}

int ModMArch::size( )
{
    int rez = 0;
    ResAlloc res(mRes, false);
    for(unsigned iF = 0; iF < files.size(); iF++)
	rez += files[iF]->size();

    return rez;
}

void ModMArch::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if(opt->name() == "info") {
	TMArchivator::cntrCmdProc(opt);
	ctrMkNode("fld",opt,-1,"/prm/st/fsz",_("Overall size of the archiver files"),R_R_R_,"root",SARH_ID,1,"tp","str");
	ctrMkNode("fld",opt,-1,"/prm/st/tarch",_("Archiving time"),R_R_R_,"root",SARH_ID,1,"tp","str");
	ctrMkNode("fld",opt,-1,"/prm/cfg/ADDR",EVAL_STR,startStat()?R_R_R_:RWRWR_,"root",SARH_ID,3,
	    "dest","sel_ed","select","/prm/cfg/dirList","help",_("Path to a directory for files of messages of the archivator."));
	ctrRemoveNode(opt,"/prm/cfg/A_PRMS");
	if(ctrMkNode("area",opt,-1,"/prm/add",_("Additional options"),R_R_R_,"root",SARH_ID)) {
	    ctrMkNode("fld",opt,-1,"/prm/add/xml",_("Files of the archive in XML"),RWRWR_,"root",SARH_ID,2,"tp","bool","help",
		_("Enables messages archiving by files in the XML-format, rather than plain text.\n"
		  "Using XML-archiving requires more RAM as it requires full file download, XML parsing and memory holding at the time of use."));
	    ctrMkNode("fld",opt,-1,"/prm/add/prev_dbl",_("Prevent duplicates"),RWRWR_,"root",SARH_ID,2,"tp","bool","help",
		_("Enables checking for duplicate messages at the time of putting a message into the archive.\n"
		  "If there is a duplicate the message does not fit into the archive.\n"
		  "This feature some increases the recording time to the archive, but in cases of "
		  "placing messages in the archive by past time from external sources it allows to eliminate the duplicates."));
	    ctrMkNode("fld",opt,-1,"/prm/add/prev_TmCatLev_dbl",_("Consider duplicates and prevent, for equal time, category, level"),RWRWR_,"root",SARH_ID,2,"tp","bool","help",
		_("Enables checking for duplicate messages at the time of putting a message into the archive.\n"
		  "As the duplicates there considers messages which equal to time, category and level.\n"
		  "If there is a duplicate then the new message will replace the old one into the archive.\n"
		  "This feature mostly usable at message text changing in time, for alarm's state to example."));
	    ctrMkNode("fld",opt,-1,"/prm/add/sz",_("Maximum size of archive's file, kB"),RWRWR_,"root",SARH_ID,2,"tp","dec","help",
		_("Sets limit on the size of one archive file.\n"
		  "Disabling the restriction can be performed by setting the parameter to zero."));
	    ctrMkNode("fld",opt,-1,"/prm/add/fl",_("Maximum number of the files"),RWRWR_,"root",SARH_ID,2,"tp","dec","help",
		_("Limits the maximum number for files of the archive and additional with the size to single file "
		  "it determines the size of the archive on disk.\n"
		  "Completely removing this restriction can be performed by setting the parameter to zero."));
	    ctrMkNode("fld",opt,-1,"/prm/add/len",_("Time size of the archive files, days"),RWRWR_,"root",SARH_ID,2,"tp","dec","help",
		_("Sets limit on the size of single archive file on time."));
	    ctrMkNode("fld",opt,-1,"/prm/add/pcktm",_("Timeout packaging archive files, minutes"),RWRWR_,"root",SARH_ID,2,"tp","dec","help",
		_("Sets the time after which, in the absence of requests, the archive file will be packaged in a gzip archive.\n"
		 "Set to zero for disabling the packing by gzip."));
	    ctrMkNode("fld",opt,-1,"/prm/add/tm",_("Period of the archives checking, minutes"),RWRWR_,"root",SARH_ID,2,"tp","dec","help",
		_("Sets the periodicity of checking the archives for the appearance or deletion files in the archive folder, "
		  "as well as exceeding the limits and removing old archives files."));
	    ctrMkNode("fld",opt,-1,"/prm/add/pack_info_fl",_("Use info file for packaged archives"),RWRWR_,"root",SARH_ID,2,"tp","bool","help",
		_("Specifies whether to create a file with information about the packed archive files by gzip-archiver.\n"
		  "When copying the files of archive to another station, this info file can speed up the target station "
		  "process of first run by eliminating the need to decompress by gzip-archiver in order to obtain the information."));
	    ctrMkNode("comm",opt,-1,"/prm/add/chk_nw",_("Check now for the directory of the archiver"),RWRW__,"root",SARH_ID,1,"help",
		_("The command, which allows you to immediately start for checking the archives, "
		  "for example, after some manual changes in the directory of the archiver."));
	}
	if(ctrMkNode("area",opt,-1,"/files",_("Files"),R_R___,"root",SARH_ID))
	    if(ctrMkNode("table",opt,-1,"/files/files",_("Files"),R_R___,"root",SARH_ID)) {
		ctrMkNode("list",opt,-1,"/files/files/nm",_("Name"),R_R___,"root",SARH_ID,1,"tp","str");
		ctrMkNode("list",opt,-1,"/files/files/beg",_("Begin"),R_R___,"root",SARH_ID,1,"tp","time");
		ctrMkNode("list",opt,-1,"/files/files/end",_("End"),R_R___,"root",SARH_ID,1,"tp","time");
		ctrMkNode("list",opt,-1,"/files/files/char",_("Charset"),R_R___,"root",SARH_ID,1,"tp","str");
		ctrMkNode("list",opt,-1,"/files/files/sz",_("Size"),R_R___,"root",SARH_ID,1,"tp","str");
		ctrMkNode("list",opt,-1,"/files/files/XML",_("XML"),R_R___,"root",SARH_ID,1,"tp","bool");
		ctrMkNode("list",opt,-1,"/files/files/pack",_("Pack"),R_R___,"root",SARH_ID,1,"tp","bool");
		ctrMkNode("list",opt,-1,"/files/files/err",_("Error"),R_R___,"root",SARH_ID,1,"tp","bool");
	    }
	return;
    }

    //Process command to page
    string a_path = opt->attr("path");
    if(a_path == "/prm/cfg/dirList" && ctrChkNode(opt))		TSYS::ctrListFS(opt, addr());
    else if(a_path == "/prm/st/fsz" && ctrChkNode(opt))		opt->setText(TSYS::cpct2str(size()));
    else if(a_path == "/prm/st/tarch" && ctrChkNode(opt))	opt->setText(tm2s(1e-6*tmProc) + "[" + tm2s(1e-6*tmProcMax) + "]");
    else if(a_path == "/prm/add/xml") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(useXML() ? "1" : "0");
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setUseXML(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/sz") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(maxSize()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setMaxSize(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/fl") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(numbFiles()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setNumbFiles(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/len") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(timeSize()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setTimeSize(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/pcktm") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(packTm()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setPackTm(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/tm") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(checkTm()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setCheckTm(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/pack_info_fl") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(packInfoFiles()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setPackInfoFiles(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/prev_dbl") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(prevDbl()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setPrevDbl(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/prev_TmCatLev_dbl") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SARH_ID,SEC_RD))	opt->setText(i2s(prevDblTmCatLev()));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SARH_ID,SEC_WR))	setPrevDblTmCatLev(s2i(opt->text()));
    }
    else if(a_path == "/prm/add/chk_nw" && ctrChkNode(opt,"set",RWRW__,"root",SARH_ID,SEC_WR))	checkArchivator(true);
    else if(a_path == "/files/files" && ctrChkNode(opt,"get",R_R___,"root",SARH_ID,SEC_RD)) {
	XMLNode *rwNm	= ctrMkNode("list",opt,-1,"/files/files/nm","",R_R___,"root",SARH_ID);
	XMLNode *rwBeg	= ctrMkNode("list",opt,-1,"/files/files/beg","",R_R___,"root",SARH_ID);
	XMLNode *rwEnd	= ctrMkNode("list",opt,-1,"/files/files/end","",R_R___,"root",SARH_ID);
	XMLNode *rwChar	= ctrMkNode("list",opt,-1,"/files/files/char","",R_R___,"root",SARH_ID);
	XMLNode *rwSz	= ctrMkNode("list",opt,-1,"/files/files/sz","",R_R___,"root",SARH_ID);
	XMLNode *rwXML	= ctrMkNode("list",opt,-1,"/files/files/XML","",R_R___,"root",SARH_ID);
	XMLNode *rwPack	= ctrMkNode("list",opt,-1,"/files/files/pack","",R_R___,"root",SARH_ID);
	XMLNode *rwErr	= ctrMkNode("list",opt,-1,"/files/files/err","",R_R___,"root",SARH_ID);

	ResAlloc res(mRes, false);
	for(unsigned iF = 0; iF < files.size(); iF++) {
	    if(rwNm)	rwNm->childAdd("el")->setText(files[iF]->name());
	    if(rwBeg)	rwBeg->childAdd("el")->setText(i2s(files[iF]->begin()));
	    if(rwEnd)	rwEnd->childAdd("el")->setText(i2s(files[iF]->end()));
	    if(rwChar)	rwChar->childAdd("el")->setText(files[iF]->charset());
	    if(rwSz)	rwSz->childAdd("el")->setText(TSYS::cpct2str(files[iF]->size()));
	    if(rwXML)	rwXML->childAdd("el")->setText(i2s(files[iF]->xmlM()));
	    if(rwPack)	rwPack->childAdd("el")->setText(i2s(files[iF]->isPack()));
	    if(rwErr)	rwErr->childAdd("el")->setText(i2s(files[iF]->err()));
	}
    }
    else TMArchivator::cntrCmdProc(opt);
}

//*************************************************
//* FSArch::MFileArch - Messages archivator file  *
//*************************************************
MFileArch::MFileArch( ModMArch *owner ) :
    scan(false), dtRes(true), mName(dtRes), mXML(true), mSize(0), mChars("UTF-8"), mErr(false), mWrite(false), mLoad(false), mPack(false),
    mBeg(0), mEnd(0), mNode(NULL), mOwner(owner)
{
    cach_pr.tm = cach_pr.off = 0;
    mAcces = time(NULL);
}

MFileArch::MFileArch( const string &iname, time_t ibeg, ModMArch *iowner, const string &icharset, bool ixml ) :
    scan(false), dtRes(true), mName(dtRes), mXML(ixml), mSize(0), mChars(icharset), mErr(false), mWrite(false), mLoad(false), mPack(false),
    mBeg(ibeg), mEnd(ibeg), mNode(NULL), mOwner(iowner)
{
    mName = iname;
    cach_pr.tm = cach_pr.off = 0;

    int hd = open(name().c_str(), O_RDWR|O_CREAT|O_TRUNC, SYS->permCrtFiles());
    if(hd <= 0) {
	owner().mess_sys(TMess::Error, _("Error creating a file '%s': %s(%d)."), name().c_str(), strerror(errno), errno);
	mErr = true;
	return;
    }
    bool fOK = true;

    if(xmlM()) {
	//Prepare XML file
	mChars = "UTF-8";
	mNode = new XMLNode();

	mNode->clear()->setName(MOD_ID)->
	    setAttr("Version", MOD_VER)->
	    setAttr("Begin", i2s(mBeg,TSYS::Hex))->
	    setAttr("End", i2s(mEnd,TSYS::Hex));
	string x_cf = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" + mNode->save(XMLNode::BrOpenPrev);
	fOK = (write(hd,x_cf.c_str(),x_cf.size()) == (int)x_cf.size());
    }
    else {
	//Prepare plain text file
	int bufSz = prmStrBuf_SZ;
	char buf[bufSz+1]; buf[bufSz] = 0;
	snprintf(buf, bufSz, "%s %s %s %8x %8x\n", MOD_ID, MOD_VER, mChars.c_str(), (unsigned int)mBeg, (unsigned int)mEnd);
	fOK = (write(hd,buf,strlen(buf)) == (int)strlen(buf));
    }
    if(close(hd) != 0)
	mess_warning(owner().nodePath().c_str(), _("Closing the file %d error '%s (%d)'!"), hd, strerror(errno), errno);
    //if(!fOK) throw owner().err_sys(_("Error writing to file '%s'"), name().c_str());
    if(fOK) {
	mLoad = true;
	mAcces = time(NULL);
    }
    else mErr = true;
}

MFileArch::~MFileArch( )
{
    check();	//Check XML-archive

    if(mNode) delete mNode;
}

void MFileArch::delFile( )
{
    ResAlloc res(mRes, true);
    remove(name().c_str());
    remove((name()+(isPack()?".info":".gz.info")).c_str());
    mErr = true;
}

void MFileArch::attach( const string &iname, bool full )
{
    FILE *f = NULL;
    int bufSz = prmStrBuf_SZ;
    char buf[bufSz+1]; buf[bufSz] = 0;
    ResAlloc res(mRes, true);

    mErr = false;
    mLoad = false;
    mName = iname;
    mPack = mod->filePack(name());
    mAcces = time(NULL);

    try {
	//Check archive and unpack if need
	if(mPack) {
	    bool infoOK = false;
	    // Get archive info from info file
	    int hd = open((name()+".info").c_str(), O_RDONLY);
	    if(hd > 0) {
		int rsz = read(hd, buf, bufSz);
		if(rsz > 0 && rsz < bufSz) {
		    buf[rsz] = 0;
		    char bChars[100];
		    if(sscanf(buf,"%lx %lx %99s %hhd",&mBeg,&mEnd,bChars,&mXML) == 4) { mChars = bChars; infoOK = true; }
		}
		if(close(hd) != 0)
		    mess_warning(owner().nodePath().c_str(), _("Closing the file %d error '%s (%d)'!"), hd, strerror(errno), errno);
	    }

	    // Get archive info from DB
	    if(!infoOK) {
		TConfig cEl(&mod->packFE());
		cEl.cfg("FILE").setS(name());
		if(TBDS::dataGet((owner().infoTbl.size()?owner().infoTbl:mod->filesDB()),mod->nodePath()+"Pack/",cEl,TBDS::NoException)) {
		    mBeg = strtol(cEl.cfg("BEGIN").getS().c_str(),NULL,16);
		    mEnd = strtol(cEl.cfg("END").getS().c_str(),NULL,16);
		    mChars = cEl.cfg("PRM1").getS();
		    mXML = s2i(cEl.cfg("PRM2").getS());
		    infoOK = true;
		}
	    }

	    if(infoOK && (!mXML || !full)) {
		//  Get the file size
		int hd = open(name().c_str(), O_RDONLY);
		if(hd > 0) {
		    mSize = lseek(hd,0,SEEK_END);
		    if(close(hd) != 0)
			mess_warning(owner().nodePath().c_str(), _("Closing the file %d error '%s (%d)'!"), hd, strerror(errno), errno);
		}
		return;
	    }

	    try { mName = mod->unPackArch(name()); } catch(TError&) { mErr = true; return; }
	    mPack = false;
	}

	f = fopen(name().c_str(), "r");
	if(f == NULL) { mErr = true; return; }

	char s_char[100];
	//Check to plain text archive
	if(fgets(buf,bufSz,f) == NULL)
	    throw owner().err_sys(_("Error header of the file '%s'!"), name().c_str());
	string s_tmpl = MOD_ID "%*s %99s %x %x";
	if(sscanf(buf,s_tmpl.c_str(),s_char,&mBeg,&mEnd) == 3) {
	    // Attach plain text archive file
	    mChars = s_char;
	    mXML = false;
	    mLoad = true;
	    fseek(f, 0, SEEK_END);
	    mSize = ftell(f);

	    // Delete Node tree
	    if(mNode) delete mNode;
	    mNode = NULL;

	    if(fclose(f) != 0)
		mess_warning(owner().nodePath().c_str(), _("Closing the file %p error '%s (%d)'!"), f, strerror(errno), errno);
	}
	else {
	    if(!mNode) mNode = new XMLNode();
	    fseek(f, 0, SEEK_SET);
	    if(full) {
		// Load and parse all file
		int r_cnt;
		string s_buf;

		// Read full file to buffer
		while((r_cnt=fread(buf,1,bufSz,f))) s_buf.append(buf, r_cnt);
		if(fclose(f) != 0)
		    mess_warning(owner().nodePath().c_str(), _("Closing the file %p error '%s (%d)'!"), f, strerror(errno), errno);
		f = NULL;

		// Parse full file
		mNode->load(s_buf);
		if(mNode->name() != MOD_ID) {
		    owner().mess_sys(TMess::Error, _("The archive file '%s' is not mine."), name().c_str());
		    mNode->clear();
		    mErr = true;
		    return;
		}
		mSize = s_buf.size();
		mWrite = false;
		mChars = "UTF-8";
		mBeg = strtol(mNode->attr("Begin").c_str(), (char **)NULL, 16);
		mEnd = strtol(mNode->attr("End").c_str(), (char **)NULL, 16);
		mLoad = true;
		mXML = true;
		return;
	    }
	    else {
		// Process only archive header
		int c;
		string prm, val;

		do {
		    while((c=fgetc(f)) != '<' && c != EOF);
		    if(c == EOF) {
			owner().mess_sys(TMess::Error, _("Error the archive file '%s'."), name().c_str());
			mErr = true;
			if(fclose(f) != 0)
			    mess_warning(owner().nodePath().c_str(), _("Closing the file %p error '%s (%d)'!"), f, strerror(errno), errno);
			return;
		    }
		    prm.clear();
		    while((c=fgetc(f)) != ' ' && c != '\t' && c != '>' && c != EOF) prm += c;
		    if(c == EOF) {
			owner().mess_sys(TMess::Error, _("Error the archive file '%s'."), name().c_str());
			mErr = true;
			if(fclose(f) != 0)
			    mess_warning(owner().nodePath().c_str(), _("Closing the file %p error '%s (%d)'!"), f, strerror(errno), errno);
			return;
		    }
		} while(prm != MOD_ID);
		// Go to
		while(true) {
		    prm.clear();
		    val.clear();
		    while((c=fgetc(f)) == ' ' || c == '\t');
		    if(c == '>' || c == EOF) break;
		    while(c != '=' && c != '>' && c != EOF)	{ prm += c; c = fgetc(f); }
		    while((c=fgetc(f)) != '"' && c != '>' && c != EOF);
		    while((c=fgetc(f)) != '"' && c != '>' && c != EOF)  val += c;

		    if(prm == "Begin")		mBeg = strtol(val.c_str(),NULL,16);
		    else if(prm == "End")	mEnd = strtol(val.c_str(),NULL,16);
		}
		fseek(f, 0, SEEK_END);
		mSize = ftell(f);
		if(fclose(f) != 0)
		    mess_warning(owner().nodePath().c_str(), _("Closing the file %p error '%s (%d)'!"), f, strerror(errno), errno);
		mWrite = false;
		mLoad = false;
		mXML = true;
		return;
	    }
	}
    } catch(TError &err) {
	mess_err(err.cat.c_str(), "%s", err.mess.c_str());
	if(mNode) delete mNode;
	mNode = NULL;
	mErr = true;
	if(f && fclose(f) != 0)
	    mess_warning(owner().nodePath().c_str(), _("Closing the file %p error '%s (%d)'!"), f, strerror(errno), errno);
    }
}

bool MFileArch::put( TMess::SRec mess )
{
    if(mErr) throw owner().err_sys(_("Messages inserting to an error archive file!"));

    ResAlloc res(mRes, true);

    if(mPack) {
	try { mName = mod->unPackArch(name()); } catch(TError &err) { mErr = true; throw; }
	mPack = false;
    }

    mAcces = time(NULL);

    if(!mLoad) {
	res.release(); attach(mName); res.request(true);
	if(mErr || !mLoad) {
	    mErr = true;
	    throw owner().err_sys(_("The archive file '%s' isn't attached!"), name().c_str());
	}
    }

    if(xmlM()) {
	unsigned iSet = mNode->childSize();
	int64_t lastTm = 0;
	for(unsigned iCh = cacheGet(FTM(mess)), passCnt = 0; iCh < mNode->childSize(); iCh++) {
	    XMLNode *xIt = mNode->childGet(iCh);
	    int64_t xTm = FTM2(strtol(xIt->attr("tm").c_str(),(char **)NULL,16), s2i(xIt->attr("tmu")));
	    if(xTm >= FTM(mess)) {
		if(iSet >= mNode->childSize())	iSet = iCh;
		if(xTm > FTM(mess))	break;
	    }
	    if((owner().prevDbl() || owner().prevDblTmCatLev()) && xTm == FTM(mess) && s2i(xIt->attr("lv")) == mess.level && xIt->attr("cat") == mess.categ) {
		if(xIt->text() == mess.mess) return true;
		else if(owner().prevDblTmCatLev()) { xIt->setText(mess.mess); mWrite = true; return true; }
	    }
	    //  Adding too big positions to the cache
	    if((passCnt++) > CACHE_POS && xTm != lastTm) { cacheSet(xTm, iCh); passCnt = 0; }
	    lastTm = xTm;
	}

	XMLNode *cl_node = mNode->childIns(iSet, "m");
	cl_node->setAttr("tm", i2s(mess.time,TSYS::Hex))->
		 setAttr("tmu", i2s(mess.utime))->
		 setAttr("lv", i2s(mess.level))->
		 setAttr("cat", mess.categ)->
		 setText(mess.mess);
	if(mess.time > mEnd) {
	    mEnd = mess.time;
	    mNode->setAttr("End",i2s(mEnd,TSYS::Hex));
	}
	mWrite = true;
	return true;
    }
    else {
	unsigned int tTm, tTmU;
	long mv_beg = 0, mv_off = 0;
	int bufSz = prmStrBuf_SZ;
	char buf[bufSz+1]; buf[bufSz] = 0;
	//Check to empty category and message
	if(!mess.categ.size())	mess.categ = " ";
	if(!mess.mess.size())	mess.mess = " ";
	//Open file
	FILE *f = fopen(name().c_str(), "r+");
	if(f == NULL) { mErr = true; return false; }
	bool fOK = true;

	//Prepare mess
	string s_buf = TSYS::strMess("%x:%d %d %s %s", (unsigned int)mess.time, mess.utime, mess.level,
	    Mess->codeConvOut(mChars,TSYS::strEncode(mess.categ,TSYS::Custom," \n\t%")).c_str(),
	    Mess->codeConvOut(mChars,TSYS::strEncode(mess.mess,TSYS::Custom," \n\t%")).c_str())+"\n";
	mv_off = s_buf.size();

	//Check for duples
	if(mess.time <= mEnd && (owner().prevDbl() || owner().prevDblTmCatLev())) {
	    long c_off = cacheGet(FTM(mess));
	    if(c_off) fseek(f, c_off, SEEK_SET);
	    else fOK = (fgets(buf,bufSz,f) != NULL);

	    while(fgets(buf,bufSz,f) != NULL) {
		int tLev = 0;
		char tCat[1001];
		if((sscanf(buf,"%x:%d %d %1000s",&tTm,&tTmU,&tLev,tCat)) < 4) continue;
		int64_t xTm = FTM2(tTm, tTmU);

		if(xTm >= FTM(mess)) {
		    if(!mv_beg)	mv_beg = ftell(f) - strlen(buf);
		    if(xTm > FTM(mess)) break;
		}
		if(tTm == mess.time && s_buf == buf) {
		    if(fclose(f) != 0)
			mess_warning(owner().nodePath().c_str(), _("Closing the file %p error '%s (%d)'!"), f, strerror(errno), errno);
		    return true;
		}
		if(owner().prevDblTmCatLev() && xTm == FTM(mess) && tLev == mess.level &&
		    TSYS::strDecode(Mess->codeConvIn(mChars,tCat),TSYS::HttpURL) == mess.categ)
		{
		    if(s_buf.size() < strlen(buf))
			s_buf = s_buf.substr(0,s_buf.size()-strlen("\n")) + string(strlen(buf)-s_buf.size(),' ') + "\n";
		    mv_beg = ftell(f) - strlen(buf); mv_off = s_buf.size() - strlen(buf);
		    break;
		}
	    }
	    fseek(f, 0, SEEK_SET);
	}

	//Put message to the end
	if(fOK && (mess.time >= mEnd)) {
	    //Update header
	    if(mess.time != mEnd) {
		mEnd = mess.time;
		snprintf(buf, bufSz, "%s %s %s %8x %8x\n", MOD_ID, MOD_VER, mChars.c_str(), (unsigned int)mBeg, (unsigned int)mEnd);
		fOK = (fwrite(buf,strlen(buf),1,f) == 1);
	    }
	    //Put mess to end file
	    fseek(f,0,SEEK_END);
	    fOK = fOK && (fwrite(s_buf.data(),s_buf.size(),1,f) == 1);
	}
	//Put message to inwards
	else {
	    if(fOK && !mv_beg) {
		// Get need position
		long c_off = cacheGet(FTM(mess));
		if(c_off) fseek(f,c_off,SEEK_SET);
		else fOK = (fgets(buf,bufSz,f) != NULL);

		// Check mess records
		int passCnt = 0;
		int64_t lastTm = 0;
		while(!mv_beg && fgets(buf,bufSz,f) != NULL) {
		    sscanf(buf, "%x:%d %*d", &tTm, &tTmU);
		    int64_t xTm = FTM2(tTm, tTmU);
		    if(xTm >= FTM(mess)) mv_beg = ftell(f) - strlen(buf);
		    //  Adding too big positions to the cache
		    else if((passCnt++) > CACHE_POS && xTm != lastTm) {
			cacheSet(xTm, ftell(f)-strlen(buf));
			passCnt = 0;
		    }
		    lastTm = xTm;
		}
	    }
	    if(fOK && mv_beg) {
		if(mv_off) {
		    fseek(f, 0, SEEK_END);
		    int mv_end = ftell(f);
		    int beg_cur;
		    do {
			beg_cur = ((mv_end-mv_beg) >= bufSz) ? mv_end-bufSz : mv_beg;
			fseek(f, beg_cur, SEEK_SET);
			fOK = fOK && (fread(buf,mv_end-beg_cur,1,f) == 1);
			fseek(f, beg_cur+mv_off, SEEK_SET);
			fOK = fOK && (fwrite(buf,mv_end-beg_cur,1,f) == 1);
			mv_end -= bufSz;
		    } while(fOK && beg_cur != mv_beg);
		}
		//  Write a new message
		fseek(f, mv_beg, SEEK_SET);
		fOK = fOK && (fwrite(s_buf.c_str(),s_buf.size(),1,f) == 1);
		cacheUpdate(FTM(mess), mv_off);
		//  Put the last value to the cache
		//cacheSet(FTM(mess), mv_beg, true);	//!!!! That can be wrong for several records with the equal time
	    }
	}
	fseek(f, 0, SEEK_END);
	mSize = ftell(f);
	if(fclose(f) != 0)
	    mess_warning(owner().nodePath().c_str(), _("Closing the file %p error '%s (%d)'!"), f, strerror(errno), errno);
	if(!fOK) owner().mess_sys(TMess::Error, _("Error writing to the archive file '%s': %s(%d)"), name().c_str(), strerror(errno), errno);

	return fOK;
    }

    return true;
}

time_t MFileArch::get( time_t bTm, time_t eTm, vector<TMess::SRec> &mess, const string &category, char level, time_t upTo )
{
    TMess::SRec bRec;

    if(mErr) throw owner().err_sys(_("Messages getting from an error archive file!"));

    ResAlloc res(mRes, true);
    if(!upTo) upTo = time(NULL) + prmInterf_TM;

    //res.request(true);
    if(mPack) {
	try { if(mPack) mName = mod->unPackArch(name()); } catch(TError &err) { mErr = true; throw; }
	mPack = false;
    }
    res.request(false);

    mAcces = time(NULL);

    if(!mLoad) {
	res.release(); attach(mName); res.request(false);
	if(mErr || !mLoad) throw owner().err_sys(_("Archive file isn't attached!"));
    }

    TRegExp re(category, "p");

    bTm = vmax(bTm, begin());
    eTm = vmin(eTm, end());
    time_t result = bTm;
    if(xmlM()) {
	int64_t lastTm = 0;
	for(unsigned iCh = cacheGet((int64_t)bTm*1000000), passCnt = 0; iCh < mNode->childSize() && time(NULL) < upTo; iCh++) {
	    //Find messages
	    bRec.time = strtol(mNode->childGet(iCh)->attr("tm").c_str(), (char**)NULL, 16);
	    bRec.utime = s2i(mNode->childGet(iCh)->attr("tmu"));
	    if(bRec.time > eTm) break;
	    if(bRec.time >= bTm) {
		result = bRec.time;
		bRec.level = (TMess::Type)s2i(mNode->childGet(iCh)->attr("lv"));
		bRec.categ = mNode->childGet(iCh)->attr("cat");
		if(!TMess::messLevelTest(level,bRec.level) || !re.test(bRec.categ)) continue;
		bRec.mess  = mNode->childGet(iCh)->text();
		bool equal = false;
		int iP = mess.size();
		for(int iM = mess.size()-1; iM >= 0; iM--) {
		    if(FTM(mess[iM]) > FTM(bRec)) iP = iM;
		    else if(FTM(mess[iM]) == FTM(bRec) && bRec.level == mess[iM].level && bRec.categ == mess[iM].categ &&
			    (owner().prevDblTmCatLev() || bRec.mess == mess[iM].mess)) {
			if(owner().prevDblTmCatLev()) mess[iM] = bRec;	//Replace previous as the archieved is priority
			equal = true;
			break;
		    }
		    else if(FTM(mess[iM]) < FTM(bRec)) break;
		}
		if(!equal) mess.insert(mess.begin()+iP, bRec);
	    }
	    if((passCnt++) > CACHE_POS && FTM(bRec) != lastTm) {
		cacheSet(FTM(bRec), iCh);
		passCnt = 0;
	    }
	    lastTm = FTM(bRec);
	}
    }
    else {
	int bufSz = 102000;
	char buf[bufSz+1]; buf[bufSz] = 0;
	//Open file
	FILE *f = fopen(name().c_str(), "r");
	if(f == NULL) { mErr = true; return eTm; }

	//Get want position
	bool rdOK = true;
	long c_off = cacheGet((int64_t)bTm*1000000);
	if(c_off) fseek(f, c_off, SEEK_SET);
	else rdOK = (fgets(buf,bufSz,f) != NULL);
	//Check mess records
	int passCnt = 0;
	int64_t lastTm = 0;
	while((rdOK=(fgets(buf,bufSz,f)!=NULL)) && time(NULL) < upTo) {
	    char stm[51]; int off = 0, bLev;
	    if(sscanf(buf,"%50s %d",stm,&bLev) != 2) continue;
	    bRec.level = (TMess::Type)bLev;
	    bRec.time = strtol(TSYS::strSepParse(stm,0,':',&off).c_str(),NULL,16);
	    bRec.utime = s2i(TSYS::strSepParse(stm,0,':',&off));
	    if(bRec.time > eTm) break;
	    if(bRec.time >= bTm) {
		result = bRec.time;
		if(!TMess::messLevelTest(level,bRec.level)) continue;
		char m_cat[1001], m_mess[100001];
		if(sscanf(buf,"%*x:%*d %*d %1000s %100000s",m_cat,m_mess) != 2) continue;
		bRec.categ = TSYS::strDecode(Mess->codeConvIn(mChars,m_cat), TSYS::HttpURL);
		bRec.mess  = TSYS::strDecode(Mess->codeConvIn(mChars,m_mess), TSYS::HttpURL);
		if(!re.test(bRec.categ)) continue;
		// Check to equal messages and inserting
		bool equal = false;
		int iP = mess.size();
		for(int iM = mess.size()-1; iM >= 0; iM--)
		    if(FTM(mess[iM]) > FTM(bRec)) iP = iM;
		    else if(FTM(mess[iM]) == FTM(bRec) && bRec.level == mess[iM].level && bRec.categ == mess[iM].categ &&
			    (owner().prevDblTmCatLev() || bRec.mess == mess[iM].mess)) {
			if(owner().prevDblTmCatLev()) mess[iM] = bRec;	//Replace previous as the archieved is priority
			equal = true;
			break;
		    }
		    else if(FTM(mess[iM]) < FTM(bRec)) break;
		if(!equal) mess.insert(mess.begin()+iP, bRec);
	    }
	    else if((passCnt++) > CACHE_POS && FTM(bRec) != lastTm) {
		cacheSet(FTM(bRec), ftell(f)-strlen(buf));
		passCnt = 0;
	    }
	    lastTm = FTM(bRec);
	}
	if(fclose(f) != 0)
	    mess_warning(owner().nodePath().c_str(), _("Closing the file %p error '%s (%d)'!"), f, strerror(errno), errno);
    }

    return result;
}

void MFileArch::check( bool free )
{
    ResAlloc res(mRes, true);
    if(!mErr && mLoad && xmlM()) {
	if(mWrite) {
	    int hd = open(name().c_str(), O_RDWR|O_TRUNC, SYS->permCrtFiles());
	    if(hd > 0) {
		string x_cf = mNode->save(XMLNode::XMLHeader|XMLNode::BrOpenPrev);
		mSize = x_cf.size();
		mWrite = !(write(hd,x_cf.c_str(),mSize) == mSize);
		if(mWrite) owner().mess_sys(TMess::Error, _("Error writing to '%s'!"), name().c_str());
		if(close(hd) != 0)
		    mess_warning(owner().nodePath().c_str(), _("Closing the file %d error '%s (%d)'!"), hd, strerror(errno), errno);
	    }
	}
	//Free memory of XML-archive after 10 minets
	if(time(NULL) > mAcces + owner().packTm()*30 || free) {
	    mNode->clear();
	    mLoad = false;
	}
    }
    //Check for pack archive file
    if(!mErr && !mPack && owner().packTm() && time(NULL) > (mAcces+owner().packTm()*60) && ((xmlM() && !mLoad) || !xmlM())) {
	mName = mod->packArch(name());
	mPack = true;
	// Get file size
	int hd = open(name().c_str(), O_RDONLY);
	if(hd > 0) {
	    mSize = lseek(hd, 0, SEEK_END);
	    if(close(hd) != 0)
		mess_warning(owner().nodePath().c_str(), _("Closing the file %d error '%s (%d)'!"), hd, strerror(errno), errno);
	}

	if(!owner().packInfoFiles() || owner().infoTbl.size()) {
	    // Write info to DB
	    TConfig cEl(&mod->packFE());
	    cEl.cfg("FILE").setS(name());
	    cEl.cfg("BEGIN").setS(ll2s(begin(),TSYS::Hex));
	    cEl.cfg("END").setS(ll2s(end(),TSYS::Hex));
	    cEl.cfg("PRM1").setS(charset());
	    cEl.cfg("PRM2").setS(i2s(xmlM()));
	    TBDS::dataSet((owner().infoTbl.size()?owner().infoTbl:mod->filesDB()), mod->nodePath()+"Pack/", cEl, TBDS::NoException);
	}
	else if((hd=open((name()+".info").c_str(),O_WRONLY|O_CREAT|O_TRUNC,SYS->permCrtFiles())) > 0) {
	    // Write info to info file
	    string si = TSYS::strMess("%lx %lx %s %d",begin(),end(),charset().c_str(),xmlM());
	    if(write(hd,si.data(),si.size()) != (int)si.size())
		mod->mess_sys(TMess::Error, _("Error writing to '%s'!"), (name()+".info").c_str());
	    if(close(hd) != 0)
		mess_warning(owner().nodePath().c_str(), _("Closing the file %d error '%s (%d)'!"), hd, strerror(errno), errno);
	}
    }
}

long MFileArch::cacheGet( int64_t tm )
{
    CacheEl rez = {0, 0};

    dtRes.lock();
    for(int iC = cache.size()-1; iC >= 0; iC--)
	if(tm >= cache[iC].tm) { rez = cache[iC]; break; }
    if(tm >= cach_pr.tm && cach_pr.tm >= rez.tm) rez = cach_pr;
    dtRes.unlock();

    return rez.off;
}

void MFileArch::cacheSet( int64_t tm, long off, bool last )
{
    CacheEl el = { tm, off };

    MtxAlloc res(dtRes, true);
    if(!last) {
	for(unsigned iC = 0; iC < cache.size(); iC++)
	    if(el.tm == cache[iC].tm)		{ cache[iC] = el; return; }
	    else if(el.tm < cache[iC].tm)	{ cache.insert(cache.begin()+iC, el); return; }
	cache.push_back(el);
    }
    else cach_pr = el;
}

void MFileArch::cacheUpdate( int64_t tm, long v_add )
{
    dtRes.lock();
    for(unsigned iC = 0; iC < cache.size(); iC++)
	if(cache[iC].tm > tm) cache[iC].off += v_add;
    if(cach_pr.tm > tm) cach_pr.off += v_add;
    dtRes.unlock();
}
