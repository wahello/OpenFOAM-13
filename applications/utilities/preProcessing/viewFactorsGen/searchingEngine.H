randomGenerator rndGen(653213);

// Determine mesh bounding boxes:
List<treeBoundBox> meshBb
(
    1,
    treeBoundBox
    (
        boundBox(coarseMesh.points(), false)
    ).extend(1e-3)
);

// Dummy bounds dictionary
dictionary dict;
dict.add("bounds", meshBb);
dict.add
(
    "distributionType",
    searchableSurfaces::distributedTriSurface::distributionTypeNames_
    [
        searchableSurfaces::distributedTriSurface::FROZEN
    ]
);
dict.add("mergeDistance", small);

labelHashSet includePatches;
forAll(patches, patchi)
{
    const polyPatch& pp = patches[patchi];
    if (!pp.coupled() && !isA<emptyPolyPatch>(pp))
    {
        includePatches.insert(patchi);
    }
}

labelList triSurfaceToAgglom(5*nFineFaces);

const triSurface localSurface = triangulate
(
    patches,
    includePatches,
    finalAgglom,
    triSurfaceToAgglom,
    globalNumbering,
    coarsePatches
);


searchableSurfaces::distributedTriSurface surfacesMesh
(
    IOobject
    (
        "wallSurface.stl",
        runTime.constant(),
        searchableSurface::geometryDir(runTime),
        runTime,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    localSurface,
    dict
);


triSurfaceToAgglom.resize(surfacesMesh.size());

//surfacesMesh.searchableSurface::write();

surfacesMesh.setField(triSurfaceToAgglom);
