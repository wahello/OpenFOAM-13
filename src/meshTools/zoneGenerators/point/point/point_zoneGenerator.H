/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2025 OpenFOAM Foundation
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
    Foam::zoneGenerators::point

Description
    A zoneGenerator which converts the point, cell and face zones
    from a list of zoneGenerators into a pointZone

    If a pointZone with the same name as the pointZone to be generated is
    returned by a zoneGenerator in the list it is ignored.

Usage
    \table
        Property     | Description             | Required | Default value
        type         | Type: point             | yes      |
        name         | Name of the zone        | no       | zoneGenerator name
        moveUpdate   | Switch to update after mesh motion | no | false
    \endtable

    To convert cellZone named \c cone1 generated by
    \verbatim
        cone1
        {
            type        truncatedCone;
            zoneType    cell;

            point1      (-0.0075 0 -1);
            point2      (-0.003 0.0025 1);
            radius1     0.001;
            radius2     0.0005;
        }
    \endverbatim
    into a pointZone also named \c cone1, the following zoneGenerator named \c
    cone1Points can be used
    \verbatim
        cone1Points
        {
            type        point;
            name        cone1;

            cone1;
        }
    \endverbatim
    If there is also a faceZone named \c cone1 which should not be included in
    the conversion to the pointZone the lookup of the \c cone1 can specify the
    cellZone type:
    \verbatim
        cone1Points
        {
            type        point;
            name        cone1;

            cone1
            {
                zoneType    cell;
            }
        }
    \endverbatim

SourceFiles
    point_zoneGenerator.C

\*---------------------------------------------------------------------------*/

#ifndef point_zoneGenerator_H
#define point_zoneGenerator_H

#include "zoneGeneratorList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace zoneGenerators
{

/*---------------------------------------------------------------------------*\
                              Class point Declaration
\*---------------------------------------------------------------------------*/

class point
:
    public zoneGenerator
{
    // Private Data

        zoneGeneratorList zoneGenerators_;


public:

    //- Runtime type information
    TypeName("point");


    // Constructors

        //- Construct from dictionary
        point
        (
            const word& name,
            const polyMesh& mesh,
            const dictionary& dict
        );


    //- Destructor
    virtual ~point();


    // Member Functions

        virtual zoneSet generate() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace zoneGenerators
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
