// Created on: 1999-11-26
// Created by: Andrey BETENEV
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

// Generator:	ExpToCas (EXPRESS -> CASCADE/XSTEP Translator) V1.0

#include <Standard_Transient.hxx>
#include <StepAP203_ClassifiedItem.hxx>
#include <StepBasic_ProductDefinitionFormation.hxx>
#include <StepRepr_AssemblyComponentUsage.hxx>

//=================================================================================================

StepAP203_ClassifiedItem::StepAP203_ClassifiedItem() {}

//=================================================================================================

Standard_Integer StepAP203_ClassifiedItem::CaseNum(const Handle(Standard_Transient)& ent) const
{
  if (ent.IsNull())
    return 0;
  if (ent->IsKind(STANDARD_TYPE(StepBasic_ProductDefinitionFormation)))
    return 1;
  if (ent->IsKind(STANDARD_TYPE(StepRepr_AssemblyComponentUsage)))
    return 2;
  return 0;
}

//=================================================================================================

Handle(StepBasic_ProductDefinitionFormation) StepAP203_ClassifiedItem::ProductDefinitionFormation()
  const
{
  return Handle(StepBasic_ProductDefinitionFormation)::DownCast(Value());
}

//=================================================================================================

Handle(StepRepr_AssemblyComponentUsage) StepAP203_ClassifiedItem::AssemblyComponentUsage() const
{
  return Handle(StepRepr_AssemblyComponentUsage)::DownCast(Value());
}
