/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2014-2025 OpenFOAM Foundation
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
    Foam::fv::solidificationMelting

Description
    This source is designed to model the effect of solidification and melting
    processes, e.g. windshield defrosting.

    The isotherm phase change occurs at the melting temperature, \c Tsol (= \c
    Tliq). The not isotherm phase change occurs between solidus and liquidus
    temperature, \c Tsol < \c Tliq respectively, as long as the melt fraction is
    greater than the max eutectic melt fraction, \c alpha1e (0 = pure_substance,
    1 = eutectic_mixture is not permitted), where a linear eutectic melt
    fraction to temperature relation is considered; e.g. given a specific
    quantity of a binary system, \c alpha1 is its melt fraction and \c alpha0 is
    its solid fraction, such that \c alpha0 = 1 - \c alpha1 therefore, assuming
    infinite solute diffusion, the quantity of a component in solid phase is
    (1 - \c alpha1) * \c CS where \c CS is the solid concentration of the
    considered component and the quantity of a component in liquid phase is \c
    alpha1 * \c CL where \c CL is the melt concentration of the considered
    component; considering that the total quantity of a component must be equal
    to the sum of the quantities of the considered component in the liquid and
    solid phases, if \c C0 is the initial concentration of the considered
    component before the phase change, then:
    \c C0 = (1 - \c alpha1) * \c CS + \c alpha1 * \c CL      (lever rule)
    from which: \c alpha1 = (\c C0 - \c CS) / (\c CL - \c CS)
    and thus:
    - for a miscible binary system \c alpha1e = 0;
    - for a binary system not miscible at solid state
      \c alpha1e = \c C0 / \c CLE where \c CLE is eutectic melt concentration;
    - for a binary system partially-miscible at solid state
      \c alpha1e = (\c C0 - \c CSE) / (\c CLE - \c CSE) where \c CSE is eutectic
      solid concentration of the relative solid solution.

    The presence of the solid phase in the flow field is incorporated into the
    model as a momentum porosity contribution; the energy associated with the
    phase change is added as an enthalpy contribution.

    References:
    \verbatim
        Voller, V. R., & Prakash, C. (1987).
        A fixed grid numerical modelling methodology for convection-diffusion
        mushy region phase-change problems.
        International Journal of Heat and Mass Transfer, 30(8), 1709-1719.

        Swaminathan, C. R., & Voller, V. R. (1992).
        A general enthalpy method for modeling solidification processes.
        Metallurgical transactions B, 23(5), 651-664.
    \endverbatim

    The model generates the field \c \<name\>:alpha1 which can be visualised to
    to show the melt distribution as a fraction [0-1].

Usage
    Example usage:
    \verbatim
    solidificationMelting1
    {
        type            solidificationMelting;

        cellZone        iceZone;

        Tsol            273;
        L               334000;
        thermoMode      thermo;
        beta            50e-6;
        rhoRef          800;
    }
    \endverbatim

    Where:
    \table
        Property   | Description                     | Required | Default value
        Tsol       | Solidus temperature [K]             | yes      |
        Tliq       | Liquidus temperature [K]            | no       | Tsol
        alpha1e    | Max eutectic melt fraction [0-1[    | no       | 0
        L          | Latent heat of fusion [J/kg]        | yes      |
        relax      | Relaxation coefficient [0-1]        | no       | 0.9
        thermoMode | Thermo mode [thermo or lookup]      | yes      |
        rhoRef     | Reference (solid) density [kg/m^3]  | yes      |
        rho        | Name of density field               | no       | rho
        T          | Name of temperature field           | no       | T
        Cp         | Name of specific heat field         | no       | Cp
        U          | Name of velocity field              | no       | U
        phi        | Name of flux field                  | no       | phi
        Cu         | Model coefficient [1/s]             | no       | 100000
        q          | Model coefficient                   | no       | 0.001
        beta       | Thermal expansion coefficient [1/K] | yes      |
        g          | Acceleration due to gravity         | no       |
    \endtable

SourceFiles
    solidificationMelting.C

\*---------------------------------------------------------------------------*/

#ifndef solidificationMelting_H
#define solidificationMelting_H

#include "fvModel.H"
#include "fvCellZone.H"
#include "fvMesh.H"
#include "volFields.H"
#include "NamedEnum.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

/*---------------------------------------------------------------------------*\
                 Class solidificationMelting Declaration
\*---------------------------------------------------------------------------*/

class solidificationMelting
:
    public fvModel
{
public:

    enum class thermoMode
    {
        thermo,
        lookup
    };

    static const NamedEnum<thermoMode, 2> thermoModeTypeNames_;


private:

    // Private Data

        //- The cellZone the fvConstraint applies to
        fvCellZone zone_;

        //- Temperature at which isotherm melting occurs
        //  and not isotherm melting starts e.g. eutectic [K]
        scalar Tsol_;

        //- Temperature at which not isotherm melting stops (Tliq > Tsol) [K]
        scalar Tliq_;

        //- Max eutectic melt fraction [0-1[
        //  where 0 = pure substance is default value,
        //        1 = eutectic mixture is not permitted
        scalar alpha1e_;

        //- Latent heat of fusion [J/kg]
        scalar L_;

        //- Phase fraction under-relaxation coefficient
        scalar relax_;

        //- Thermodynamics mode
        thermoMode mode_;

        //- Reference density - typically the solid density
        scalar rhoRef_;

        //- Name of temperature field - default = "T" (optional)
        word TName_;

        //- Name of specific heat capacity field - default = "Cp" (optional)
        word CpName_;

        //- Name of velocity field - default = "U" (optional)
        word UName_;

        //- Name of flux field - default = "phi" (optional)
        word phiName_;

        //- Mushy region momentum sink coefficient [1/s]; default = 10^5
        scalar Cu_;

        //- Coefficient used in porosity calc - default = 0.001
        scalar q_;

        //- Thermal expansion coefficient [1/K]
        scalar beta_;

        //- Reference heat capacity for thermoMode::lookup
        scalar CpRef_;

        //- Gravitational acceleration
        vector g_;

        //- Phase fraction indicator field
        mutable volScalarField alpha1_;

        //- Current time index (used for updating)
        mutable label curTimeIndex_;

        //- Temperature change cached for source calculation when alpha1 updated
        mutable scalarField deltaT_;


    // Private Member Functions

        //- Non-virtual read
        void readCoeffs(const dictionary& dict);

        //- Return the specific heat capacity field
        tmp<volScalarField> Cp() const;

        //- Return the gravity vector
        vector g() const;

        //- Update the model
        void update(const volScalarField& Cp) const;

        //- Helper function to apply to the energy equation
        template<class RhoFieldType>
        inline void apply(const RhoFieldType& rho, fvMatrix<scalar>& eqn) const;


public:

    //- Runtime type information
    TypeName("solidificationMelting");


    // Constructors

        //- Construct from explicit source name and mesh
        solidificationMelting
        (
            const word& name,
            const word& modelType,
            const fvMesh& mesh,
            const dictionary& dict
        );

        //- Disallow default bitwise copy construction
        solidificationMelting
        (
            const solidificationMelting&
        ) = delete;


    // Member Functions

        // Checks

            //- Return the list of fields for which the fvModel adds source term
            //  to the transport equation
            virtual wordList addSupFields() const;


        // Evaluation

            //- Add explicit contribution to enthalpy equation
            virtual void addSup
            (
                const volScalarField& he,
                fvMatrix<scalar>& eqn
            ) const;

            //- Add implicit contribution to momentum equation
            virtual void addSup
            (
                const volVectorField& U,
                fvMatrix<vector>& eqn
            ) const;

            //- Add explicit contribution to compressible enthalpy equation
            virtual void addSup
            (
                const volScalarField& rho,
                const volScalarField& he,
                fvMatrix<scalar>& eqn
            ) const;

            //- Add implicit contribution to compressible momentum equation
            virtual void addSup
            (
                const volScalarField& rho,
                const volVectorField& U,
                fvMatrix<vector>& eqn
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

            //- Read source dictionary
            virtual bool read(const dictionary& dict);


    // Member Operators

        //- Disallow default bitwise assignment
        void operator=(const solidificationMelting&) = delete;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
