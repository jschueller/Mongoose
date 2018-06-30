/* ========================================================================== */
/* === Include/Mongoose_EdgeSeparator.hpp =================================== */
/* ========================================================================== */

/* -----------------------------------------------------------------------------
 * Mongoose Graph Partitioning Library  Copyright (C) 2017-2018,
 * Scott P. Kolodziej, Nuri S. Yeralan, Timothy A. Davis, William W. Hager
 * Mongoose is licensed under Version 3 of the GNU General Public License.
 * Mongoose is also available under other licenses; contact authors for details.
 * -------------------------------------------------------------------------- */

#pragma once

#include "Mongoose_Graph.hpp"
#include "Mongoose_EdgeCutOptions.hpp"

namespace Mongoose
{

struct EdgeCut
{
    bool *partition;     /** T/F denoting partition side     */
    Int n;               /** # vertices                      */

    /** Cut Cost Metrics *****************************************************/
    double cut_cost;    /** Sum of edge weights in cut set    */
    Int cut_size;       /** Number of edges in cut set        */
    double w0;          /** Sum of partition 0 vertex weights */
    double w1;          /** Sum of partition 1 vertex weights */
    double imbalance;   /** Degree to which the partitioning
                            is imbalanced, and this is
                            computed as (0.5 - W0/W).         */

    // desctructor (no constructor)
    ~EdgeCut();
};

EdgeCut *edge_cut(const Graph *);
EdgeCut *edge_cut(const Graph *, const EdgeCut_Options *);

} // end namespace Mongoose
