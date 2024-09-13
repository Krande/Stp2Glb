#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom2d_Line.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <STEPControl_Writer.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <BRep_Builder.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Pnt2d.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Precision.hxx>
#include <TopLoc_Location.hxx>  // For location

void make_a_bspline_surf(const std::string& stp_file) {
    // Define the control points for the B-Spline surface from STEP file
    TColgp_Array2OfPnt controlPoints(1, 4, 1, 2);  // Adjust range as needed
    controlPoints.SetValue(1, 1, gp_Pnt(-0.5, 0.5, 0.0));
    controlPoints.SetValue(2, 1, gp_Pnt(-0.561, 0.272, 0.333));
    controlPoints.SetValue(3, 1, gp_Pnt(-0.622, 0.045, 0.667));
    controlPoints.SetValue(4, 1, gp_Pnt(-0.683, -0.183, 1.0));

    controlPoints.SetValue(1, 2, gp_Pnt(-0.5, -0.5, 0.0));
    controlPoints.SetValue(2, 2, gp_Pnt(-0.272, -0.561, 0.333));
    controlPoints.SetValue(3, 2, gp_Pnt(-0.045, -0.622, 0.667));
    controlPoints.SetValue(4, 2, gp_Pnt(0.183, -0.683, 1.0));

    // Define knots and multiplicities
    TColStd_Array1OfReal uKnots(1, 2);
    uKnots.SetValue(1, 0.0);
    uKnots.SetValue(2, 1224.744871392);  // From STEP data

    TColStd_Array1OfReal vKnots(1, 2);
    vKnots.SetValue(1, 3.0);
    vKnots.SetValue(2, 4.0);  // From STEP data

    TColStd_Array1OfInteger uMults(1, 2);
    uMults.SetValue(1, 4);
    uMults.SetValue(2, 4);

    TColStd_Array1OfInteger vMults(1, 2);
    vMults.SetValue(1, 2);
    vMults.SetValue(2, 2);

    // Create the B-Spline surface
    Handle(Geom_BSplineSurface) bsplineSurface = new Geom_BSplineSurface(
            controlPoints, uKnots, vKnots, uMults, vMults, 3, 1, Standard_False, Standard_False
    );

    // Create boundary points and 3D edges
    gp_Pnt p1(-0.5, 0.5, 0.0);          // Point from #55
    gp_Pnt p2(-0.5, -0.5, 0.0);         // Point from #56
    gp_Pnt p3(0.183, -0.683, 1.0);      // Point from #62
    gp_Pnt p4(-0.683, -0.183, 1.0);     // Point from #61

    TopoDS_Edge edge1 = BRepBuilderAPI_MakeEdge(p1, p2); // Edge from point #55 to #56
    TopoDS_Edge edge2 = BRepBuilderAPI_MakeEdge(p2, p3); // Edge from point #56 to #62
    TopoDS_Edge edge3 = BRepBuilderAPI_MakeEdge(p3, p4); // Edge from point #62 to #61
    TopoDS_Edge edge4 = BRepBuilderAPI_MakeEdge(p4, p1); // Edge from point #61 to #55

    // Create corresponding 2D curves in the parametric space (u-v space) of the B-Spline surface
    gp_Pnt2d uv1(0.0, 0.0);  // Parametric coordinates corresponding to p1 (u, v)
    gp_Pnt2d uv2(0.0, 1.0);  // Parametric coordinates corresponding to p2 (u, v)
    gp_Pnt2d uv3(1.0, 1.0);  // Parametric coordinates corresponding to p3 (u, v)
    gp_Pnt2d uv4(1.0, 0.0);  // Parametric coordinates corresponding to p4 (u, v)

    Handle(Geom2d_TrimmedCurve) c2dEdge1 = new Geom2d_TrimmedCurve(new Geom2d_Line(gp_Lin2d(uv1, gp_Dir2d(uv2.XY() - uv1.XY()))), 0.0, 1.0);
    Handle(Geom2d_TrimmedCurve) c2dEdge2 = new Geom2d_TrimmedCurve(new Geom2d_Line(gp_Lin2d(uv2, gp_Dir2d(uv3.XY() - uv2.XY()))), 0.0, 1.0);
    Handle(Geom2d_TrimmedCurve) c2dEdge3 = new Geom2d_TrimmedCurve(new Geom2d_Line(gp_Lin2d(uv3, gp_Dir2d(uv4.XY() - uv3.XY()))), 0.0, 1.0);
    Handle(Geom2d_TrimmedCurve) c2dEdge4 = new Geom2d_TrimmedCurve(new Geom2d_Line(gp_Lin2d(uv4, gp_Dir2d(uv1.XY() - uv4.XY()))), 0.0, 1.0);

    // Assign the 2D curves to the edges on the B-Spline surface using the correct signature
    BRep_Builder builder;
    TopLoc_Location identityLocation;  // No transformation (identity)
    builder.UpdateEdge(edge1, c2dEdge1, bsplineSurface, identityLocation, Precision::Confusion());
    builder.UpdateEdge(edge2, c2dEdge2, bsplineSurface, identityLocation, Precision::Confusion());
    builder.UpdateEdge(edge3, c2dEdge3, bsplineSurface, identityLocation, Precision::Confusion());
    builder.UpdateEdge(edge4, c2dEdge4, bsplineSurface, identityLocation, Precision::Confusion());

    // Create a wire from the edges to represent the boundary
    BRepBuilderAPI_MakeWire wireMaker;
    wireMaker.Add(edge1);
    wireMaker.Add(edge2);
    wireMaker.Add(edge3);
    wireMaker.Add(edge4);
    TopoDS_Wire wire = wireMaker.Wire();

    // Create a face from the B-Spline surface with the boundary wire
    TopoDS_Face face = BRepBuilderAPI_MakeFace(bsplineSurface, wire);

    // Optionally, update the face tolerance
    builder.UpdateFace(face, 1e-6);  // Set tolerance if needed

    // Create a shell and add the face
    TopoDS_Shell shell;
    builder.MakeShell(shell);
    builder.Add(shell, face);

    shell.Closed(Standard_True);  // Set the shell as closed

    // Export to STEP file if needed
    STEPControl_Writer writer;
    writer.Transfer(shell, STEPControl_AsIs);
    writer.Write(stp_file.c_str());

}
