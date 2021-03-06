// irpapp.cpp : implementation file
//
// Copyright � 1998-2015 Casey McSpadden   
//		mailto:casey@crossriver.com
//		http://www.crossriver.com/
//
// ==========================================================================  
// DESCRIPTION:	
// ==========================================================================
// CMIrpApplication is the main IRPSIM class and the point of entry for all
// IRPSIM functionality
// ==========================================================================
//
// ==========================================================================  
// HISTORY:	
// ==========================================================================
//			1.00	09 March 2015	- Initial re-write and release.
// ==========================================================================
//
/////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "irpapp.h"
#include "accum.h"
#include "vtime.h"
#include "vartypes.h"
#include "varcol.h"
#include "interval.h"
#include "regions.h"
#include "defines.h"
#include "reliab.h"
#include "simulat.h"
#include "scenario.h"
#include "script.h"
#include "simarray.h"
#include "savesima.h"
#include "notify.h"

#include "cmlib.h"
#include "token.h"

#include <stdio.h>
#include <iomanip>
#include <ctype.h>

//#include <fstream>
//static wofstream sdebug(L"debug_irpapp.txt");

//template class _IRPCLASS CMPSSmallArray<CMVariable>;

CMIrpApplication::CMIrpApplication() :
scenarios(),
simulations(),
outputvars(),
loadedfiles(),
//attachedfiles(),
variables(0),
oldvariables(0),
currentscript(0),
currentscenario(0),
errorcode(0),
vartypes()
{
	options = new CMOptions();
	casesensitiveflag = CMString::set_case_sensitive(0);
	skipwhitespaceflag = CMString::skip_whitespace(1);
    variables = new CMVariableCollection(1);
	oldvariables =	CMVariable::SetCollectionContext(variables);
}

CMIrpApplication::~CMIrpApplication()
{
	ResetApplication();
    //CMVariable::SetCollectionContext(oldvariables);
	delete options;
    delete variables;
	CMString::set_case_sensitive(casesensitiveflag);
	CMString::skip_whitespace(skipwhitespaceflag);
}

unsigned CMIrpApplication::SimulationsCount()
{
	return simulations.Count();
}

void CMIrpApplication::OpenProject(const CMString& name)
{
	ResetApplication();
	CMVariable::SetCollectionContext(variables);
//	project_file = getfullpathname(name.c_str());
	CMString strName(name);
	int varsread = 0;
	m_strProjectFile = name;
	CMNotifier::Notify(CMNotifier::LOG, L"Opening Project: " + m_strProjectFile);
	read_file(name, varsread);
	if (varsread) {
		update_variable_links();
		check_category_integrity();
	}
	//AddFiles(&strName,1);
}

/*
struct _thread_data {
	CMIrpApplication* pApp;
	CMString* pFileNames;
	int nFiles;
	//DWORD dwThreadId;
};
*/

int CMIrpApplication::read_file(const CMString& name,int &varsread)
{
	static const wchar_t* delims = L"# \t\r\n";
	CMString messageheader = L"Reading ";
	CMString errorheader = L"problem reading file ";
	wifstream* stack[128];
	int current=0,currfile,oldfile,defno;
	CMString currpath = name;

	stack[0] = new wifstream(name.c_str(),ios::in|IOS_BINARY);
	wifstream* s = stack[0];

	if (s->fail()) {
		CMNotifier::Notify(CMNotifier::ERROR, errorheader+name);
		return -1;
	}

	currfile = add_file_to_list(name);
	defno = 1;

	CMNotifier::Notify(CMNotifier::LOG, messageheader + name);

    CMTimeMachine tm(CM_MONTH,1);
	int err = 0;
	while (s && !err) {
		CMString line;
		long index = (long)s->tellg();
		line.read_line(*s);
		line = stripends(line);
	
		if (!line.is_null() && line[0]==L'#') {
			CMTokenizer next(line);
			CMString token = next(delims);
			if (token == L"include") {
				CMString fname = stripends(CMString(line.c_str() + 8));
				currpath = getabsolutepath(name.c_str(), fname.c_str());

				s = stack[++current] = new wifstream(currpath.c_str(),ios::in|IOS_BINARY);
				if (s->fail()) {
					CMNotifier::Notify(CMNotifier::ERROR, errorheader + fname);
					err = -1;
				}
				else {
					defno = 0;
					oldfile=currfile;
					currfile = add_file_to_list(currpath);
					CMNotifier::Notify(CMNotifier::LOG, messageheader + currpath);
				}
			}
			/*
			else if (token == L"attach") {
				CMString fname = next(delims);
				add_file_to_list(fname,FILE_ATTACHED);
				CMNotifier::Notify(CMNotifier::INFO, messageheader + fname, true);
			}
			*/
			else if (token == L"define") {
				defno++;
				CMString s1 = next(delims);
				CMString s2 = next(delims);
				CMDefinitions::Add(s1,s2,currfile);
			}
			else if (token==L"options") {
				defno++;
				CMNotifier::Notify(CMNotifier::INFO, messageheader + L"options");
				options->SetApplicationId(currfile);
				options->SetApplicationIndex(defno);
				s->seekg(index);
				*s >> *options;
			}
			else if (token==L"intervals") {
				defno++;
				CMNotifier::Notify(CMNotifier::INFO, messageheader + L"intervals");
				CMInterval::Read(*s);
				CMInterval::SetApplicationIdAll(currfile);
				CMInterval::SetApplicationIndexAll(defno);
			}
			else if (token==L"scenario") {
				defno++;
				s->seekg(index);
				CMScenario* sce = new CMScenario(currfile);
				*s >> *sce;
				scenarios.Add(sce);
				sce->SetApplicationIndex(defno);
				CMNotifier::Notify(CMNotifier::INFO, messageheader + sce->GetName());
			}
			else if (token==L"script") {
				defno++;
				s->seekg(index);
				CMScript* scr = new CMScript(currfile);
				*s >> *scr;
				scripts.Add(scr);
				scr->SetApplicationIndex(defno);
				CMNotifier::Notify(CMNotifier::INFO, messageheader + scr->GetName());
			}
			else if (token==L"category") {
				defno++;
				s->seekg(index);
				CMCategory* cat = new CMCategory(currfile);
				*s >> *cat;
				CMCategory::AddCategory(cat);
				cat->SetApplicationIndex(defno);
				CMNotifier::Notify(CMNotifier::INFO, messageheader + cat->GetName());
			}
			else if (token==L"vardef") {
				defno++;
				if (!varsread)
					variables->SetStateAll(CMVariable::vsLinksUpdated,FALSE);
				varsread++;
				s->seekg(index);
				CMVariable* v;
				*s >> v;
				if (v) {
					if (variables->Find(v->GetName()))
						delete v;
					else {
						variables->Add(v);
						v->SetApplicationId(currfile);
						v->SetApplicationIndex(defno);
						CMNotifier::Notify(CMNotifier::INFO, messageheader + v->GetName());
					}
				}
			}
			else {
				CMNotifier::Notify(CMNotifier::ERROR, L"Unrecognized token: #" + token + L" in file " + currpath);
			}
		} // if (!line.is_null() && line[0]==L'#') {
		if (s->eof()) {
			delete stack[current--];
			s = current >= 0 ? stack[current] : 0;
			currfile = oldfile;
			defno = 0;
		}
	} // while (s && !err) {

	if (current >= 0)
		for (int i=0;i<=current;i++)
			delete stack[i];

	return err;
}

BOOL CMIrpApplication::update_variable_links()
{
	BOOL ret = TRUE;
	CMVariable* v;
    CMString region,vtype,vname;
	const wchar_t* aggname;

	CMNotifier::Notify(CMNotifier::LOG, L"Updating variable types");

	// Destroy aggregate regional variables
	
	variables->DestroyIfState(CMVariable::vsRegional,0);
	
	// update regions
	CMRegions::Reset();
	CMVariableIterator iter;
	while ((v=iter())!=0)
   	v->SetRegion(CMRegions::GetRegionId(v->GetAssociation(L"region")));
	iter.Reset();
	while ((v=iter())!=0) {
		if (v->GetRegion()>=0) {
			region = CMRegions::GetRegionName(v->GetRegion());
   		for (int i=0;v->GetAssociation(i,vtype,vname);i++) {
				if (CMVariableTypes::VarIntFromString(vtype.c_str(),1)>=0) {
					CMVariable* vfound = CMVariable::Find(vname);
         		if (vfound) {
           	   	vfound->AddAssociation(L"region",region);
			     	   vfound->SetRegion(v->GetRegion());
					}
				}
     	   }
      }
	}

	if (CMRegions::RegionCount()>0) {
		//swprintf_s(buffer,128,L"%d Regions Defined",CMRegions::RegionCount());
		//CMNotifier::Notify(CMNotifier::LOG, buffer);
		for (unsigned short i=0;i<CMRegions::RegionCount();i++)
			CMNotifier::Notify(CMNotifier::LOG, L"Region " + CMRegions::GetRegionName(i) + L" defined");
	}

	// recreate aggregate regional variables

	for (int i=0;(aggname=CMVariableTypes::AggStringFromInt(i))!=0;i++) {
		for (unsigned short j=0;j<CMRegions::RegionCount();j++) {
			CMVariable* v  = new CMVariable((CMString)aggname + L"." + CMRegions::GetRegionName(j),CMVariable::vsAggregate | CMVariable::vsRegional);
			v->SetType(i-1000);
			if (CMVariableTypes::IsAggSum(i)) v->SetState(CMVariable::vsSum,TRUE);
			variables->Add(v);
		}
	}

	// update variable associations for nodes
	
	variables->UpdateVariableTypes();
		
	// find missing variables
	
	iter.Reset();
	//CMNotifier::Notify(CMNotifier::LOG, L"\r\nLooking for missing variables:");
	iter.Reset();
	long vcount = variables->Count();
	for (long n=0;;n++) {
		if ((v=iter())==0)
			break;
		if (v->GetState(CMVariable::vsAggregate | CMVariable::vsSystem))
			continue;
		int pct = (int)(100*n/vcount);
		CMIrpObjectIterator* ni = v->CreateIterator();
		const wchar_t* vname;
		int first=1;
		while (ni && ((vname=ni->GetNext())!=0)) {
			if (!CMVariable::Find(vname) && !CMDefinitions::IsDefined(vname)) {
				//if (first)
					//CMNotifier::Notify(CMNotifier::LOG, CMString(L"\r\n") + v->GetName() + L"\r\n missing:");
				//first=0;
				ret = FALSE;
				CMString msg(L"Undefined variable or definition " + CMString(vname) + L" in definition of " + v->GetName());
				CMNotifier::Notify(CMNotifier::LOG, msg);
				CMNotifier::Notify(CMNotifier::ERROR, msg);
			}
		}
	}
	//CMNotifier::Notify(CMNotifier::LOG, L"\r\nFinished\r\n");
	//CMNotifier::Notify(CMNotifier::INFO, L"");
	return ret;
}

BOOL CMIrpApplication::check_category_integrity()
{
	BOOL ret = TRUE;
	unsigned short n = 0;
	CMCategory *pCat;
	while ((pCat = CMCategory::GetCategory(n)) != 0) {
		for (int i = 0; i < pCat->MemberCount(); i++) {
			const CMString& strName = pCat->GetMemberName(i);
			if (CMVariable::Find(strName) == 0) {
				CMString msg(L"Undefined variable " + strName + L" in definition of category " + pCat->GetName());
				CMNotifier::Notify(CMNotifier::WARNING, msg);
				ret = FALSE;
			}
		}
		n++;
	}
	return ret;
}

CMString CMIrpApplication::GetObjectFileName(CMIrpObject* pObject)
{
	CMString ret;
	if (pObject) ret = LoadedFile(pObject->GetApplicationId());
	return ret;
}

CMScenario* CMIrpApplication::UseScenario(const CMString& name)
{
	try {
		CMVariable::SetCollectionContext(variables);
		ResetOutputVariables();
		unsigned short n;
		for (n = 0; n < scenarios.Count(); n++)
			if (name == scenarios[n]->GetName())
				break;
		if (n < scenarios.Count()) {
			currentscenario = scenarios[n];
			scenarios[n]->Use(options);
			for (unsigned i = 0; i < scenarios[n]->Variables(); i++) {
				CMString varname = scenarios[n]->VariableName(i);
				outputvars.Add(varname);
				/*
				for (unsigned j = 0; j < simulations.Count(); j++) {
					CMSimulationArray* array = simulations[j]->SimArray();
					CMAccumulatorArray* accum = simulations[j]->Accumulator();
					if (array)
						array->SetVariableState(array->VariableIndex(varname), CMVariableDescriptor::vdOutput, TRUE);
					if (accum)
						accum->SetVariableState(accum->VariableIndex(varname), CMVariableDescriptor::vdOutput, TRUE);
				}
				*/
			}
		}
		else {
			currentscenario = NULL;
			CMVariableIterator next;
			CMVariable* v;
			while ((v = next()) != 0) {
				v->SetState(CMVariable::vsSelected | CMVariable::vsSaveOutcomes, FALSE);
			}
		}
	}
   catch (CMException& except) {
	   CMNotifier::Notify(CMNotifier::ERROR, except.What());
   }
	return currentscenario;
}

CMScript* CMIrpApplication::FindScript(const CMString& name)
{
	for (unsigned short i=0;i<scripts.Count();i++)
   	if (name == scripts[i]->GetName())
      	return scripts[i];
   return NULL;
}

CMScenario* CMIrpApplication::FindScenario(const CMString& name)
{
	for (unsigned short i=0;i<scenarios.Count();i++)
   	if (name == scenarios[i]->GetName())
      	return scenarios[i];
   return NULL;
}

CMCategory* CMIrpApplication::FindCategory(const CMString& name)
{
	return CMCategory::FindCategory(name);	
}

CMIrpObject* CMIrpApplication::FindIrpObject(const CMString& name)
{
	CMIrpObject* pi = CMVariable::Find(name);	if (pi) return pi;
	
	pi = FindCategory(name); if (pi) return pi;

	pi = FindScript(name);	if (pi) return pi;

	pi = FindScenario(name); if (pi) return pi;

	return NULL;
}


CMScript* CMIrpApplication::UseScript(const CMString& name)
{
	return (currentscript = FindScript(name));
}

void CMIrpApplication::SetVariableStateAll(ULONG aState,BOOL action)
{
	if (variables) variables->SetStateAll(aState,action);
}

void CMIrpApplication::UpdateVariableLinksAll()
{
	if (variables) variables->UpdateVariableLinks();
}

void CMIrpApplication::UpdateVariableLinkStatusAll()
{
	if (variables) variables->UpdateLinkStatus();
}

void CMIrpApplication::ResetApplication()
{
	simulations.ResetAndDestroy(1);
	if (variables)
	   variables->ResetCollection();
	CMRegions::Reset();
	CMDefinitions::Reset();
	CMInterval::Reset();
	CMCategory::DestroyCategories();
	CMNode::DestroyNodes();
	CMNode::ResetAggregateVariables(false);
	scenarios.ResetAndDestroy(1);
	scripts.ResetAndDestroy(1);
	outputvars.Reset(1);
	loadedfiles.Reset(1);
	//attachedfiles.Reset(1);
	currentscript = NULL;
	currentscenario = NULL;
	m_strProjectFile = L"";
}

CMSimulation* CMIrpApplication::RunningSimulation()
{
	for (unsigned i=0;i<SimulationsCount();i++)
		if (simulations[i]->Running())
			return simulations[i];
	return NULL;
}


CMSimulation* CMIrpApplication::CreateSimulation()  //const wchar_t* pFileName)
{
	CMSimulation* newsim = NULL;

	if (currentscript == NULL) {
		CMNotifier::Notify(CMNotifier::ERROR, L"Cannot create simulation: No script selected");
		return NULL;
	}

	try {
		CMVariable::SetCollectionContext(variables);
		if (!RunningSimulation()) {
			newsim = new CMSimulation(this);
			if (newsim->Fail()) {
				delete newsim;
				CMNotifier::Notify(CMNotifier::ERROR, L"Unable to create simulation");
				newsim = NULL;
			}
		}

		if (newsim)
			simulations.Add(newsim);
	}

	catch (CMException& except) {
		CMNotifier::Notify(CMNotifier::ERROR, except.What());
	}

	return (newsim);
}

void CMIrpApplication::DeleteSimulation(CMSimulation* pSim,int save)
{
	try {
		for (unsigned i=0;i<simulations.Count();i++) {
			if (pSim==simulations[i]) {
				//CMString fname = pSim->GetFileName();
				//if (save) pSim->Save();
				simulations.DetachAt(i,1);
				/*
				if (save) {
					if (add_file_to_list(fname,FILE_ATTACHED)>=0)
						Synchronize(SYNC_FILE_ATTACHED,NULL);
				}
				*/
				break;
			}
		}
   }
   catch (CMException& except) {
	   CMNotifier::Notify(CMNotifier::ERROR, except.What());
   }
}

BOOL CMIrpApplication::RunSimulation(CMSimulation* pSim)
{
	if (pSim==NULL || RunningSimulation())
		return FALSE;
	if (currentscript == NULL) {
		CMNotifier::Notify(CMNotifier::ERROR, L"Cannot run simulation: No script selected");
			return FALSE;
	}
	if (!pSim->Initialized()) {
		for (unsigned short i=0;i<scripts.Count();i++)
			scripts[i]->Parse(*this);
		pSim->SetScript(currentscript->GetName());
	}
	BOOL success = pSim->Run();
	if (!success)
		CMNotifier::Notify(CMNotifier::ERROR, L"Problem running the simulation");
	return success;
}


BOOL CMIrpApplication::PauseSimulation(CMSimulation* pSim,BOOL bAction)
{
	if (pSim) pSim->Pause(bAction);
	return TRUE;
}

int CMIrpApplication::WriteOutcomes(const CMString& filename, CMSimulation* sim)
{
	if (!sim) return -1;
	CMSaveSimulationAscii report(*sim, this);
	for (unsigned i = 0; i < outputvars.Count(); i++)
		report.AddOutputVariable(outputvars[i]);
	CMVariableIterator iter;
	CMVariable* v;
	while ((v = iter()) != 0)
		if (v->GetState(CMVariable::vsSaveOutcomes) && !outputvars.Contains(v->GetName()))
			report.AddOutputVariable(v->GetName());
	return report.Outcomes(filename);
}

int CMIrpApplication::WriteSummary(const CMString& filename,CMSimulation* sim)
{
	if (!sim) return -1;
	CMSaveSimulationAscii report(*sim,this);
	for (unsigned i=0;i<outputvars.Count();i++)
   	report.AddOutputVariable(outputvars[i]);
	CMVariableIterator iter;
   CMVariable* v;
   while ((v=iter())!=0)
	   if (v->GetState(CMVariable::vsSaveOutcomes) && !outputvars.Contains(v->GetName()))
	   	   report.AddOutputVariable(v->GetName());
   return report.Summary(filename);
}

int CMIrpApplication::add_file_to_list(const CMString& path)
{
	int nRet = -1;
	loadedfiles.Add(path);
	nRet = loadedfiles.Count() - 1;
	return nRet;
}
