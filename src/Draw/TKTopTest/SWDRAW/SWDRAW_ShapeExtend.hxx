// Created on: 1999-03-09
// Created by: data exchange team
// Copyright (c) 1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#ifndef _SWDRAW_ShapeExtend_HeaderFile
#define _SWDRAW_ShapeExtend_HeaderFile

#include <Standard.hxx>
#include <Standard_DefineAlloc.hxx>

#include <Draw_Interpretor.hxx>

//! Contains commands to activate package ShapeExtend
//! List of DRAW commands and corresponding functionalities:
//! sortcompound - ShapeExtend_Explorer::SortedCompound
class SWDRAW_ShapeExtend
{
public:
  DEFINE_STANDARD_ALLOC

  //! Loads commands defined in ShapeExtend
  Standard_EXPORT static void InitCommands(Draw_Interpretor& theCommands);

protected:
private:
};

#endif // _SWDRAW_ShapeExtend_HeaderFile
