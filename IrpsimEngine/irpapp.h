// irpapp.h : header file
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
#pragma once

#include "cmdefs.h"
#include "smparray.h"
#include "smvarray.h"
#include "units.h"

#include "irp.h"
#include "options.h"
#include "vartypes.h"
#include "script.h"
#include "category.h"
#include "scenario.h"
#include "simulat.h"
#include "variable.h"
#include "varcol.h"

#include <fstream>
using namespace std;

class _IRPCLASS CMReliability;
class _IRPCLASS CMSimulationArray;

class _IRPCLASS CMIrpApplication
{
protected:
	//******** DATA
	CMString m_strProjectFile;
    CMScript* currentscript;
    CMScenario* currentscenario;
	int errorcode;
	CMOptions* options;
	CMVariableTypes vartypes;
	CMVariableCollection* oldvariables;
	CMVariableCollection* variables;

	struct _pct_complete_struct {
		int nVal;
		const wchar_t* pTitle;
	};
	
	class _IRPCLASS CMPSmallArray<CMScenario> scenarios;
	class _IRPCLASS CMPSmallArray<CMScript> scripts;
	class _IRPCLASS CMPSmallArray<CMSimulation> simulations;
	class _IRPCLASS CMVSmallArray<CMString> outputvars;
	class _IRPCLASS CMVSmallArray<CMString> loadedfiles;
	//class _IRPCLASS CMVSmallArray<CMString> attachedfiles;

	int casesensitiveflag;
	int skipwhitespaceflag;

	//********

	// Protected virtual functions

	//static DWORD WINAPI add_file_proc(LPVOID lpParameter);
	BOOL update_variable_links(); // Updates and checks integrity of variable links
	BOOL check_category_integrity(); // Checks integrity of categories
	int add_file_to_list(const CMString& name); // adds a file to appropriate list
	int read_file(const CMString& name,int& varsread);

	void SetVariableStateAll(ULONG aState, BOOL action);
	void UpdateVariableLinksAll();
	void UpdateVariableLinkStatusAll();
	//int DestroyVariable(const CMString& vname);
	void ResetOutputVariables() { outputvars.Reset(1); }
	void AddOutputVariable(const CMString& name) { outputvars.Add(name); }
	CMString OutputVariable(unsigned short n) { if (n<outputvars.Count()) return outputvars[n]; return CMString(); }
	unsigned short OutputVariables() const { return outputvars.Count(); }
	int IsOutputVariable(const CMString& name) { return outputvars.Contains(name); }

	enum {EUnknownError=1,EReadingFile,EOpeningFile,ENoSimFileName,
			ESimFileNameInUse,ESimInProgress,EMissingVariables};
public:
	enum {IRP_VARIABLE=1,IRP_SCENARIO,IRP_CATEGORY,IRP_SCRIPT,SYNC_FILE_ADDED,
		SYNC_FILE_ATTACHED,SYNC_SCENARIO_ADDED,SYNC_SIMULATION_UPDATE,SYNC_PCT_COMPLETE};

	CMIrpApplication();
	~CMIrpApplication();

	static const wchar_t* GetVersionInfo() { return L"Irpsim Engine version 9.0"; }

	// Open an IRPSIM project, which is essentially a collection of files (vardef, scripts, etc.)
	void OpenProject(const CMString& name);

	CMOptions* Options() {return options;}
	void SetOptions(const CMOptions* op);

	CMString GetProjectFile() { return m_strProjectFile; }
	
	// Get the name (full path) of the file containing an IRPSIM object
	CMString GetObjectFileName(CMIrpObject* pObject); 

	unsigned short ScenariosCount() {return scenarios.Count();}
	CMScenario* Scenario(unsigned short n) {return (n<scenarios.Count()) ? scenarios[n] : 0;}
	CMScenario* UseScenario(const CMString& name);
	CMScenario* CurrentScenario() {return currentscenario;}

	unsigned short ScriptsCount() {return scripts.Count();}
	CMScript* Script(unsigned short n) {return (n<scripts.Count()) ? scripts[n] : 0;}
	CMScript* UseScript(const CMString& name);
	CMScript* CurrentScript() {return currentscript;}

	void SetOption(const CMString &name, const CMString& value) { options->SetOption(name, value); }
	CMString GetOption(const CMString& name) { return options->GetOption(name); }

   unsigned short Categories() {return CMCategory::CategoryCount();}
   CMCategory* Category(unsigned short n) {return CMCategory::GetCategory(n);}

	CMScript*	FindScript(const CMString& name);
	CMCategory* FindCategory(const CMString& name);
	CMScenario* FindScenario(const CMString& name);

	CMIrpObject* FindIrpObject(const CMString& name);
		
   CMVariableCollection* VariableCollection() {return variables;}

	//Call to create a new simulation

    CMSimulation* CreateSimulation(); //const wchar_t* pFileName = 0);
	BOOL RunSimulation(CMSimulation* pSim);
	BOOL PauseSimulation(CMSimulation* pSim,BOOL bAction);
	CMSimulation* RunningSimulation();

	int WriteOutcomes(const CMString& filename,CMSimulation* sim);
	int WriteSummary(const CMString& filename,CMSimulation* sim);

	void ResetApplication();

	int LoadedFilesCount() {return loadedfiles.Count();}
	CMString LoadedFile(unsigned short n) {if (n>=0 && n<loadedfiles.Count()) return loadedfiles[n]; return CMString();}

	void DeleteSimulation(CMSimulation* pSim,int save);
	unsigned SimulationsCount();
};
