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

Application
    createPatch

Description
    Utility to create patches out of selected boundary faces. Faces come either
    from existing patches or from a faceSet.

    More specifically it:
    - Creates new patches from selected boundary faces
    - Synchronises faces on coupled patches
    - Synchronises points on coupled patches (optional)
    - Removes non-constraint patches with no faces

\*---------------------------------------------------------------------------*/

#include "cyclicPolyPatch.H"
#include "syncTools.H"
#include "argList.H"
#include "polyMesh.H"
#include "Time.H"
#include "SortableList.H"
#include "OFstream.H"
#include "meshTools.H"
#include "zoneGenerator.H"
#include "faceSet.H"
#include "polyTopoChange.H"
#include "systemDict.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void changePatchID
(
    const polyMesh& mesh,
    const label faceID,
    const label patchID,
    polyTopoChange& meshMod
)
{
    meshMod.modifyFace
    (
        mesh.faces()[faceID],               // face
        faceID,                             // face ID
        mesh.faceOwner()[faceID],           // owner
        -1,                                 // neighbour
        false,                              // flip flux
        patchID                             // patch ID
    );
}


// Filter out the empty patches.
void filterPatches(polyMesh& mesh, const HashSet<word>& addedPatchNames)
{
    const polyBoundaryMesh& patches = mesh.boundaryMesh();

    // Patches to keep
    DynamicList<polyPatch*> allPatches(patches.size());

    label nOldPatches = returnReduce(patches.size(), sumOp<label>());

    // Copy old patches.
    forAll(patches, patchi)
    {
        const polyPatch& pp = patches[patchi];

        // Note: reduce possible since non-proc patches guaranteed in same order
        if (!isA<processorPolyPatch>(pp))
        {
            // Add if
            // - listed in createPatchDict
            // - or constraint type
            // - or non zero size
            if
            (
                addedPatchNames.found(pp.name())
             || polyPatch::constraintType(pp.type())
             || returnReduce(pp.size(), sumOp<label>()) > 0
            )
            {
                allPatches.append
                (
                    pp.clone
                    (
                        patches,
                        allPatches.size(),
                        pp.size(),
                        pp.start()
                    ).ptr()
                );
            }
            else
            {
                Info<< "Removing zero-sized patch " << pp.name()
                    << " type " << pp.type()
                    << " at position " << patchi << endl;
            }
        }
    }

    // Copy non-empty processor patches
    forAll(patches, patchi)
    {
        const polyPatch& pp = patches[patchi];

        if (isA<processorPolyPatch>(pp))
        {
            if (pp.size())
            {
                allPatches.append
                (
                    pp.clone
                    (
                        patches,
                        allPatches.size(),
                        pp.size(),
                        pp.start()
                    ).ptr()
                );
            }
            else
            {
                Info<< "Removing empty processor patch " << pp.name()
                    << " at position " << patchi << endl;
            }
        }
    }

    label nAllPatches = returnReduce(allPatches.size(), sumOp<label>());
    if (nAllPatches != nOldPatches)
    {
        Info<< "Removing patches." << endl;
        allPatches.shrink();
        mesh.removeBoundary();
        mesh.addPatches(allPatches);
    }
    else
    {
        Info<< "No patches removed." << endl;
        forAll(allPatches, i)
        {
            delete allPatches[i];
        }
    }
}


// Write current match for all patches the as OBJ files
void writeCyclicMatchObjs(const fileName& prefix, const polyMesh& mesh)
{
    const polyBoundaryMesh& patches = mesh.boundaryMesh();

    forAll(patches, patchi)
    {
        if
        (
            isA<cyclicPolyPatch>(patches[patchi])
         && refCast<const cyclicPolyPatch>(patches[patchi]).owner()
        )
        {
            const cyclicPolyPatch& cycPatch =
                refCast<const cyclicPolyPatch>(patches[patchi]);

            // Write patches
            {
                OFstream str(prefix+cycPatch.name() + ".obj");
                Pout<< "Writing " << cycPatch.name()
                    << " faces to " << str.name() << endl;
                meshTools::writeOBJ
                (
                    str,
                    cycPatch,
                    cycPatch.points()
                );
            }

            const cyclicPolyPatch& nbrPatch = cycPatch.nbrPatch();
            {
                OFstream str(prefix+nbrPatch.name()+".obj");
                Pout<< "Writing " << nbrPatch.name()
                    << " faces to " << str.name() << endl;
                meshTools::writeOBJ
                (
                    str,
                    nbrPatch,
                    nbrPatch.points()
                );
            }


            // Lines between corresponding face centres
            OFstream str(prefix+cycPatch.name()+nbrPatch.name()+"_match.obj");
            label vertI = 0;

            Pout<< "Writing cyclic match as lines between face centres to "
                << str.name() << endl;

            forAll(cycPatch, facei)
            {
                const point& fc0 = mesh.faceCentres()[cycPatch.start()+facei];
                meshTools::writeOBJ(str, fc0);
                vertI++;
                const point& fc1 = mesh.faceCentres()[nbrPatch.start()+facei];
                meshTools::writeOBJ(str, fc1);
                vertI++;

                str<< "l " << vertI-1 << ' ' << vertI << nl;
            }
        }
    }
}


// Synchronise points on both sides of coupled boundaries.
template<class CombineOp>
void syncPoints
(
    const polyMesh& mesh,
    pointField& points,
    const CombineOp& cop,
    const point& nullValue
)
{
    if (points.size() != mesh.nPoints())
    {
        FatalErrorInFunction
            << "Number of values " << points.size()
            << " is not equal to the number of points in the mesh "
            << mesh.nPoints() << abort(FatalError);
    }

    const polyBoundaryMesh& patches = mesh.boundaryMesh();

    // Is there any coupled patch with transformation?
    bool hasTransformation = false;

    if (Pstream::parRun())
    {
        // Send

        forAll(patches, patchi)
        {
            const polyPatch& pp = patches[patchi];

            if
            (
                isA<processorPolyPatch>(pp)
             && pp.nPoints() > 0
             && refCast<const processorPolyPatch>(pp).owner()
            )
            {
                const processorPolyPatch& procPatch =
                    refCast<const processorPolyPatch>(pp);

                // Get data per patchPoint in neighbouring point numbers.
                pointField patchInfo(procPatch.nPoints(), nullValue);

                const labelList& meshPts = procPatch.meshPoints();
                const labelList& nbrPts = procPatch.nbrPoints();

                forAll(nbrPts, pointi)
                {
                    label nbrPointi = nbrPts[pointi];
                    if (nbrPointi >= 0 && nbrPointi < patchInfo.size())
                    {
                        patchInfo[nbrPointi] = points[meshPts[pointi]];
                    }
                }

                OPstream toNbr
                (
                    Pstream::commsTypes::blocking,
                    procPatch.neighbProcNo()
                );
                toNbr << patchInfo;
            }
        }


        // Receive and set.

        forAll(patches, patchi)
        {
            const polyPatch& pp = patches[patchi];

            if
            (
                isA<processorPolyPatch>(pp)
             && pp.nPoints() > 0
             && !refCast<const processorPolyPatch>(pp).owner()
            )
            {
                const processorPolyPatch& procPatch =
                    refCast<const processorPolyPatch>(pp);

                pointField nbrPatchInfo(procPatch.nPoints());
                {
                    // We do not know the number of points on the other side
                    // so cannot use Pstream::read.
                    IPstream fromNbr
                    (
                        Pstream::commsTypes::blocking,
                        procPatch.neighbProcNo()
                    );
                    fromNbr >> nbrPatchInfo;
                }
                // Null any value which is not on neighbouring processor
                nbrPatchInfo.setSize(procPatch.nPoints(), nullValue);

                if (procPatch.transform().transformsPosition())
                {
                    hasTransformation = true;
                    procPatch.transform().transformPosition
                    (
                        nbrPatchInfo,
                        nbrPatchInfo
                    );
                }

                const labelList& meshPts = procPatch.meshPoints();

                forAll(meshPts, pointi)
                {
                    label meshPointi = meshPts[pointi];
                    points[meshPointi] = nbrPatchInfo[pointi];
                }
            }
        }
    }

    // Do the cyclics.
    forAll(patches, patchi)
    {
        const polyPatch& pp = patches[patchi];

        if
        (
            isA<cyclicPolyPatch>(pp)
         && refCast<const cyclicPolyPatch>(pp).owner()
        )
        {
            const cyclicPolyPatch& cycPatch =
                refCast<const cyclicPolyPatch>(pp);

            const edgeList& coupledPoints = cycPatch.coupledPoints();
            const labelList& meshPts = cycPatch.meshPoints();
            const cyclicPolyPatch& nbrPatch = cycPatch.nbrPatch();
            const labelList& nbrMeshPts = nbrPatch.meshPoints();

            pointField patchPoints(coupledPoints.size());

            forAll(coupledPoints, i)
            {
                const edge& e = coupledPoints[i];
                label point0 = meshPts[e[0]];
                patchPoints[i] = points[point0];
            }

            if (cycPatch.transform().transformsPosition())
            {
                hasTransformation = true;
                cycPatch.transform().invTransformPosition
                (
                    patchPoints,
                    patchPoints
                );
            }

            forAll(coupledPoints, i)
            {
                const edge& e = coupledPoints[i];
                label point1 = nbrMeshPts[e[1]];
                points[point1] = patchPoints[i];
            }
        }
    }

    //- Note: hasTransformation is only used for warning messages so
    //  reduction not strictly necessary.
    // reduce(hasTransformation, orOp<bool>());

    // Synchronise multiple shared points.
    const globalMeshData& pd = mesh.globalData();

    if (pd.nGlobalPoints() > 0)
    {
        if (hasTransformation)
        {
            WarningInFunction
                << "There are decomposed cyclics in this mesh with"
                << " transformations." << endl
                << "This is not supported. The result will be incorrect"
                << endl;
        }


        // Values on shared points.
        pointField sharedPts(pd.nGlobalPoints(), nullValue);

        forAll(pd.sharedPointLabels(), i)
        {
            label meshPointi = pd.sharedPointLabels()[i];
            // Fill my entries in the shared points
            sharedPts[pd.sharedPointAddr()[i]] = points[meshPointi];
        }

        // Combine on master.
        Pstream::listCombineGather(sharedPts, cop);
        Pstream::listCombineScatter(sharedPts);

        // Now we will all have the same information. Merge it back with
        // my local information.
        forAll(pd.sharedPointLabels(), i)
        {
            label meshPointi = pd.sharedPointLabels()[i];
            points[meshPointi] = sharedPts[pd.sharedPointAddr()[i]];
        }
    }
}



int main(int argc, char *argv[])
{
    #include "addNoOverwriteOption.H"
    #include "addMeshOption.H"
    #include "addRegionOption.H"
    #include "addDictOption.H"
    #include "setRootCase.H"
    #include "createTimeNoFunctionObjects.H"

    Foam::word meshRegionName = polyMesh::defaultRegion;
    args.optionReadIfPresent("region", meshRegionName);

    #include "setNoOverwrite.H"

    #include "createSpecifiedPolyMesh.H"

    const word oldInstance = mesh.pointsInstance();

    const dictionary dict(systemDict("createPatchDict", args, mesh));

    // Whether to write cyclic matches to .OBJ files
    const Switch writeCyclicMatch
    (
        dict.lookupOrDefault("writeCyclicMatch", false)
    );

    const polyBoundaryMesh& patches = mesh.boundaryMesh();

    // If running parallel check same patches everywhere
    patches.checkParallelSync(true);


    if (writeCyclicMatch)
    {
        writeCyclicMatchObjs("initial_", mesh);
    }

    // For backwards-compatibility read patches as list of dictionaries
    // if not a dictionary
    dictionary patchesListDict("patches", dict);

    if (!dict.isDict("patches"))
    {
        // Read patch construct info from dictionary
        PtrList<dictionary> patchSources(dict.lookup("patches"));

        forAll(patchSources, psi)
        {
            const dictionary& dict = patchSources[psi];
            patchesListDict.add(dict.lookup<word>("name"), dict);
        }
    }

    const dictionary& patchesDict =
        dict.isDict("patches")
      ? dict.subDict("patches")
      : patchesListDict;


    HashSet<word> addedPatchNames;
    forAllConstIter(dictionary, patchesDict, iter)
    {
        addedPatchNames.insert(iter().keyword());
    }


    // 1. Add all new patches
    // ~~~~~~~~~~~~~~~~~~~~~~

    if (patchesDict.size())
    {
        // Old and new patches.
        DynamicList<polyPatch*> allPatches(patches.size()+patchesDict.size());

        label startFacei = mesh.nInternalFaces();

        // Copy old patches.
        forAll(patches, patchi)
        {
            const polyPatch& pp = patches[patchi];

            if (!isA<processorPolyPatch>(pp))
            {
                allPatches.append
                (
                    pp.clone
                    (
                        patches,
                        patchi,
                        pp.size(),
                        pp.start()
                    ).ptr()
                );
                startFacei += pp.size();
            }
        }

        forAllConstIter(dictionary, patchesDict, iter)
        {
            const word& patchName = iter().keyword();
            const dictionary& dict = iter().dict();

            label destPatchi = patches.findIndex(patchName);

            if (destPatchi == -1)
            {
                dictionary patchDict(dict.subDict("patchInfo"));

                destPatchi = allPatches.size();

                Info<< "Adding new patch " << patchName
                    << " as patch " << destPatchi
                    << " from " << patchDict << endl;

                patchDict.set("nFaces", 0);
                patchDict.set("startFace", startFacei);

                // Add an empty patch.
                allPatches.append
                (
                    polyPatch::New
                    (
                        patchName,
                        patchDict,
                        destPatchi,
                        patches
                    ).ptr()
                );
            }
            else
            {
                Info<< "Patch '" << patchName << "' already exists.  Only "
                    << "moving patch faces - type will remain the same" << endl;
            }
        }

        // Copy old patches.
        forAll(patches, patchi)
        {
            const polyPatch& pp = patches[patchi];

            if (isA<processorPolyPatch>(pp))
            {
                allPatches.append
                (
                    pp.clone
                    (
                        patches,
                        patchi,
                        pp.size(),
                        pp.start()
                    ).ptr()
                );
                startFacei += pp.size();
            }
        }

        allPatches.shrink();
        mesh.removeBoundary();
        mesh.addPatches(allPatches);

        Info<< endl;
    }



    // 2. Repatch faces
    // ~~~~~~~~~~~~~~~~

    polyTopoChange meshMod(mesh);

    forAllConstIter(dictionary, patchesDict, iter)
    {
        const word& patchName = iter().keyword();
        const dictionary& dict = iter().dict();

        label destPatchi = patches.findIndex(patchName);

        if (destPatchi == -1)
        {
            FatalErrorInFunction
                << "patch " << patchName << " not added. Problem."
                << abort(FatalError);
        }

        const word sourceType(dict.lookup("constructFrom"));

        if (sourceType == "patches")
        {
            labelHashSet patchesDict
            (
                patches.patchSet
                (
                    wordReList(dict.lookup("patches"))
                )
            );

            // Repatch faces of the patches.
            forAllConstIter(labelHashSet, patchesDict, iter)
            {
                const polyPatch& pp = patches[iter.key()];

                Info<< "Moving faces from patch " << pp.name()
                    << " to patch " << destPatchi << endl;

                forAll(pp, i)
                {
                    changePatchID
                    (
                        mesh,
                        pp.start() + i,
                        destPatchi,
                        meshMod
                    );
                }
            }
        }
        else if (sourceType == "zone")
        {
            SortableList<label> patchFaces;

            if (dict.isDict("zone"))
            {
                autoPtr<zoneGenerator> zg
                (
                    zoneGenerator::New
                    (
                        "zone",
                        zoneTypes::face,
                        mesh,
                        dict.subDict("zone")
                    )
                );

                patchFaces = zg->generate().fZone();

                Info<< "Set "
                    << returnReduce(patchFaces.size(), sumOp<label>())
                    << " faces from zoneGenerator " << zg->name()
                    << " of type " << zg->type() << endl;
            }
            else
            {
                const word zoneName(dict.lookup("zone"));

                patchFaces = mesh.faceZones()[zoneName];

                Info<< "Read "
                    << returnReduce(patchFaces.size(), sumOp<label>())
                    << " faces from faceZone " << zoneName << endl;
            }

            patchFaces.sort();

            forAll(patchFaces, i)
            {
                label facei = patchFaces[i];

                if (mesh.isInternalFace(facei))
                {
                    FatalErrorInFunction
                        << "Face " << facei << " specified in faceZone "
                        << " is not an external face of the mesh." << endl
                        << "This application can only repatch existing boundary"
                        << " faces." << exit(FatalError);
                }

                changePatchID
                (
                    mesh,
                    facei,
                    destPatchi,
                    meshMod
                );
            }
        }
        else if (sourceType == "set")
        {
            const word setName(dict.lookup("set"));

            faceSet faces(mesh, setName);

            Info<< "Read " << returnReduce(faces.size(), sumOp<label>())
                << " faces from faceSet " << faces.name() << endl;

            // Sort (since faceSet contains faces in arbitrary order)
            labelList faceLabels(faces.toc());

            SortableList<label> patchFaces(faceLabels);

            forAll(patchFaces, i)
            {
                label facei = patchFaces[i];

                if (mesh.isInternalFace(facei))
                {
                    FatalErrorInFunction
                        << "Face " << facei << " specified in set "
                        << faces.name()
                        << " is not an external face of the mesh." << endl
                        << "This application can only repatch existing boundary"
                        << " faces." << exit(FatalError);
                }

                changePatchID
                (
                    mesh,
                    facei,
                    destPatchi,
                    meshMod
                );
            }
        }
        else
        {
            FatalErrorInFunction
                << "Invalid source type " << sourceType << endl
                << "Valid source types are 'patches' 'set'" << exit(FatalError);
        }
    }
    Info<< endl;


    // Change mesh, use motion to reforce calculation of transformation
    // tensors.
    Info<< "Doing topology modification to order faces." << nl << endl;
    autoPtr<polyTopoChangeMap> map = meshMod.changeMesh(mesh);

    // 3. Remove zeros-sized patches
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Info<< "Removing patches with no faces in them." << nl<< endl;
    filterPatches(mesh, addedPatchNames);


    if (writeCyclicMatch)
    {
        writeCyclicMatchObjs("final_", mesh);
    }


    // Set the precision of the points data to 10
    IOstream::defaultPrecision(max(10u, IOstream::defaultPrecision()));

    if (!overwrite)
    {
        runTime++;
    }
    else
    {
        mesh.setInstance(oldInstance);
    }

    // Write resulting mesh
    Info<< "Writing repatched mesh to " << runTime.name() << nl << endl;
    mesh.write();

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
