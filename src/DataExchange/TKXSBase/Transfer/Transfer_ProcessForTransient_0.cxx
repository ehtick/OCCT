// Created on: 1992-02-03
// Created by: Christian CAILLET
// Copyright (c) 1992-1999 Matra Datavision
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

#include <Transfer_ProcessForTransient.hxx>

#include <Interface_Check.hxx>
#include <Interface_CheckIterator.hxx>
#include <Interface_InterfaceError.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Message_Msg.hxx>
#include <Message_Msg.hxx>
#include <Message_Msg.hxx>
#include <Message_ProgressScope.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <Standard_Transient.hxx>
#include <Standard_Type.hxx>
#include <TCollection_HAsciiString.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_HArray1OfInteger.hxx>
#include <TColStd_MapIteratorOfMapOfInteger.hxx>
#include <Transfer_ActorOfProcessForTransient.hxx>
#include <Transfer_ActorOfProcessForTransient.hxx>
#include <Transfer_Binder.hxx>
#include <Transfer_FindHasher.hxx>
#include <Transfer_IteratorOfProcessForTransient.hxx>
#include <Transfer_IteratorOfProcessForTransient.hxx>
#include <Transfer_MultipleBinder.hxx>
#include <Transfer_SimpleBinderOfTransient.hxx>
#include <Transfer_StatusResult.hxx>
#include <Transfer_TransferDeadLoop.hxx>
#include <Transfer_TransferFailure.hxx>
#include <Transfer_TransferMapOfProcessForTransient.hxx>
#include <Transfer_TransferMapOfProcessForTransient.hxx>
#include <Transfer_VoidBinder.hxx>

//=================================================================================================

Transfer_ProcessForTransient::Transfer_ProcessForTransient(const Standard_Integer nb)
    : themap(nb)
{
  theerrh      = Standard_True;
  therootm     = Standard_False;
  thelevel     = 0;
  therootl     = 0;
  themessenger = Message::DefaultMessenger();
  thetrace     = 0;
  theindex     = 0;
}

//=================================================================================================

Transfer_ProcessForTransient::Transfer_ProcessForTransient(
  const Handle(Message_Messenger)& messenger,
  const Standard_Integer           nb)
    : themap(nb)
{
  theerrh  = Standard_True;
  therootm = Standard_False;
  thelevel = 0;
  therootl = 0;
  SetMessenger(messenger);
  thetrace = 1;
  theindex = 0;
}

//=================================================================================================

void Transfer_ProcessForTransient::Clear()
{
  thelevel = 0;
  therootl = 0;
  theroots.Clear();
  themap.Clear();
  theindex = 0;
  thelastobj.Nullify();
  thelastbnd.Nullify();
}

//=================================================================================================

void Transfer_ProcessForTransient::Clean()
{
  Standard_Integer i, nb  = NbMapped();
  Standard_Integer j, unb = 0;
  for (i = 1; i <= nb; i++)
  {
    if (themap(i).IsNull())
      unb++;
  }
  if (unb == 0)
    return;

  // Redo the map -> offsets
  TColStd_Array1OfInteger unbs(1, nb);
  unbs.Init(0);
  Transfer_TransferMapOfProcessForTransient newmap(nb * 2);
  for (i = 1; i <= nb; i++)
  {
    Handle(Standard_Transient) ent = Mapped(i);
    Handle(Transfer_Binder)    bnd = MapItem(i);
    if (bnd.IsNull())
      continue;
    j = newmap.Add(ent, bnd);
    unbs.SetValue(i, j);
  }
  themap.Assign(newmap);

  // Update Root List
  TColStd_IndexedMapOfInteger aNewRoots;
  for (i = 1; i <= theroots.Extent(); i++)
  {
    j                  = theroots.FindKey(i);
    Standard_Integer k = unbs.Value(j);
    if (k)
      aNewRoots.Add(k);
  }
  theroots.Clear();
  theroots = aNewRoots;

  // The rest: cleaning
  thelastobj.Nullify();
  thelastbnd.Nullify();
  theindex = 0;
}

//=================================================================================================

void Transfer_ProcessForTransient::Resize(const Standard_Integer nb)
{
  if (nb > themap.NbBuckets())
    themap.ReSize(nb);
}

//=================================================================================================

void Transfer_ProcessForTransient::SetActor(
  const Handle(Transfer_ActorOfProcessForTransient)& actor)
{
  if (theactor == actor)
    return;
  if (theactor.IsNull())
    theactor = actor;
  else if (actor.IsNull())
    theactor = actor;
  else if (theactor->IsLast())
  {
    actor->SetNext(theactor);
    theactor = actor;
  }
  else
    theactor->SetNext(actor);
}

//=================================================================================================

Handle(Transfer_ActorOfProcessForTransient) Transfer_ProcessForTransient::Actor() const
{
  return theactor;
}

//=================================================================================================

Handle(Transfer_Binder) Transfer_ProcessForTransient::Find(
  const Handle(Standard_Transient)& start) const
{
  if (thelastobj == start)
  {
    if (theindex > 0)
      return thelastbnd;
  }
  Standard_Integer index = themap.FindIndex(start);
  if (index > 0)
  {
    const Handle(Transfer_Binder)& binder = themap.FindFromIndex(index);
    return binder;
  }
  return Handle(Transfer_Binder)();
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::IsBound(
  const Handle(Standard_Transient)& start) const
{
  Handle(Transfer_Binder) binder = Find(start);
  if (binder.IsNull())
    return Standard_False;
  return binder->HasResult();
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::IsAlreadyUsed(
  const Handle(Standard_Transient)& start) const
{
  Handle(Transfer_Binder) binder = Find(start);
  if (binder.IsNull())
  {
    StartTrace(binder, start, thelevel, 4);
    throw Transfer_TransferFailure(
      "TransferProcess : IsAlreadyUsed, transfer not done cannot be used...");
  }
  return (binder->Status() == Transfer_StatusUsed);
}

//=================================================================================================

Handle(Transfer_Binder) Transfer_ProcessForTransient::FindAndMask(
  const Handle(Standard_Transient)& start)
{
  if (thelastobj == start)
  {
    if (theindex > 0)
      return thelastbnd;
  }
  thelastobj = start;
  theindex   = themap.FindIndex(start);
  if (theindex > 0)
    thelastbnd = themap.FindFromIndex(theindex);
  else
    thelastbnd.Nullify();
  return thelastbnd;
}

//=================================================================================================

void Transfer_ProcessForTransient::Bind(const Handle(Standard_Transient)& start,
                                        const Handle(Transfer_Binder)&    binder)
{
  if (binder.IsNull())
    return;
  Handle(Transfer_Binder) former = FindAndMask(start);
  if (!former.IsNull())
  {
    // We admit VoidBinder: then we resume our Check
    if (former->DynamicType() == STANDARD_TYPE(Transfer_VoidBinder))
    {
      binder->Merge(former);
      themap(theindex) = binder;
    }
    else if (former->Status() == Transfer_StatusUsed)
    {
      StartTrace(former, start, thelevel, 4);
      throw Transfer_TransferFailure("TransferProcess : Bind, already Bound");
    }
    else
    {
      if (thetrace > 2)
        StartTrace(former, start, thelevel, 5);
      binder->CCheck()->GetMessages(former->Check());
    }
  }
  if (theindex == 0 || thelastbnd.IsNull())
  {
    if (theindex == 0)
      theindex = themap.Add(start, binder);
    else
      themap(theindex) = binder;
    thelastbnd = binder;
  }
  else
  {
    thelastbnd       = binder;
    themap(theindex) = binder;
  }
}

//=================================================================================================

void Transfer_ProcessForTransient::Rebind(const Handle(Standard_Transient)& start,
                                          const Handle(Transfer_Binder)&    binder)
{
  Bind(start, binder);
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::Unbind(const Handle(Standard_Transient)& start)
{
  Handle(Transfer_Binder) former = FindAndMask(start);
  if (theindex == 0)
    return Standard_False;
  if (former.IsNull())
    return Standard_False;
  if (former->DynamicType() == STANDARD_TYPE(Transfer_VoidBinder))
    return Standard_True;
  themap(theindex) = thelastbnd;
  if (theroots.Contains(theindex))
  {
    TColStd_IndexedMapOfInteger aNewRoots;
    for (Standard_Integer i = 1; i <= theroots.Extent(); i++)
      if (theindex != theroots.FindKey(i))
        aNewRoots.Add(theroots.FindKey(i));

    theroots.Clear();
    theroots = aNewRoots;
  }

  thelastobj.Nullify();
  thelastbnd.Nullify();
  theindex = 0;
  return Standard_True;
}

//=================================================================================================

Handle(Transfer_Binder) Transfer_ProcessForTransient::FindElseBind(
  const Handle(Standard_Transient)& start)
{
  Handle(Transfer_Binder) binder = FindAndMask(start);
  if (!binder.IsNull())
    return binder;
  binder = new Transfer_VoidBinder;
  Bind(start, binder);
  return binder;
}

//=================================================================================================

void Transfer_ProcessForTransient::SetMessenger(const Handle(Message_Messenger)& messenger)
{
  if (messenger.IsNull())
    themessenger = Message::DefaultMessenger();
  else
    themessenger = messenger;
}

//=================================================================================================

Handle(Message_Messenger) Transfer_ProcessForTransient::Messenger() const
{
  return themessenger;
}

//=================================================================================================

void Transfer_ProcessForTransient::SetTraceLevel(const Standard_Integer tracelev)
{
  thetrace = tracelev;
}

//=================================================================================================

Standard_Integer Transfer_ProcessForTransient::TraceLevel() const
{
  return thetrace;
}

//=================================================================================================

void Transfer_ProcessForTransient::SendFail(const Handle(Standard_Transient)& start,
                                            const Message_Msg&                amsg)
{
  AddFail(start, amsg);
}

//=================================================================================================

void Transfer_ProcessForTransient::SendWarning(const Handle(Standard_Transient)& start,
                                               const Message_Msg&                amsg)
{
  AddWarning(start, amsg);
}

//=================================================================================================

void Transfer_ProcessForTransient::SendMsg(const Handle(Standard_Transient)& start,
                                           const Message_Msg&                amsg)
{
  Handle(Transfer_Binder) binder = FindAndMask(start);
  if (binder.IsNull())
  {
    binder = new Transfer_VoidBinder;
    Bind(start, binder);
  }
  // Feeds the trace: Causing rule (user messages)
  if (thetrace > 0)
  {
    StartTrace(binder, start, thelevel, 6);
    Message_Messenger::StreamBuffer aSender = themessenger->SendInfo();
    aSender << amsg.Value();
    if (amsg.IsEdited() && thetrace > 2)
      aSender << " [from: " << amsg.Original() << "]";
    aSender << std::endl;
  }
}

//=================================================================================================

void Transfer_ProcessForTransient::AddFail(const Handle(Standard_Transient)& start,
                                           const Standard_CString            mess,
                                           const Standard_CString            orig)
{
  Handle(Transfer_Binder) binder = FindAndMask(start);
  if (binder.IsNull())
  {
    binder = new Transfer_VoidBinder;
    Bind(start, binder);
  }
  binder->AddFail(mess, orig);
  if (thetrace > 0)
  {
    StartTrace(binder, start, thelevel, 1);
    Message_Messenger::StreamBuffer aSender = themessenger->SendFail();
    aSender << "    --> Fail : " << mess;
    if (orig[0] != '\0' && thetrace > 2)
      aSender << " [from: " << orig << "]";
    aSender << std::endl;
  }
}

//=================================================================================================

void Transfer_ProcessForTransient::AddError(const Handle(Standard_Transient)& start,
                                            const Standard_CString            mess,
                                            const Standard_CString            orig)
{
  AddFail(start, mess, orig);
}

//=================================================================================================

void Transfer_ProcessForTransient::AddFail(const Handle(Standard_Transient)& start,
                                           const Message_Msg&                amsg)
{
  if (amsg.IsEdited())
    AddFail(start,
            TCollection_AsciiString(amsg.Value()).ToCString(),
            TCollection_AsciiString(amsg.Original()).ToCString());
  else
    AddFail(start, TCollection_AsciiString(amsg.Value()).ToCString());
}

//=================================================================================================

void Transfer_ProcessForTransient::AddWarning(const Handle(Standard_Transient)& start,
                                              const Standard_CString            mess,
                                              const Standard_CString            orig)
{
  Handle(Transfer_Binder) binder = FindAndMask(start);
  if (binder.IsNull())
  {
    binder = new Transfer_VoidBinder;
    Bind(start, binder);
  }
  binder->AddWarning(mess, orig);
  if (thetrace > 1)
  {
    StartTrace(binder, start, thelevel, 2);
    Message_Messenger::StreamBuffer aSender = themessenger->SendWarning();
    aSender << "    --> Warning : " << mess;
    if (orig[0] != '\0' && thetrace > 2)
      aSender << " [from: " << orig << "]";
    aSender << std::endl;
  }
}

//=================================================================================================

void Transfer_ProcessForTransient::AddWarning(const Handle(Standard_Transient)& start,
                                              const Message_Msg&                amsg)
{
  if (amsg.IsEdited())
    AddWarning(start,
               TCollection_AsciiString(amsg.Value()).ToCString(),
               TCollection_AsciiString(amsg.Original()).ToCString());
  else
    AddWarning(start, TCollection_AsciiString(amsg.Value()).ToCString());
}

//=================================================================================================

void Transfer_ProcessForTransient::Mend(const Handle(Standard_Transient)& start,
                                        const Standard_CString            pref)
{
  Handle(Transfer_Binder) binder = FindAndMask(start);
  if (binder.IsNull())
    return;
  Handle(Interface_Check) ach = binder->CCheck();
  ach->Mend(pref);
}

//=================================================================================================

Handle(Interface_Check) Transfer_ProcessForTransient::Check(
  const Handle(Standard_Transient)& start) const
{
  const Handle(Transfer_Binder)& binder = Find(start);
  if (binder.IsNull())
  {
    Handle(Interface_Check) check;
    return check;
  }
  return binder->Check();
}

//=================================================================================================

void Transfer_ProcessForTransient::BindTransient(const Handle(Standard_Transient)& start,
                                                 const Handle(Standard_Transient)& res)
{
  if (res.IsNull())
    return;
  Handle(Transfer_Binder)                  former = Find(start);
  Handle(Transfer_SimpleBinderOfTransient) binder =
    Handle(Transfer_SimpleBinderOfTransient)::DownCast(former);
  if (!binder.IsNull())
  {
    if (binder->Status() == Transfer_StatusVoid)
    {
      binder->SetResult(res);
      return;
    }
  }
  binder = new Transfer_SimpleBinderOfTransient;
  binder->SetResult(res);
  if (former.IsNull())
    Bind(start, binder);
  else
    Rebind(start, binder);
}

//=================================================================================================

const Handle(Standard_Transient)& Transfer_ProcessForTransient::FindTransient(
  const Handle(Standard_Transient)& start) const
{
  static const Handle(Standard_Transient)  aDummy;
  Handle(Transfer_SimpleBinderOfTransient) binder =
    Handle(Transfer_SimpleBinderOfTransient)::DownCast(Find(start));
  if (binder.IsNull())
    return aDummy;
  if (!binder->HasResult())
    return aDummy;
  return binder->Result();
}

//=================================================================================================

void Transfer_ProcessForTransient::BindMultiple(const Handle(Standard_Transient)& start)
{
  Handle(Transfer_Binder) binder = FindAndMask(start);
  if (!binder.IsNull())
  {
    if (!binder->IsKind(STANDARD_TYPE(Transfer_MultipleBinder)))
    {
      StartTrace(thelastbnd, start, thelevel, 4);
      throw Transfer_TransferFailure("TransferProcess : BindMultiple");
    }
  }
  else
    Bind(start, new Transfer_MultipleBinder);
}

//=================================================================================================

void Transfer_ProcessForTransient::AddMultiple(const Handle(Standard_Transient)& start,
                                               const Handle(Standard_Transient)& res)
{
  Handle(Transfer_Binder)         binder = FindAndMask(start);
  Handle(Transfer_MultipleBinder) multr  = Handle(Transfer_MultipleBinder)::DownCast(binder);
  if (multr.IsNull())
  {
    StartTrace(binder, start, thelevel, 4);
    if (binder.IsNull())
      throw Transfer_TransferFailure("TransferProcess : AddMultiple, nothing bound");
    else
      throw Transfer_TransferFailure("TransferProcess : AddMultiple, Binder not a MultipleBinder");
  }
  multr->AddResult(res);
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::FindTypedTransient(
  const Handle(Standard_Transient)& start,
  const Handle(Standard_Type)&      atype,
  Handle(Standard_Transient)&       val) const
{
  return GetTypedTransient(Find(start), atype, val);
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::GetTypedTransient(
  const Handle(Transfer_Binder)& binder,
  const Handle(Standard_Type)&   atype,
  Handle(Standard_Transient)&    val) const
{
  return Transfer_SimpleBinderOfTransient::GetTypedResult(binder, atype, val);
}

//=================================================================================================

Standard_Integer Transfer_ProcessForTransient::NbMapped() const
{
  return themap.Extent();
}

//=================================================================================================

const Handle(Standard_Transient)& Transfer_ProcessForTransient::Mapped(
  const Standard_Integer num) const
{
  return themap.FindKey(num);
}

//=================================================================================================

Standard_Integer Transfer_ProcessForTransient::MapIndex(
  const Handle(Standard_Transient)& start) const
{
  return themap.FindIndex(start);
}

//=================================================================================================

Handle(Transfer_Binder) Transfer_ProcessForTransient::MapItem(const Standard_Integer num) const
{
  Handle(Transfer_Binder) binder = themap.FindFromIndex(num);
  return binder;
}

//=================================================================================================

void Transfer_ProcessForTransient::SetRoot(const Handle(Standard_Transient)& start)
{
  Standard_Integer index = MapIndex(start);
  if (index == 0)
  {
    return;
  }

  theroots.Add(index);
  if (thetrace > 2)
    StartTrace(MapItem(index), start, thelevel, 3);
}

//=================================================================================================

void Transfer_ProcessForTransient::SetRootManagement(const Standard_Boolean stat)
{
  therootm = stat;
}

//=================================================================================================

Standard_Integer Transfer_ProcessForTransient::NbRoots() const
{
  return theroots.Extent();
}

//=================================================================================================

const Handle(Standard_Transient)& Transfer_ProcessForTransient::Root(
  const Standard_Integer num) const
{
  Standard_Integer ind = 0;
  if (num > 0 && num <= theroots.Extent())
    ind = theroots.FindKey(num);
  return themap.FindKey(ind);
}

//=================================================================================================

Handle(Transfer_Binder) Transfer_ProcessForTransient::RootItem(const Standard_Integer num) const
{
  Standard_Integer ind = 0;
  if (num > 0 && num <= theroots.Extent())
    ind = theroots.FindKey(num);
  return themap.FindFromIndex(ind);
}

//=================================================================================================

Standard_Integer Transfer_ProcessForTransient::RootIndex(
  const Handle(Standard_Transient)& start) const
{
  Standard_Integer index = MapIndex(start);
  if (index == 0)
    return 0;
  return theroots.FindIndex(index);
}

//=================================================================================================

Standard_Integer Transfer_ProcessForTransient::NestingLevel() const
{
  return thelevel;
}

//=================================================================================================

void Transfer_ProcessForTransient::ResetNestingLevel()
{
  thelevel = 0;
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::Recognize(
  const Handle(Standard_Transient)& start) const
{
  Handle(Transfer_ActorOfProcessForTransient) actor = theactor;
  // We scan the Next until we have a Result
  while (!actor.IsNull())
  {
    if (actor->Recognize(start))
      return Standard_True;
    actor = actor->Next();
  }
  return Standard_False;
}

//=================================================================================================

Handle(Transfer_Binder) Transfer_ProcessForTransient::Transferring(
  const Handle(Standard_Transient)& start,
  const Message_ProgressRange&      theProgress)
{
  Handle(Transfer_Binder) former = FindAndMask(start);
  // We consider that this new Transfer request therefore corresponds to a
  // additional use: note "AlreadyUsed", therefore non-modifiable result
  if (!former.IsNull())
  {
    if (former->HasResult())
    {
      former->SetAlreadyUsed();
      return former;
    }

    Message_Messenger::StreamBuffer aSender = themessenger->SendInfo();
    Transfer_StatusExec             statex  = former->StatusExec();
    switch (statex)
    {
      case Transfer_StatusInitial:
        break;
      case Transfer_StatusDone:
        aSender << " .. and Transfer done" << std::endl;
        return former;
      case Transfer_StatusRun:
        former->SetStatusExec(Transfer_StatusLoop);
        return former;
      case Transfer_StatusError:
        if (thetrace)
        {
          aSender << "                  *** Transfer in Error Status  :" << std::endl;
          StartTrace(former, start, thelevel, 0);
        }
        else
          StartTrace(former, start, thelevel, 4);
        throw Transfer_TransferFailure("TransferProcess : Transfer in Error Status");
      case Transfer_StatusLoop:
        if (thetrace)
        {
          aSender << "                  *** Transfer  Head of Dead Loop  :" << std::endl;
          StartTrace(former, start, thelevel, 0);
        }
        else
          StartTrace(former, start, thelevel, 4);
        throw Transfer_TransferDeadLoop("TransferProcess : Transfer at Head of a Dead Loop");
    }
    former->SetStatusExec(Transfer_StatusRun);
  }

  Handle(Transfer_Binder) binder;
  Standard_Boolean        newbind = Standard_False;
  if (theerrh)
  {
    Message_Messenger::StreamBuffer aSender = themessenger->SendInfo();

    // Transfer under protection for exceptions (for notification in fact)
    Standard_Integer oldlev = thelevel;
    try
    {
      OCC_CATCH_SIGNALS
      binder = TransferProduct(start, theProgress);
    }

    // Exceptions to catch up on: they are not all the same
    catch (Transfer_TransferDeadLoop const&)
    {
      if (binder.IsNull())
      {
        aSender << "                  *** Dead Loop with no Result" << std::endl;
        if (thetrace)
          StartTrace(binder, start, thelevel - 1, 0);
        binder = new Transfer_VoidBinder;
        Bind(start, binder);
        newbind = Standard_True;
      }
      else if (binder->StatusExec() == Transfer_StatusLoop)
      {
        if (thetrace)
        {
          aSender << "                  *** Dead Loop : Finding head of Loop :" << std::endl;
          StartTrace(binder, start, thelevel - 1, 0);
        }
        else
          StartTrace(binder, start, thelevel - 1, 4);
        throw Transfer_TransferFailure("TransferProcess : Head of Dead Loop");
        // In other words, we change the exception (we exit the loop)
      }
      else
      {
        if (thetrace)
        {
          aSender << "                  *** Dead Loop : Actor in Loop :" << std::endl;
          StartTrace(binder, start, thelevel - 1, 0);
        }
      }
      binder->AddFail("Transfer in dead Loop");
      thelevel = oldlev;
    }
    catch (Standard_Failure const& anException)
    {
      if (binder.IsNull())
      {
        aSender << "                  *** Exception Raised with no Result" << std::endl;
        binder = new Transfer_VoidBinder;
        Bind(start, binder);
        newbind = Standard_True;
      }
      binder->AddFail("Transfer stopped by exception raising");
      if (thetrace)
      {
        aSender << "    *** Raised : " << anException.GetMessageString() << std::endl;
        StartTrace(binder, start, thelevel - 1, 4);
      }
      thelevel = oldlev;
    }
  }

  // Unprotected transfer (thus, dbx by hand in case of crash by Raise)
  else
    binder = TransferProduct(start, theProgress);

  if (theProgress.UserBreak())
    return Handle(Transfer_Binder)();

  if (!newbind && !binder.IsNull())
  {
    if (former.IsNull())
    {
      if (!IsBound(start))
        Bind(start, binder);
      else
      {
        Rebind(start, binder);
      }
    }
    else
      Rebind(start, binder);
  }
  else
  {
    if (!former.IsNull())
      former->SetStatusExec(Transfer_StatusDone); //+
    return Handle(Transfer_Binder)();             // Binder Null ... que faire d autre ?
  }

  if (therootl >= thelevel)
  {
    therootl = 0;
    if (therootm && binder->Status() != Transfer_StatusVoid)
    {
      SetRoot(start);
    }
  }
  return thelastbnd;
}

//=================================================================================================

Handle(Transfer_Binder) Transfer_ProcessForTransient::TransferProduct(
  const Handle(Standard_Transient)& start,
  const Message_ProgressRange&      theProgress)
{
  thelevel++;
  Handle(Transfer_Binder)                     binder;
  Handle(Transfer_ActorOfProcessForTransient) actor = theactor;

  // We scan the Next until we have a Result
  Message_ProgressScope aScope(theProgress, NULL, 1, true);
  while (!actor.IsNull())
  {
    if (actor->Recognize(start))
    {
      binder = actor->Transferring(start, this, aScope.Next());
    }
    else
    {
      binder.Nullify();
    }
    if (!binder.IsNull())
    {
      break;
    }
    actor = actor->Next();
  }
  if (aScope.UserBreak())
    return Handle(Transfer_Binder)();

  if (binder.IsNull())
  {
    if (thelevel > 0)
      thelevel--;
    return binder;
  }
  if (therootl == 0 && binder->StatusExec() == Transfer_StatusDone)
    therootl = thelevel - 1;

  if (thelevel > 0)
    thelevel--;
  return binder;
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::Transfer(const Handle(Standard_Transient)& start,
                                                        const Message_ProgressRange& theProgress)
{
  Handle(Transfer_Binder) binder = Transferring(start, theProgress);
  return (!binder.IsNull());
}

//=================================================================================================

void Transfer_ProcessForTransient::SetErrorHandle(const Standard_Boolean err)
{
  theerrh = err;
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::ErrorHandle() const
{
  return theerrh;
}

//=================================================================================================

void Transfer_ProcessForTransient::StartTrace(const Handle(Transfer_Binder)&    binder,
                                              const Handle(Standard_Transient)& start,
                                              const Standard_Integer            level,
                                              const Standard_Integer            mode) const
{
  Message_Messenger::StreamBuffer aSender = themessenger->SendInfo();
  // ###  Fail (Roots:50)  --  Start start->DynamicType()
  // ###  Fail (Roots:50)  --  Start id:#label.. Type:start->DynamicType()
  if (thetrace > 3)
  { // Internal to be switch when searching bug (trace >= 4)
    if (mode == 1)
      aSender << "  ###  Fail";
    if (mode == 2)
      aSender << "  ###  Warning";
    if (mode == 3)
      aSender << "  ###  New Root n0 " << theroots.Extent();
    if (mode == 4)
      aSender << "  ###  Exception";
    if (mode == 5)
      aSender << "  ###  Substitution";
    if (mode == 6)
      aSender << "  ###  Information";
    if (level > 1)
      aSender << " (nested)";
    if (mode >= 0 && mode != 3)
      aSender << " at " << theroots.Extent() << " Roots";
  }
  if (!start.IsNull())
    PrintTrace(start, aSender);

  if (!binder.IsNull())
  {
    Handle(Transfer_Binder) bnd    = binder;
    Standard_Boolean        hasres = Standard_False;
    while (!bnd.IsNull())
    {
      if (bnd->Status() != Transfer_StatusVoid)
      {
        if (!hasres)
          aSender << "\n  ---  Result Type : ";
        else
          aSender << " , ";
        aSender << bnd->ResultTypeName();
        hasres = Standard_True;
      }
      bnd = bnd->NextResult();
    }
    if (!hasres && mode > 2)
    {
      aSender << "\n  ---  No Result recorded";
    }
  }
  aSender << std::endl;
}

//=================================================================================================

void Transfer_ProcessForTransient::PrintTrace(const Handle(Standard_Transient)& start,
                                              Standard_OStream&                 S) const
{
  if (!start.IsNull())
    S << " Type:" << start->DynamicType()->Name();
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::IsLooping(const Standard_Integer alevel) const
{
  return alevel > NbMapped();
}

//=================================================================================================

Transfer_IteratorOfProcessForTransient Transfer_ProcessForTransient::RootResult(
  const Standard_Boolean withstart) const
{
  Transfer_IteratorOfProcessForTransient iter(withstart);
  Standard_Integer                       max = theroots.Extent();
  for (Standard_Integer j = 1; j <= max; j++)
  {
    Standard_Integer        i      = theroots.FindKey(j);
    Handle(Transfer_Binder) binder = MapItem(i);
    if (binder.IsNull())
      continue;
    if (withstart)
      iter.Add(binder, Mapped(i));
    else
      iter.Add(binder);
  }
  return iter;
}

//=================================================================================================

Transfer_IteratorOfProcessForTransient Transfer_ProcessForTransient::CompleteResult(
  const Standard_Boolean withstart) const
{
  Transfer_IteratorOfProcessForTransient iter(withstart);
  Standard_Integer                       max = NbMapped();
  for (Standard_Integer i = 1; i <= max; i++)
  {
    Handle(Transfer_Binder) binder = MapItem(i);
    if (binder.IsNull())
      continue;
    if (withstart)
      iter.Add(binder, Mapped(i));
    else
      iter.Add(binder);
  }
  return iter;
}

//=================================================================================================

Transfer_IteratorOfProcessForTransient Transfer_ProcessForTransient::AbnormalResult() const
{
  Transfer_IteratorOfProcessForTransient iter(Standard_True);
  Standard_Integer                       max = NbMapped();
  for (Standard_Integer i = 1; i <= max; i++)
  {
    Handle(Transfer_Binder) binder = MapItem(i);
    if (binder.IsNull())
      continue;
    Transfer_StatusExec statex = binder->StatusExec();
    if (statex != Transfer_StatusInitial && statex != Transfer_StatusDone)
      iter.Add(binder, Mapped(i));
  }
  return iter;
}

//=================================================================================================

Interface_CheckIterator Transfer_ProcessForTransient::CheckList(
  const Standard_Boolean erronly) const
{
  Interface_CheckIterator list;
  Standard_Integer        num, max = NbMapped();
  for (Standard_Integer i = 1; i <= max; i++)
  {
    Handle(Transfer_Binder) binder = MapItem(i);
    if (binder.IsNull())
      continue;
    Transfer_StatusExec     statex = binder->StatusExec();
    Handle(Interface_Check) check  = binder->Check();
    if (statex != Transfer_StatusInitial && statex != Transfer_StatusDone && !check->HasFailed())
      check->AddFail("Transfer in Abnormal Status (!= Initial or Done)");
    if (!check->HasFailed() && (erronly || check->NbWarnings() == 0))
      continue;
    const Handle(Standard_Transient)& ent = Mapped(i);
    num                                   = CheckNum(ent);
    if (num == 0)
      num = i;
    check->SetEntity(ent);
    list.Add(check, num);
  }
  return list;
}

//=================================================================================================

Transfer_IteratorOfProcessForTransient Transfer_ProcessForTransient::ResultOne(
  const Handle(Standard_Transient)& start,
  const Standard_Integer            level,
  const Standard_Boolean            withstart) const
{
  Transfer_IteratorOfProcessForTransient iter(withstart);
  Standard_Integer                       max = NbMapped();
  Standard_Integer                       ind = MapIndex(start);
  if (ind == 0)
    return iter;
  Standard_Integer                 i1  = (level == 0 ? ind : 1);
  Standard_Integer                 i2  = (level == 0 ? ind : max);
  Handle(TColStd_HArray1OfInteger) map = new TColStd_HArray1OfInteger(i1, i2, 0);

  for (Standard_Integer i = i1; i <= i2; i++)
  {
    ind = map->Value(i);
    if (ind == 0)
      continue;
    Handle(Transfer_Binder) binder = MapItem(i);
    if (binder.IsNull())
      continue;
    if (withstart)
      iter.Add(binder, Mapped(ind));
    else
      iter.Add(binder);
  }
  return iter;
}

//=================================================================================================

Interface_CheckIterator Transfer_ProcessForTransient::CheckListOne(
  const Handle(Standard_Transient)& start,
  const Standard_Integer            level,
  const Standard_Boolean            erronly) const
{
  Interface_CheckIterator list;
  Standard_Integer        max = NbMapped();
  Standard_Integer        num, ind = MapIndex(start);
  if (ind == 0)
    return list;
  Standard_Integer                 i1  = (level == 0 ? ind : 1);
  Standard_Integer                 i2  = (level == 0 ? ind : max);
  Handle(TColStd_HArray1OfInteger) map = new TColStd_HArray1OfInteger(i1, i2, 0);

  for (Standard_Integer i = i1; i <= i2; i++)
  {
    ind = map->Value(i);
    if (ind == 0)
      continue;
    Handle(Transfer_Binder) binder = MapItem(ind);
    if (binder.IsNull())
      continue;
    Transfer_StatusExec     statex = binder->StatusExec();
    Handle(Interface_Check) check  = binder->Check();
    if (statex != Transfer_StatusInitial && statex != Transfer_StatusDone && !check->HasFailed())
      check->AddFail("Transfer in Abnormal Status (!= Initial or Done)");
    if (!check->HasFailed() && (erronly || check->NbWarnings() == 0))
      continue;
    const Handle(Standard_Transient)& ent = Mapped(ind);
    num                                   = CheckNum(ent);
    if (num == 0)
      num = ind;
    check->SetEntity(ent);
    list.Add(check, num);
  }
  return list;
}

//=================================================================================================

Standard_Boolean Transfer_ProcessForTransient::IsCheckListEmpty(
  const Handle(Standard_Transient)& start,
  const Standard_Integer            level,
  const Standard_Boolean            erronly) const
{
  Standard_Integer max = NbMapped();
  Standard_Integer ind = MapIndex(start);
  if (ind == 0)
    return Standard_False;
  Standard_Integer                 i1  = (level == 0 ? ind : 1);
  Standard_Integer                 i2  = (level == 0 ? ind : max);
  Handle(TColStd_HArray1OfInteger) map = new TColStd_HArray1OfInteger(i1, i2, 0);

  for (Standard_Integer i = i1; i <= i2; i++)
  {
    ind = map->Value(i);
    if (ind == 0)
      continue;
    Handle(Transfer_Binder) binder = MapItem(ind);
    if (binder.IsNull())
      continue;

    Transfer_StatusExec     statex = binder->StatusExec();
    Handle(Interface_Check) check  = binder->Check();
    if (statex != Transfer_StatusInitial && statex != Transfer_StatusDone)
      return Standard_False;
    if (check->HasFailed() || (!erronly && check->NbWarnings() > 0))
      return Standard_False;
  }
  return Standard_True;
}

//=================================================================================================

void Transfer_ProcessForTransient::RemoveResult(const Handle(Standard_Transient)& start,
                                                const Standard_Integer            level,
                                                const Standard_Boolean /*compute*/)
{
  // if (compute) ComputeScopes();
  Standard_Integer max = NbMapped();
  Standard_Integer ind = MapIndex(start);
  if (ind == 0)
    return;
  Standard_Integer                 i1  = (level == 0 ? ind : 1);
  Standard_Integer                 i2  = (level == 0 ? ind : max);
  Handle(TColStd_HArray1OfInteger) map = new TColStd_HArray1OfInteger(i1, i2, 0);

  Standard_Integer i;
  for (i = i1; i <= i2; i++)
  {
    ind = map->Value(i);
    if (ind == 0)
      continue;
    Handle(Transfer_Binder) binder = MapItem(ind);
    if (binder.IsNull())
      continue;
  }
}

//=================================================================================================

Standard_Integer Transfer_ProcessForTransient::CheckNum(const Handle(Standard_Transient)&) const
{
  return 0;
}
