// Created on: 1996-12-30
// Created by: Stagiaire Mary FABIEN
// Copyright (c) 1996-1999 Matra Datavision
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

#ifndef _BRepBuilderAPI_GTransform_HeaderFile
#define _BRepBuilderAPI_GTransform_HeaderFile

#include <Standard.hxx>
#include <Standard_DefineAlloc.hxx>

#include <gp_GTrsf.hxx>
#include <BRepBuilderAPI_Collect.hxx>
#include <BRepBuilderAPI_ModifyShape.hxx>
#include <TopTools_ListOfShape.hxx>
class TopoDS_Shape;

//! Geometric transformation on a shape.
//! The transformation to be applied is defined as a gp_GTrsf
//! transformation. It may be:
//! -      a transformation equivalent to a gp_Trsf transformation, the
//! most common case: you should , however, use a BRepAPI_Transform
//! object to perform this kind of transformation; or
//! -      an affinity, or
//! -      more generally, any type of point transformation which may
//! be defined by a three row, four column matrix of transformation.
//! In the last two cases, the underlying geometry of the
//! following shapes may change:
//! -      a curve which supports an edge of the shape, or
//! -      a surface which supports a face of the shape;
//! For example, a circle may be transformed into an ellipse when
//! applying an affinity transformation.
//! The transformation is applied to:
//! -      all the curves which support edges of the shape, and
//! -      all the surfaces which support faces of the shape.
//! A GTransform object provides a framework for:
//! -      defining the geometric transformation to be applied,
//! -      implementing the transformation algorithm, and
//! -      consulting the result.
class BRepBuilderAPI_GTransform : public BRepBuilderAPI_ModifyShape
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructs a framework for applying the geometric
  //! transformation T to a shape. Use the function
  //! Perform to define the shape to transform.
  Standard_EXPORT BRepBuilderAPI_GTransform(const gp_GTrsf& T);

  //! Constructs a framework for applying the geometric
  //! transformation T to a shape, and applies it to the shape S.
  //! -   If the transformation T is direct and isometric (i.e. if
  //! the determinant of the vectorial part of T is equal to
  //! 1.), and if Copy equals false (default value), the
  //! resulting shape is the same as the original but with
  //! a new location assigned to it.
  //! -   In all other cases, the transformation is applied to
  //! a duplicate of S.
  //! Use the function Shape to access the result.
  //! Note: the constructed framework can be reused to
  //! apply the same geometric transformation to other
  //! shapes: just specify them with the function Perform.
  Standard_EXPORT BRepBuilderAPI_GTransform(const TopoDS_Shape&    S,
                                            const gp_GTrsf&        T,
                                            const Standard_Boolean Copy = Standard_False);

  //! Applies the geometric transformation defined at the
  //! time of construction of this framework to the shape S.
  //! -   If the transformation T is direct and isometric (i.e. if
  //! the determinant of the vectorial part of T is equal to
  //! 1.), and if Copy equals false (default value), the
  //! resulting shape is the same as the original but with
  //! a new location assigned to it.
  //! -   In all other cases, the transformation is applied to a duplicate of S.
  //! Use the function Shape to access the result.
  //! Note: this framework can be reused to apply the same
  //! geometric transformation to other shapes: just specify
  //! them by calling the function Perform again.
  Standard_EXPORT void Perform(const TopoDS_Shape& S, const Standard_Boolean Copy = Standard_False);

  //! Returns the list  of shapes modified from the shape
  //! <S>.
  Standard_EXPORT virtual const TopTools_ListOfShape& Modified(const TopoDS_Shape& S)
    Standard_OVERRIDE;

  //! Returns the modified shape corresponding to <S>.
  Standard_EXPORT virtual TopoDS_Shape ModifiedShape(const TopoDS_Shape& S) const Standard_OVERRIDE;

protected:
private:
  gp_GTrsf               myGTrsf;
  BRepBuilderAPI_Collect myHist;
};

#endif // _BRepBuilderAPI_GTransform_HeaderFile
