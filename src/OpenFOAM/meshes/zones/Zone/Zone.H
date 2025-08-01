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
    Foam::Zone

Description
    Base class for zones

SourceFiles
    Zone.C

\*---------------------------------------------------------------------------*/

#ifndef Zone_H
#define Zone_H

#include "className.H"
#include "labelList.H"
#include "dictionary.H"
#include "HashSet.H"
#include "Map.H"
#include "pointFieldFwd.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of friend functions and operators

class polyTopoChangeMap;
class polyMeshMap;
class polyDistributionMap;

template<class ZoneType, class ZonesType> class Zone;

template<class ZoneType, class ZonesType>
Ostream& operator<<(Ostream&, const Zone<ZoneType, ZonesType>&);

/*---------------------------------------------------------------------------*\
                            Class Zone Declaration
\*---------------------------------------------------------------------------*/

template<class ZoneType, class ZonesType>
class Zone
:
    public labelList
{

protected:

    // Protected data

        //- Name of zone
        word name_;

        //- Reference to zone list
        const ZonesType& zones_;

        //- Flag indicating the zone is updated by the generator
        //  following mesh motion
        bool moveUpdate_;

        //- Flag indicating the zone is updated by the generator
        //  following topology change
        bool topoUpdate_;


        // Demand-driven private data

            //- Map of labels in zone for fast location lookup
            mutable Map<label>* lookupMapPtr_;


    // Protected Member Functions

        //- Select the
        template<class Type>
        void select(const Type& zone);

        //- Construct the look-up map
        void calcLookupMap() const;

        //- Update the addressing using the maps provided
        void topoChange(const labelList& map, const labelList& reverseMap);


public:

    // Constructors

        //- Construct from components
        Zone
        (
            const word& name,
            const labelUList& indices,
            const ZonesType& zones,
            const bool moveUpdate = false,
            const bool topoUpdate = false
        );

        //- Construct from components, moving contents
        Zone
        (
            const word& name,
            labelList&& indices,
            const ZonesType& zones,
            const bool moveUpdate = false,
            const bool topoUpdate = false
        );

        //- Construct from dictionary
        Zone
        (
            const word& name,
            const dictionary&,
            const ZonesType& zones
        );

        //- Construct given the original zone and resetting the
        //  cell list and mesh zones information
        Zone
        (
            const Zone&,
            const word& name,
            const labelUList& indices,
            const ZonesType& zones
        );

        //- Construct given the original zone, resetting the
        //  cell list and mesh zones information
        Zone
        (
            const Zone&,
            const word& name,
            labelList&& indices,
            const ZonesType& zones
        );

        //- Disallow default bitwise copy construction
        Zone(const Zone&) = delete;


    //- Destructor
    ~Zone();


    // Member Functions

        //- Return name
        const word& name() const
        {
            return name_;
        }

        //- Return name as the keyword
        const word& keyword() const
        {
            return name_;
        }

        //- Return ZonesType reference
        const ZonesType& zones() const;

        //- Flag indicating the zone is updated by the generator
        //  following mesh motion
        bool moveUpdate() const
        {
            return moveUpdate_;
        }

        //- Flag indicating the zone is updated by the generator
        //  following mesh topology change
        bool topoUpdate() const
        {
            return topoUpdate_;
        }

        //- Map storing the local index for every global index.  Used to find
        //  the index of the item in the zone from the known global index. If
        //  the item is not in the zone, returns -1
        label localIndex(const label globalIndex) const;

        //- Return a reference to the look-up map
        const Map<label>& lookupMap() const;

        //- Clear addressing
        void clearAddressing();

        //- Check zone definition with max size given. Return true if in error.
        bool checkDefinition
        (
            const label maxSize,
            const bool report = false
        ) const;

        //- Insert given indices into zone
        void insert(const labelHashSet& newIndices);

        //- Swap two zones
        void swap(Zone&);

        //- Correct patch after moving points
        void movePoints(const pointField&)
        {}

        //- Update from another mesh using the given map
        void mapMesh(const polyMeshMap&);

        //- Redistribute or update using the given distribution map
        void distribute(const polyDistributionMap&);


    // Member Operators

        //- Assignment operator
        void operator=(const Zone&);

        //- Move assignment operator
        void operator=(Zone&&);

        //- Assignment operator to indices
        void operator=(const labelUList&);

        //- Move assignment of indices
        void operator=(labelList&&);


    // I-O

        //- Ostream Operator
        friend Ostream& operator<< <ZoneType, ZonesType>
        (
            Ostream&,
            const Zone<ZoneType, ZonesType>&
        );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "Zone.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
