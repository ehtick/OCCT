// Created on: 1997-02-12
// Created by: Alexander BRIVIN
// Copyright (c) 1997-1999 Matra Datavision
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

#ifndef _Vrml_Scale_HeaderFile
#define _Vrml_Scale_HeaderFile

#include <Standard.hxx>
#include <Standard_DefineAlloc.hxx>
#include <Standard_Handle.hxx>

#include <gp_Vec.hxx>
#include <Standard_OStream.hxx>

//! defines a Scale node of VRML specifying transform
//! properties.
//! This  node  defines  a  3D  scaling  about  the  origin.
//! By  default  :
//! myRotation  =  (1 1 1)
class Vrml_Scale
{
public:
  DEFINE_STANDARD_ALLOC

  Standard_EXPORT Vrml_Scale();

  Standard_EXPORT Vrml_Scale(const gp_Vec& aScaleFactor);

  Standard_EXPORT void SetScaleFactor(const gp_Vec& aScaleFactor);

  Standard_EXPORT gp_Vec ScaleFactor() const;

  Standard_EXPORT Standard_OStream& Print(Standard_OStream& anOStream) const;

protected:
private:
  gp_Vec myScaleFactor;
};

#endif // _Vrml_Scale_HeaderFile
