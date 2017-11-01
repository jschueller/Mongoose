#pragma once

#include "SuiteSparse_config.h"
#include <string>

namespace Mongoose
{

/* Type definitions */
typedef SuiteSparse_long Int;

/* Enumerations */
enum MatchingStrategy
{
    Random,
    HEM,
    HEMPA,
    HEMDavisPA
};

enum GuessCutType
{
    GuessQP,
    GuessRandom,
    GuessNaturalOrder
};

enum MatchType
{
    MatchType_Orphan    = 0,
    MatchType_Standard  = 1,
    MatchType_Brotherly = 2,
    MatchType_Community = 3
};

struct Options
{
    Int randomSeed;

    /** Coarsening Options ***************************************************/
    Int coarsenLimit;
    MatchingStrategy matchingStrategy;
    bool doCommunityMatching;
    double davisBrotherlyThreshold;

    /** Guess Partitioning Options *******************************************/
    GuessCutType guessCutType; /* The guess cut type to use */

    /** Waterdance Options ***************************************************/
    Int numDances; /* The number of interplays between FM and QP
                      at any one coarsening level. */

    /**** Fidducia-Mattheyes Options *****************************************/
    bool useFM;              /* Flag governing the use of FM             */
    Int fmSearchDepth;       /* The # of non-positive gain move to make  */
    Int fmConsiderCount;     /* The # of heap entries to consider        */
    Int fmMaxNumRefinements; /* Max # of times to run FidduciaMattheyes  */

    /**** Quadratic Programming Options **************************************/
    bool useQPGradProj;         /* Flag governing the use of gradproj       */
    double gradProjTolerance;   /* Convergence tol for projected gradient   */
    Int gradprojIterationLimit; /* Max # of iterations for gradproj         */

    /** Final Partition Target Metrics ***************************************/
    double targetSplit;        /* The desired split ratio (default 50/50)  */
    double softSplitTolerance; /* The allowable soft split tolerance.      */
    /* Cuts within this tolerance are treated   */
    /* equally.                                 */

    /* Constructor & Destructor */
    static Options *Create();
};

class Graph
{
public:
    /** CSparse3 Interoperability ********************************************/
    Int cs_n;     /** # columns                       */
    Int cs_m;     /** # rows                          */
    Int cs_nz;    /** # triplet entries or -1         */
    Int cs_nzmax; /** max # nonzeros                  */

    /** Graph Data ***********************************************************/
    Int n;     /** # vertices                      */
    Int nz;    /** # edges                         */
    Int *p;    /** Column pointers                 */
    Int *i;    /** Row indices                     */
    double *x; /** Edge weight                     */
    double *w; /** Node weight                     */
    double X;  /** Sum of edge weights             */
    double W;  /** Sum of node weights             */

    double H; /** Heuristic max penalty to assess */

    /** Partition Data *******************************************************/
    bool *partition;     /** T/F denoting partition side     */
    double *vertexGains; /** Gains for each vertex           */
    Int *externalDegree; /** # edges lying across the cut    */
    Int *bhIndex;        /** Index+1 of a vertex in the heap */
    Int *bhHeap[2];      /** Heap data structure organized by
                            boundaryGains descending         */
    Int bhSize[2];       /** Size of the boundary heap       */

    /** Cut Cost Metrics *****************************************************/
    double heuCost;   /** cutCost + balance penalty       */
    double cutCost;   /** Sum of edge weights in cut set  */
    double W0;        /** Sum of partition 0 node weights */
    double W1;        /** Sum of partition 1 node weights */
    double imbalance; /** Degree to which the partitioning
                          is imbalanced, and this is
                          computed as (0.5 - W0/W).       */

    /** Matching Data ********************************************************/
    Graph *parent;    /** Link to the parent graph        */
    Int clevel;       /** Coarsening level for this graph */
    Int cn;           /** # vertices in coarse graph      */
    Int *matching;    /** Linked List of matched vertices */
    Int *matchmap;    /** Map from fine to coarse vertices */
    Int *invmatchmap; /** Map from coarse to fine vertices */
    Int *matchtype;   /** Vertex's match classification
                           0: Orphan
                           1: Standard (random, hem, shem)
                           2: Brotherly
                           3: Community                   */
    Int singleton;

    /* Constructor & Destructor */
    static Graph *Create(Int _n, Int _nz);
    static Graph *Create(Graph *_parent);
    ~Graph();
    bool initialize(const Options *options);
};

/**
 * Generate a Graph from a Matrix Market file.
 *
 * Generate a Graph class instance from a Matrix Market file. The matrix
 * contained in the file must be sparse, real, and square. If the matrix
 * is not symmetric, it will be made symmetric with (A+A')/2. If the matrix has
 * more than one connected component, the largest will be found and the rest
 * discarded. If a diagonal is present, it will be removed.
 *
 * @param filename the filename or path to the Matrix Market File.
 */
Graph *readGraph(const std::string &filename);

/**
 * Generate a Graph from a Matrix Market file.
 *
 * Generate a Graph class instance from a Matrix Market file. The matrix
 * contained in the file must be sparse, real, and square. If the matrix
 * is not symmetric, it will be made symmetric with (A+A')/2. If the matrix has
 * more than one connected component, the largest will be found and the rest
 * discarded. If a diagonal is present, it will be removed.
 *
 * @param filename the filename or path to the Matrix Market File.
 */
Graph *readGraph(const char *filename);

int ComputeEdgeSeparator(Graph *);
int ComputeEdgeSeparator(Graph *, const Options *);

} // end namespace Mongoose