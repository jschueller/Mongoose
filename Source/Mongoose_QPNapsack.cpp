/* ========================================================================== */
/* === QPnapsack ============================================================ */
/* ==========================================================================
    Find x that minimizes ||x-y|| while satisfying 0 <= x <= 1,
    a'x = b, lo <= b <= hi.  It is assumed that the column vector a is strictly
    positive since, in our application, the vector a is the node weights, which
    are >= 1. If a is NULL, then it is assumed that a is identically 1.
    The approach is to solve the dual problem obtained by introducing
    a multiplier lambda for the constraint a'x = b.  The dual function is

    L (lambda) = min { ||x-y||^2 + lambda (a'x - b): 0 <= x <= 1, lo <= b <= hi}

    The dual function is concave. It is continuously differentiable
    except at lambda = 0.  If mu denotes the maximizer of the dual function,
    then the solution of the primal problem is

    x = proj (y - mu*a) ,

    where proj (z) is the projection of z onto the set { x : 0 <= x <= 1}.
    Thus we have

       proj (z)_i = 1   if z_i >= 1,
                    0   if z_i <= 0,
                    z_i otherwise  .

    Note that for any lambda, the minimizing x in the dual function is

       x (lambda) = proj (y - lambda*a).

    The slope of the dual function is

      L'(lambda) = a'proj (x(lambda)) - hi (if lambda > 0)
                   a'proj (x(lambda)) - lo (if lambda < 0)

    The minimizing b in the dual function is b = hi if lambda > 0 and b = lo
    if b <= 0.  When L' (lamdbda) = 0 with lambda != 0, either x'a = hi or
    x'a = lo.  The minimum is attained at lambda = 0 if and only if the
    slope of L is negative at lambda = 0+ and positive at lambda = 0-.
    This is equivalent to the inequalities

              lo <= a' proj (y) <= hi .

    The solution technique is to start with an initial guess lambda for
    mu and search for a zero of L'. We have the following cases:

    1. lambda >= 0, L'(lambda+) >= 0: mu >= lambda. If L' = 0, then done. 
                                    Otherwise, increase lambda using napup until
                                    slope vanishes

    2. lambda <= 0, L'(lambda-) <= 0: mu <= lambda. If L' = 0, then done. 
                                    Otherwise, decrease lambda using napdown 
                                    until slope vanishes

    3. lambda >= 0, L'(lambda+)  < 0: If L' (0-) < 0, then mu < 0. Call napdown
                                    with lambda = 0 as starting guess.  If
                                    L' (0+) > 0, then 0 < mu < lambda. Call
                                    napdown with given starting guess lambda.
                                    Otherwise, if L' (0+) <= 0, then mu = 0.

    4. lambda <= 0, L'(lambda-)  > 0: If L' (0+) > 0, then mu > 0. Call napup
                                    with lambda = 0 as starting guess.  If
                                    L' (0-) < 0, then lambda < mu < 0.  Call
                                    napup with given starting guess lambda.
                                    Otherwise, if L' (0-) >= 0, then mu = 0.

    By the "free set" we mean those i for which 0 < x_i (lambda) < 1.  The
    total time taken by napsack is O (n + h log n), where n is the size of y,
    h is the number of times an element of x (lambda) moves off the boundary
    into the free set (entries between zero and one) plus the number of times
    elements move from the free set to the opposite boundary.  A heap is used
    to hold the entries in the boundary and in the free set.  If the slope
    vanishes at either the starting lambda or at lambda = 0, then no heap is
    constructed, and the time is just O (n).

    If we have a guess for which components of x will be free at the optimal
    solution, then we can obtain a good guess for the starting lambda by
    setting the slope of the dual function to zero and solving for lambda.  If
    FreeSet_status is not NULL, then the FreeSet_status array is used to
    compute a starting guess for lambda based on the estimated free indices.
    Note that FreeSet_status is an INPUT array, it is not modified by this
    routine.
   ========================================================================== */

#include "Mongoose_Internal.hpp"
#include "Mongoose_QPNapsack.hpp"
#include "Mongoose_QPNapDown.hpp"
#include "Mongoose_QPNapUp.hpp"
#include "Mongoose_Debug.hpp"
#include "Mongoose_Logger.hpp"

namespace Mongoose
{

void checkatx (Double *x, Double *a, Int n, Double lo, Double hi)
{
    Double atx = 0. ; 
    int ok = 1 ;
    for (Int k = 0 ; k < n ; k++)
    {
        if (x [k] < 0.) { ok = 0 ; PR (("x [%ld] = %g < 0!\n", k, x [k])) ; }
        if (x [k] > 1.) { ok = 0 ; PR (("x [%ld] = %g > 1!\n", k, x [k])) ; }
        atx += ((a == NULL) ? 1 : a [k]) * x [k] ;
    }
    if (atx < lo - 0.001) { ok = 0 ; }
    if (atx > hi + 0.001) { ok = 0 ; }
    if (!ok)
    {
        PR (("napsack error! lo %g a'x %g hi %g\n", lo, atx, hi)) ;
        FFLUSH ;
        ASSERT (0) ;
    }
}

Double QPnapsack        /* return the final lambda */
(
    Double *x,      /* holds y on input, and the solution x on output */
    Int n,          /* size of x, constraint lo <= a'x <= hi */
    Double lo,      /* partition lower bound */
    Double hi,      /* partition upper bound */
    Double *Gw,     /* vector of nodal weights */
    Double Lambda,  /* initial guess for lambda */
    Int *FreeSet_status, /* FreeSet_status [i] = +1,-1, or 0 on input,
                       for 3 cases: x_i =1,0, or 0< x_i< 1.  Not modified. */
    Double *w,      /* work array of size n   */
    Int *heap1,     /* work array of size n+1 */
    Int *heap2      /* work array of size n+1 */
)
{
    Double lambda = Lambda;
    PR (("QPNapsack start [\n")) ;

    /* ---------------------------------------------------------------------- */
    /* compute starting guess if FreeSet_status is provided and lambda != 0 */
    /* ---------------------------------------------------------------------- */

    if ((FreeSet_status != NULL) && (lambda != 0))
    {
        Double asum = (lambda > 0 ? -hi : -lo);
        Double a2sum = 0.;

        for (Int k = 0; k < n; k++)
        {
            if (FreeSet_status[k] == 1)
            {
                asum += Gw[k];
            }
            else if (FreeSet_status[k] == 0)
            {
                Weight ai = Gw[k];
                asum += x[k] * ai;
                a2sum += ai * ai;
            }
        }

        if (a2sum != 0.) lambda = asum / a2sum;
    }

    /* ---------------------------------------------------------------------- */
    /* compute the initial slope */
    /* ---------------------------------------------------------------------- */

    Double slope = 0;
    for (Int k = 0; k < n; k++)
    {
        Double xi = x[k] - Gw[k] * lambda;
        if (xi >= 1.)
        {
            slope += Gw[k];
        }
        else if (xi > 0.)
        {
            slope += Gw[k] * xi;
        }
    }
    PR (("slope %g lo %g hi %g\n", slope, lo, hi)) ;

    /* remember: must still adjust slope by "-hi" or "-lo" for its final value */

    if ((lambda >= 0.) && (slope >= hi)) /* case 1 */
    {
        if (slope > hi)
        {
            PR (("napsack case 1 up\n")) ;
            lambda = QPnapup(x, n, lambda, Gw, hi, w, heap1, heap2);
            lambda = MONGOOSE_MAX2(0., lambda);
        }
        else
        {
            PR (("napsack case 1 nothing\n")) ;
        }
    }
    else if ((lambda <= 0.) && (slope <= lo)) /* case 2 */
    {
        if (slope < lo)
        {
            PR (("napsack case 2 down\n")) ;
            lambda = QPnapdown(x, n, lambda, Gw, lo, w, heap1, heap2);
            lambda = MONGOOSE_MIN2(lambda, 0.);
        }
        else
        {
            PR (("napsack case 2 nothing\n")) ;
        }
    }
    else /* case 3 or 4 */
    {
        if (lambda != 0.)
        {
            Double slope0 = 0.;
            for (Int k = 0; k < n; k++)
            {
                Double xi = x[k];
                if (xi >= 1.)
                {
                    slope0 += Gw[k];
                }
                else if (xi > 0.)
                {
                    slope0 += Gw[k] * xi;
                }
            }

            if ((lambda >= 0) && (slope < hi)) /* case 3 */
            {
                if (slope0 < lo)
                {
                    PR (("napsack case 3a down\n")) ;
                    lambda = 0.;
                    lambda = QPnapdown(x, n, lambda, Gw, lo, w, heap1, heap2);
                    if (lambda > 0.) lambda = 0.;
                }
                else if (slope0 > hi)
                {
                    PR (("napsack case 3b down\n")) ;
                    lambda = QPnapdown(x, n, lambda, Gw, hi, w, heap1, heap2);
                    if (lambda < 0.) lambda = 0.;
                }
                else
                {
                    PR (("napsack case 3c nothing\n")) ;
                    lambda = 0.;
                }
            }
            else /* ( (lambda <= 0) && (slope > lo) )  case 4 */
            {
                if (slope0 > hi)
                {
                    PR (("napsack case 4a up\n")) ;
                    lambda = 0.;
                    lambda = QPnapup(x, n, lambda, Gw, hi, w, heap1, heap2);
                    lambda = MONGOOSE_MAX2(lambda, 0.);
                }
                else if (slope0 < lo)
                {
                    PR (("napsack case 4b up\n")) ;
                    lambda = QPnapup(x, n, lambda, Gw, lo, w, heap1, heap2);
                    lambda = MONGOOSE_MIN2(0., lambda);
                }
                else
                {
                    PR (("napsack case 4c nothing\n")) ;
                    lambda = 0.;
                }
            }
        }
        else /* lambda == 0 */
        {
            if (slope < hi) /* case 3 */
            {
                if (slope < lo)
                {
                    PR (("napsack case 3d down\n")) ;
                    lambda = QPnapdown(x, n, lambda, Gw, lo, w, heap1, heap2);
                    lambda = MONGOOSE_MIN2(0., lambda);
                }
                else
                {
                    PR (("napsack case 3e nothing\n")) ;
                }
            }
            else /* ( slope > lo )                    case 4 */
            {
                if (slope > hi)
                {
                    PR (("napsack case 4d up\n")) ;
                    lambda = QPnapup(x, n, lambda, Gw, hi, w, heap1, heap2);
                    lambda = MONGOOSE_MAX2(lambda, 0.);
                }
                else
                {
                    PR (("napsack case 4e nothing\n")) ;
                }
            }
        }
    }

    /* ---------------------------------------------------------------------- */
    /* replace x by x (lambda) */
    /* ---------------------------------------------------------------------- */

    PR (("lambda %g\n", lambda)) ;
    if (lambda == 0.)
    {
        for (Int k = 0; k < n; k++)
        {
            Double xi = x[k];
            // TODO: Rewrite nested ternary operator
            x[k] = (xi < 0. ? 0. : xi > 1. ?  1. : xi);
        }
        checkatx (x, Gw, n, lo, hi) ;
    }
    else
    {
        for (Int k = 0; k < n; k++)
        {
            Double xi = x[k] - Gw[k] * lambda;
            // TODO: Rewrite nested ternary operator
            x[k] = (xi < 0. ? 0. : xi > 1. ?  1. : xi);
        }
        checkatx (x, Gw, n, lo, hi) ;
    }
    PR (("QPNapsack done ]\n")) ;

    return lambda;
}

} // end namespace Mongoose
