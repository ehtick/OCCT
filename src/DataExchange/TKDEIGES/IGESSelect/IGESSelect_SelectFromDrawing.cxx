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

#include <IGESData_ViewKindEntity.hxx>
#include <IGESSelect_SelectFromDrawing.hxx>
#include <Interface_EntityIterator.hxx>
#include <Interface_Graph.hxx>
#include <Interface_Macros.hxx>
#include <Standard_Type.hxx>
#include <TCollection_AsciiString.hxx>

IMPLEMENT_STANDARD_RTTIEXT(IGESSelect_SelectFromDrawing, IFSelect_SelectDeduct)

#define PourDrawing 404

IGESSelect_SelectFromDrawing::IGESSelect_SelectFromDrawing() {}

Interface_EntityIterator IGESSelect_SelectFromDrawing::RootResult(const Interface_Graph& G) const
{
  Interface_EntityIterator list, views;
  Interface_EntityIterator draws = InputResult(G);
  if (draws.NbEntities() == 0)
    return list;
  Standard_Integer nb = G.Size();
  Standard_Integer i; // svv Jan11 2000 : porting on DEC

  //  Pour chaque Drawing : prendre d une part l integralite de son contenu,
  //  (c-a-d avec le "Frame"), d autre part les entites attachees a ses vues
  for (draws.Start(); draws.More(); draws.Next())
  {
    DeclareAndCast(IGESData_IGESEntity, igesent, draws.Value());
    if (igesent.IsNull())
      continue;
    if (igesent->TypeNumber() != PourDrawing)
      continue;
    list.GetOneItem(igesent);
    Interface_EntityIterator someviews = G.Shareds(draws.Value());
    list.AddList(someviews.Content());
  }
  for (i = 1; i <= nb; i++)
  {
    //    if (!G.IsPresent(i)) continue;
    DeclareAndCast(IGESData_IGESEntity, igesent, G.Entity(i));
    if (igesent.IsNull())
      continue;
    Standard_Integer nv = G.EntityNumber(igesent->View());
    if (nv > 0 && nv <= nb)
      list.GetOneItem(igesent);
  }
  return list;
}

TCollection_AsciiString IGESSelect_SelectFromDrawing::Label() const
{
  return TCollection_AsciiString("Entities attached to a Drawing");
}
