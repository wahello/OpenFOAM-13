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
    Foam::functionObjects::fieldValue

Description
    Base class for field value -based function objects.

See also
    Foam::functionObject
    Foam::functionObjects::fvMeshFunctionObject
    Foam::functionObjects::logFiles

SourceFiles
    fieldValue.C

\*---------------------------------------------------------------------------*/

#ifndef fieldValue_functionObject_H
#define fieldValue_functionObject_H

#include "fvMeshFunctionObject.H"
#include "logFiles.H"
#include "Switch.H"
#include "Field.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of classes
class fvMesh;

namespace functionObjects
{

/*---------------------------------------------------------------------------*\
                         Class fieldValue Declaration
\*---------------------------------------------------------------------------*/

class fieldValue
:
    public fvMeshFunctionObject,
    public logFiles
{

protected:

    // Protected data

        //- The name of the type of value (volFieldValue, or surfaceFieldValue)
        const word valueType_;

        //- List of field names to operate on
        wordList fields_;

        //- Output field values flag
        Switch writeFields_;

        //- Results dictionary for external access of results
        dictionary resultDict_;


public:

    //- Run-time type information
    TypeName("fieldValue");

    // Declare runtime constructor selection table

        declareRunTimeSelectionTable
        (
            autoPtr,
            fieldValue,
            dictionary,
            (
                const word& name,
                const objectRegistry& obr,
                const dictionary& dict
            ),
            (name, obr, dict)
        );


    // Constructors

        //- Construct from Time and dictionary
        fieldValue
        (
            const word& name,
            const Time& runTime,
            const dictionary& dict,
            const word& valueType
        );

        //- Construct from objectRegistry and dictionary
        fieldValue
        (
            const word& name,
            const objectRegistry& obr,
            const dictionary& dict,
            const word& valueType
        );

        //- Return a reference to the selected fieldValue
        static autoPtr<fieldValue> New
        (
            const word& name,
            const objectRegistry& obr,
            const dictionary& dict,
            const bool output = true
        );


    //- Destructor
    virtual ~fieldValue();


    // Member Functions

        //- Return the reference to the construction dictionary
        inline const dictionary& dict() const;

        //- Return the list of fields required
        inline virtual wordList fields() const;

        //- Return the output field values flag
        inline const Switch& writeFields() const;

        //- Return access to the latest set of results
        inline const dictionary& resultDict() const;

        //- Read from dictionary
        virtual bool read(const dictionary& dict);

        //- Execute
        virtual bool execute();

        //- Write
        virtual bool write();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace functionObjects
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "fieldValueI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
