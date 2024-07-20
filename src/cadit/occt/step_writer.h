#ifndef NANO_OCCT_STEP_WRITER_H
#define NANO_OCCT_STEP_WRITER_H

#include <iostream>
#include <filesystem>

#include <BRep_Builder.hxx>
#include <Interface_Static.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDocStd_Application.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Compound.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XSControl_WorkSession.hxx>
#include <TDF_Label.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include "../../binding_core.h"
#include "../../helpers/helpers.h"



void step_writer_module(nb::module_ &m);

#endif // NANO_OCCT_STEP_WRITER_H
