/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2013-2025 OpenFOAM Foundation
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
    Foam::fv::effectivenessHeatExchanger

Description
    Heat exchanger model, based on an effectiveness

    The total heat exchange source is given by:
    \f[
        Q_t = e(\phi, \dot{m}_2) (T_2 - T_1) \phi c_p
    \f]

    where:
    \vartable
        Q_t       | total heat source
        e(\phi,\dot{m}_2) | effectiveness table
        \phi      | net mass flux entering heat exchanger [kg/s]
        \dot{m}_2 | secondary mass flow rate [kg/s]
        T_1       | primary inlet temperature [K]
        T_2       | secondary inlet temperature [K]
        c_p       | specific heat capacity [J/kg/K]
    \endvartable

    The distribution inside the hear exchanger is given by:
    \f[
        Q_c = \frac{V_c |U_c| (T_c - T_{ref})}{\sum(V_c |U_c| (T_c - T_{ref}))}
    \f]

    where:
    \vartable
        Q_c     | source for cell
        V_c     | volume of the cell [m^3]
        U_c     | local cell velocity [m/s]
        T_c     | local call temperature [K]
        T_{ref} | min or max(T) in cell zone depending on the sign of Q_t [K]
    \endvartable

Usage
    Example usage:
    \verbatim
    effectivenessHeatExchanger1
    {
        type                    effectivenessHeatExchanger;

        cellZone                porosity;

        secondaryMassFlowRate   1.0;
        secondaryInletT         336;
        primaryInletT           293;

        faceZone                facesZoneInletOriented;

        effectiveness           <function2>;
    }
    \endverbatim

    Note:
      - The effectiveness Function2 is described in terms of the primary and
        secondary mass flow rates and has the same units as the secondary mass
        flow rate and kg/s for phi.
      - faceZone is the faces at the inlet of the cellzone, it needs to be
        created with flip map flags. It is used to integrate the net mass flow
        rate into the heat exchanger

SourceFiles
    effectivenessHeatExchanger.C

\*---------------------------------------------------------------------------*/

#ifndef effectivenessHeatExchanger_H
#define effectivenessHeatExchanger_H

#include "fvModel.H"
#include "fvCellZone.H"
#include "Function2.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

/*---------------------------------------------------------------------------*\
              Class effectivenessHeatExchanger Declaration
\*---------------------------------------------------------------------------*/

class effectivenessHeatExchanger
:
    public fvModel
{
    // Private data

        //- The cellZone the fvConstraint applies to
        fvCellZone zone_;

        //- Secondary flow mass rate [kg/s]
        scalar secondaryMassFlowRate_;

        //- Inlet secondary temperature [K]
        scalar secondaryInletT_;

        //- Primary air temperature at the heat exchanger inlet [K]
        scalar primaryInletT_;

        //- 2D function for effectiveness [kg/s]
        //  function of primary and secondary mass flow rates [kg/s]
        autoPtr<Function2<scalar>> eTable_;

        //- Name of velocity field; default = U
        word UName_;

        //- Name of temperature field; default = T
        word TName_;

        //- Name of the flux
        word phiName_;

        //- Name of the faceZone at the heat exchange inlet
        word faceZoneName_;

        //- Id for the face zone
        label zoneIndex_;

         //- Local list of face IDs
        labelList faceId_;

        //- Local list of patch ID per face
        labelList facePatchId_;

        //- List of +1/-1 representing face flip map (1 use as is, -1 negate)
        labelList faceSign_;

        //- Area of the face zone
        scalar faceZoneArea_;


private:

    // Private Member Functions

        //- Non-virtual read
        void readCoeffs(const dictionary& dict);

        //- Set the zone information
        void setZone();

        //- Calculate total area of faceZone across processors
        void calculateTotalArea(scalar& area) const;


public:

    //- Runtime type information
    TypeName("effectivenessHeatExchanger");


    // Constructors

        //- Construct from components
        effectivenessHeatExchanger
        (
            const word& name,
            const word& modelType,
            const fvMesh& mesh,
            const dictionary& dict
        );

        //- Disallow default bitwise copy construction
        effectivenessHeatExchanger
        (
            const effectivenessHeatExchanger&
        ) = delete;


    //- Destructor
    virtual ~effectivenessHeatExchanger()
    {}


    // Member Functions

        //- Return the list of fields for which the fvModel adds source term
        //  to the transport equation
        virtual wordList addSupFields() const;

        //- Explicit and implicit source for compressible equation
        virtual void addSup
        (
            const volScalarField& rho,
            const volScalarField& he,
            fvMatrix<scalar>& eqn
        ) const;


        // Mesh changes

            //- Update for mesh motion
            virtual bool movePoints();

            //- Update topology using the given map
            virtual void topoChange(const polyTopoChangeMap&);

            //- Update from another mesh using the given map
            virtual void mapMesh(const polyMeshMap&);

            //- Redistribute or update using the given distribution map
            virtual void distribute(const polyDistributionMap&);


        // IO

            //- Read dictionary
            virtual bool read(const dictionary& dict);


    // Member Operators

        //- Disallow default bitwise assignment
        void operator=(const effectivenessHeatExchanger&) = delete;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
