// scenario.cpp : implementation file
//
// Copyright � 1998-2015 Casey McSpadden   
//		mailto:casey@crossriver.com
//		http://www.crossriver.com/
//
// ==========================================================================  
// DESCRIPTION:	
// ==========================================================================
// CMScenario implements IRPSIM's concept of a simulation "scenario"
// ==========================================================================
//
// ==========================================================================  
// HISTORY:	
// ==========================================================================
//			1.00	09 March 2015	- Initial re-write and release.
// ==========================================================================
//
/////////////////////////////////////////////////////////////////////////////
#include "scenario.h"
#include "variable.h"
#include "varcol.h"

#include "cmlib.h"
#include "token.h"

#include <iomanip>

const wchar_t* CMScenario::IsA() { return L"CMScenario"; }

CMScenario::~CMScenario()
{
	options.ResetAndDestroy();
}

void CMScenario::AddEntry(const CMString& nm,const CMString& value,int forceoption)
{
	CMString val = stripends(value);
	int len = nm.length();
	if (nm[0] == L'#' || forceoption) {
		CMOption* o = new CMOption(nm,val);
		options.Add(o);
		len = o->GetName().length();
	}
	else if (len) {
		varnames.Add(nm);
		int flg = 0;
		CMTokenizer next(value);
		CMString token;
		while (!(token=next(L" \t")).is_null()) {
			if (to_lower(token[0]) == L's') flg |= SaveFlag;
			else if (to_lower(token[0]) == L'w') flg |= WriteFlag;
		}
		flags.Add(flg);
	}
	if (len >= maxwidth) maxwidth = len+1;
}

wostream& CMScenario::write(wostream& s)
{
	unsigned i;
	s << setiosflags(ios::left) << L"#SCENARIO  " << GetName() << ENDL;
	for (i=0;i<options.Count();i++)
		s << L'#' << setw(maxwidth+2) << options[i]->GetName() << options[i]->GetValue() << ENDL;
	for (i=0;i<varnames.Count();i++) {
		s << setw(maxwidth+3) << varnames[i];
		if (flags[i] & SaveFlag) s << L"save  ";
		if (flags[i] & WriteFlag) s << L"write  ";
		s << ENDL;
	}
	return s << L"#END" << ENDL;
}

wistream& CMScenario::read(wistream& s)
{
	static wchar_t* header = L"#scenario";
	static wchar_t* footer = L"#end";

	options.ResetAndDestroy(1);
	varnames.Reset(1);
	flags.Reset(1);

	CMString line;

	int begin = 0;

	while (!s.eof()) {
		line.read_line(s);
		line = stripends(line);
		if (line.is_null() || line[0]==L'*')
			continue;
		CMTokenizer next(line);
		CMString first = next(L" \t");
		if (line(0,wcslen(header)) == header) {
			name = stripends(next(L"\r\n"));
			begin = 1;
			continue;
		}
		else if (line(0,wcslen(footer)) == footer) {
			if (!begin)		continue;
			else       		break;
		}
		else if (!begin)
			continue;

		AddEntry(first,next(L"\r\n"));
	}

	return s;
}

void CMScenario::Use(CMOptions& op)
{
	CMVariableIterator next;
	CMVariable* v;
   unsigned i;
	while ((v=next())!=0) {
		v->SetState(CMVariable::vsSelected|CMVariable::vsSaveOutcomes|CMVariable::vsOutput,FALSE);
		if (v->GetState(CMVariable::vsAggregate))
			v->SetState(CMVariable::vsSelected,TRUE);
	}
	for (i=0;i<varnames.Count();i++) {
		CMVariable* v = CMVariable::Find(varnames[i]);
		if (v) {
			v->SetState(CMVariable::vsSelected,TRUE);
			if (flags[i] & SaveFlag) v->SetState(CMVariable::vsSaveOutcomes,TRUE);
			if (flags[i] & WriteFlag) v->SetState(CMVariable::vsOutput,TRUE);
		}
	}
	for (i=0;i<options.Count();i++)
		op.SetOption(options[i]->GetName(),options[i]->GetValue());
}












