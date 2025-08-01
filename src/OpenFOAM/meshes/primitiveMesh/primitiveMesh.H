/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2025 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::primitiveMesh

Description
    Cell-face mesh analysis engine

SourceFiles
    primitiveMeshI.H
    primitiveMesh.C
    primitiveMeshClear.C
    primitiveMeshCellCells.C
    primitiveMeshEdgeCells.C
    primitiveMeshPointCells.C
    primitiveMeshCells.C
    primitiveMeshEdgeFaces.C
    primitiveMeshPointFaces.C
    primitiveMeshCellEdges.C
    primitiveMeshPointEdges.C
    primitiveMeshPointPoints.C
    primitiveMeshEdges.C
    primitiveMeshCellCentresAndVols.C
    primitiveMeshFaceCentresAndAreas.C
    primitiveMeshFindCell.C

\*---------------------------------------------------------------------------*/

#ifndef primitiveMesh_H
#define primitiveMesh_H

#include "DynamicList.H"
#include "edgeList.H"
#include "pointField.H"
#include "faceList.H"
#include "cellList.H"
#include "cellShapeList.H"
#include "labelList.H"
#include "boolList.H"
#include "HashSet.H"
#include "Map.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class PackedBoolList;

/*---------------------------------------------------------------------------*\
                        Class primitiveMesh Declaration
\*---------------------------------------------------------------------------*/

class primitiveMesh
{
    // Private Member Data

        // Primitive size data

            //- Number of internal points (or -1 if points not sorted)
            label nInternalPoints_;

            //- Number of points
            label nPoints_;

            //- Number of internal edges using 0 boundary points
            mutable label nInternal0Edges_;

            //- Number of internal edges using 0 or 1 boundary points
            mutable label nInternal1Edges_;

            //- Number of internal edges using 0,1 or 2boundary points
            mutable label nInternalEdges_;

            //- Number of edges
            mutable label nEdges_;

            //- Number of internal faces
            label nInternalFaces_;

            //- Number of faces
            label nFaces_;

            //- Number of cells
            label nCells_;


        // Shapes

            //- Cell shapes
            mutable cellShapeList* cellShapesPtr_;

            //- Edges
            mutable edgeList* edgesPtr_;


        // Connectivity

            //- Cell-cells
            mutable labelListList* ccPtr_;

            //- Edge-cells
            mutable labelListList* ecPtr_;

            //- Point-cells
            mutable labelListList* pcPtr_;

            //- Cell-faces
            mutable cellList* cfPtr_;

            //- Edge-faces
            mutable labelListList* efPtr_;

            //- Point-faces
            mutable labelListList* pfPtr_;

            //- Cell-edges
            mutable labelListList* cePtr_;

            //- Face-edges
            mutable labelListList* fePtr_;

            //- Point-edges
            mutable labelListList* pePtr_;

            //- Point-points
            mutable labelListList* ppPtr_;

            //- Cell-points
            mutable labelListList* cpPtr_;


        // On-the-fly edge addressing storage

            //- Temporary storage for addressing.
            mutable DynamicList<label> labels_;

            //- Temporary storage for addressing
            mutable labelHashSet labelSet_;


        // Geometric data

            //- Cell centres
            mutable vectorField* cellCentresPtr_;

            //- Face centres
            mutable vectorField* faceCentresPtr_;

            //- Cell volumes
            mutable scalarField* cellVolumesPtr_;

            //- Face areas
            mutable vectorField* faceAreasPtr_;

            //- Face area magnitudes
            mutable scalarField* magFaceAreasPtr_;


    // Private Member Functions

        // Topological calculations

            //- Calculate cell shapes
            void calcCellShapes() const;

            //- Calculate cell-cell addressing
            void calcCellCells() const;

            //- Calculate point-cell addressing
            void calcPointCells() const;

            //- Calculate cell-face addressing
            void calcCells() const;

            //- Calculate edge list
            void calcCellEdges() const;

            //- Calculate point-point addressing
            void calcPointPoints() const;

            //- Calculate edges, pointEdges and faceEdges (if doFaceEdges=true)
            //  During edge calculation, a larger set of data is assembled.
            //  Create and destroy as a set, using clearOutEdges()
            void calcEdges(const bool doFaceEdges) const;

            void clearOutEdges();

            //- Helper: return (after optional creation) edge between two points
            static label getEdge
            (
                List<DynamicList<label>>&,
                DynamicList<edge>&,
                const label,
                const label
            );

            //- For on-the-fly addressing calculation
            static label findFirstCommonElementFromSortedLists
            (
                const labelList&,
                const labelList&
            );


        // Geometrical calculations

            //- Calculate face centres and areas
            void calcFaceCentresAndAreas() const;

            //- Calculate face centres and areas
            void makeFaceCentresAndAreas
            (
                const pointField& p,
                vectorField& fCtrs,
                vectorField& fAreas,
                scalarField& magfAreas
            ) const;

            //- Calculate cell centres and volumes
            void calcCellCentresAndVols() const;

            //- Calculate cell centres and volumes
            void makeCellCentresAndVols
            (
                const vectorField& fCtrs,
                const vectorField& fAreas,
                vectorField& cellCtrs,
                scalarField& cellVols
            ) const;

            //- Calculate edge vectors
            void calcEdgeVectors() const;


protected:

    // Protected Static Member Functions

        //- Test if the supplied methods are the same. Same type overload.
        //  Compares pointers.
        template<class Class, class Type>
        static inline bool isMethod
        (
            const Type& (Class::*method)() const,
            const Type& (Class::*otherMethod)() const
        );

        //- Test if the supplied methods are the same. Different type
        //  overload. Returns false.
        template<class Class, class Type, class OtherType>
        static inline bool isMethod
        (
            const Type& (Class::*method)() const,
            const OtherType& (Class::*otherMethod)() const
        );


    // Protected Constructors

        //- Construct null
        primitiveMesh();


public:

    //- Runtime type information
    ClassName("primitiveMesh");


    // Static data

        //- Estimated number of cells per edge
        static const unsigned cellsPerEdge_ = 4;

        //- Estimated number of cells per point
        static const unsigned cellsPerPoint_ = 8;

        //- Estimated number of faces per cell
        static const unsigned facesPerCell_ = 6;

        //- Estimated number of faces per edge
        static const unsigned facesPerEdge_ = 4;

        //- Estimated number of faces per point
        static const unsigned facesPerPoint_ = 12;

        //- Estimated number of edges per cell
        static const unsigned edgesPerCell_ = 12;

        //- Estimated number of edges per cell
        static const unsigned edgesPerFace_ = 4;

        //- Estimated number of edges per point
        static const unsigned edgesPerPoint_ = 6;

        //- Estimated number of points per cell
        static const unsigned pointsPerCell_ = 8;

        //- Estimated number of points per face
        static const unsigned pointsPerFace_ = 4;


    // Constructors

        //- Construct from components
        primitiveMesh
        (
            const label nPoints,
            const label nInternalFaces,
            const label nFaces,
            const label nCells
        );

        //- Move constructor
        primitiveMesh(primitiveMesh&&) = default;

        //- Disallow default bitwise copy construction
        primitiveMesh(const primitiveMesh&) = delete;


    //- Destructor
    virtual ~primitiveMesh();


    // Member Functions

        //- Reset this primitiveMesh given the primitive array sizes
        void reset
        (
            const label nPoints,
            const label nInternalFaces,
            const label nFaces,
            const label nCells
        );

        //- Reset this primitiveMesh given the primitive array sizes and cells
        void reset
        (
            const label nPoints,
            const label nInternalFaces,
            const label nFaces,
            const label nCells,
            cellList& cells
        );

        //- Reset this primitiveMesh given the primitive array sizes and cells
        void reset
        (
            const label nPoints,
            const label nInternalFaces,
            const label nFaces,
            const label nCells,
            cellList&& cells
        );


        // Access

            // Mesh size parameters

                inline label nPoints() const;
                inline label nEdges() const;
                inline label nInternalFaces() const;
                inline label nFaces() const;
                inline label nCells() const;

                // If points are ordered (nInternalPoints != -1):

                    //- Points not on boundary
                    inline label nInternalPoints() const;

                    //- Internal edges (i.e. not on boundary face) using
                    //  no boundary point
                    inline label nInternal0Edges() const;

                    //- Internal edges using 0 or 1 boundary point
                    inline label nInternal1Edges() const;

                    //- Internal edges using 0,1 or 2 boundary points
                    inline label nInternalEdges() const;


            // Primitive mesh data

                //- Return mesh points
                virtual const pointField& points() const = 0;

                //- Return faces
                virtual const faceList& faces() const = 0;

                //- Face face-owner addressing
                virtual const labelList& faceOwner() const = 0;

                //- Face face-neighbour addressing
                virtual const labelList& faceNeighbour() const = 0;

                //- Return old points for mesh motion
                virtual const pointField& oldPoints() const = 0;


            // Derived mesh data

                //- Return cell shapes
                const cellShapeList& cellShapes() const;

                //- Return mesh edges. Uses calcEdges.
                const edgeList& edges() const;

                //- Helper function to calculate cell-face addressing from
                //  face-cell addressing. If nCells is not provided it will
                //  scan for the maximum.
                static void calcCells
                (
                    cellList&,
                    const labelUList& own,
                    const labelUList& nei,
                    const label nCells = -1
                );

                //- Helper function to calculate point ordering. Returns true
                //  if points already ordered, false and fills pointMap (old to
                //  new). Map splits points into those not used by any boundary
                //  face and those that are.
                static bool calcPointOrder
                (
                    label& nInternalPoints,
                    labelList& pointMap,
                    const faceList&,
                    const label nInternalFaces,
                    const label nPoints
                );

            // Return mesh connectivity

                const labelListList& cellCells() const;
                // faceCells given as owner and neighbour
                const labelListList& edgeCells() const;
                const labelListList& pointCells() const;

                const cellList& cells() const;
                // faceFaces considered unnecessary
                const labelListList& edgeFaces() const;
                const labelListList& pointFaces() const;

                const labelListList& cellEdges() const;
                const labelListList& faceEdges() const;
                // edgeEdges considered unnecessary
                const labelListList& pointEdges() const;
                const labelListList& pointPoints() const;
                const labelListList& cellPoints() const;


            // Geometric data (raw!)

                const vectorField& cellCentres() const;
                const vectorField& faceCentres() const;
                const scalarField& cellVolumes() const;
                const vectorField& faceAreas() const;
                const scalarField& magFaceAreas() const;


            // Mesh motion

                //- Move points, returns volumes swept by faces in motion
                tmp<scalarField> movePoints
                (
                    const pointField& p,
                    const pointField& oldP
                );


            //- Return true if given face label is internal to the mesh
            inline bool isInternalFace(const label faceIndex) const;


        // Useful derived info

            //- Return true if the point in the cell bounding box.
            //  The bounding box may be isotropically inflated by the fraction
            //  inflationFraction
            bool pointInCellBB
            (
                const point& p,
                label celli,
                scalar inflationFraction = 0
            ) const;

            //- Return true if the  point is in the cell
            bool pointInCell(const point& p, label celli) const;

            //- Find the cell with the nearest cell centre to location
            label findNearestCell(const point& location) const;

            //- Find cell enclosing this location (-1 if not in mesh)
            label findCell(const point& location) const;


        // Storage management

            //- Print a list of all the currently allocated mesh data
            void printAllocated() const;

            //- Return whether the result of the given method has been allocated
            template<class Type>
            inline bool has
            (
                const Type& (primitiveMesh::*method)() const
            ) const;

            // On-the-fly addressing calculation. These functions return either
            // a reference to the full addressing (if already calculated) or
            // a reference to the supplied storage. The one-argument ones
            // use member DynamicList labels_ so be careful when not storing
            // result.

            //- cellCells using cells.
            const labelList& cellCells
            (
                const label celli,
                DynamicList<label>&
            ) const;

            const labelList& cellCells(const label celli) const;

            //- cellPoints using cells
            const labelList& cellPoints
            (
                const label celli,
                DynamicList<label>&
            ) const;

            const labelList& cellPoints(const label celli) const;

            //- pointCells using pointFaces
            const labelList& pointCells
            (
                const label pointi,
                DynamicList<label>&
            ) const;

            const labelList& pointCells(const label pointi) const;

            //- pointPoints using edges, pointEdges
            const labelList& pointPoints
            (
                const label pointi,
                DynamicList<label>&
            ) const;

            const labelList& pointPoints(const label pointi) const;

            //- faceEdges using pointFaces, edges, pointEdges
            const labelList& faceEdges
            (
                const label facei,
                DynamicList<label>&
            ) const;

            const labelList& faceEdges(const label facei) const;

            //- edgeFaces using pointFaces, edges, pointEdges
            const labelList& edgeFaces
            (
                const label edgeI,
                DynamicList<label>&
            ) const;

            const labelList& edgeFaces(const label edgeI) const;

            //- edgeCells using pointFaces, edges, pointEdges
            const labelList& edgeCells
            (
                const label edgeI,
                DynamicList<label>&
            ) const;

            const labelList& edgeCells(const label edgeI) const;

            //- cellEdges using cells, pointFaces, edges, pointEdges
            const labelList& cellEdges
            (
                const label celli,
                DynamicList<label>&
            ) const;

            const labelList& cellEdges(const label celli) const;

            //- Clear geometry
            void clearGeom();

            //- Clear topological data
            void clearAddressing();

            //- Clear all geometry and addressing unnecessary for CFD
            void clearOut();


    // Member Operators

        //- Disallow default bitwise assignment
        void operator=(const primitiveMesh&) = delete;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "primitiveMeshI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
