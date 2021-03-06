// vardesc.h : header file
//
// Copyright � 1998-2015 Casey McSpadden   
//		mailto:casey@crossriver.com
//		http://www.crossriver.com/
//
// ==========================================================================  
// DESCRIPTION:	
// ==========================================================================
// CMVariableDescriptor maintains the name, type and state of a variable.
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

#include "irp.h"
#include "cmdefs.h"
#include "cmstring.h"
#include <iostream>
using namespace std;

class _IRPCLASS CMVariableDescriptor
{
	CMString name;
    CMString type;
	long state;
public:
	enum {vdSum=0x0001,vdMoney=0x0002,vdNoUnits=0x0004,vdInteger=0x0008,
   		vdOutput=0x0010,vdGraph=0x0020};
	CMVariableDescriptor() : name() , type() , state(0) {}
	CMVariableDescriptor(const CMString& aName,const CMString& aType,int aState=0) :
		name(aName) , type(aType) , state(aState) {}
	CMVariableDescriptor& operator = (const CMVariableDescriptor& v)
		{name=v.name;type=v.type;state=v.state;return *this;}
	int operator == (const CMVariableDescriptor& v) const {return name==v.name;}
	int operator == (const CMString& aName) const {return name==aName;}
	operator const CMString& () {return name;}
	BOOL  GetState(int aState) {return (state&aState) ? TRUE : FALSE;}
	int   GetState() {return state;}
	const CMString& Name() {return name;}
	const CMString& Type() {return type;}
	void  Name(const CMString& aName) {name=aName;}
	void  Type(const CMString& aType) {type=aType;}
	void  SetState(int aState,BOOL action)
		{if (action==TRUE) state|=aState;else state&=~aState;}
	wostream& WriteBinary(wostream& s);
	wistream& ReadBinary(wistream& s);
	int BinarySize();
};

