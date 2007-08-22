use Math;
use Tensor;
use FTree;
use Quadrature;
use TwoScale;
use Fn1d;

class Function1d {
    var k             = 5;    // use first k Legendre polynomials as the basis in each box
    var thresh        = 1e-5; // truncation threshold for small wavelet coefficients
    var f: Fn1d       = nil;  // analytic function f(x) to be projected into the numerical represntation
    var initial_level = 2;    // initial level of refinement
    var max_level     = 30;   // maximum level of refinement mostly as a sanity check
    var compressed    = false;// keep track of what basis we are in

    // hashes for sum and difference coeffs
    var s = FTree(order=k);
    var d = FTree(order=k);

    // two-scale relationship matrices
    var hgDom = [0..2*k-1, 0..2*k-1];
    var hg    : [hgDom] real;
    var hgT   : [hgDom] real;

    // Quadrature stuff
    var quadDom   = [0..k-1];
    var quad_x    : [quadDom] real;    // points
    var quad_w    : [quadDom] real;    // weights
    var quad_npt  = k;

    var quad_phiDom = [0..k-1, 0..k-1];
    var quad_phi    : [quad_phiDom] real;  // phi[point,i]
    var quad_phiT   : [quad_phiDom] real;  // phi[point,i] transpose
    var quad_phiw   : [quad_phiDom] real;  // phi[point,i]*weight[point]

    // blocks of the block tridiagonal derivative operator
    //var rm, r0, rp;

    // constructors and helpers

    def initialize() {
        writeln("Creating Function: k=", k, " thresh=", thresh);

        writeln("  initializing two-scale relation coefficients");
        hg  = getCoeffs(k); //FIXME: rename this fcn
        hgT = transpose(tensorDup(hg));

        writeln("  initializing quadrature coefficients");
        init_quadrature(k);

        // blocks of the block tridiagonal derivative operator
        // rm, r0, rp = make_dc_periodic(k)

        // initial refinement of analytic function f(x)
        if f != nil {
            writeln("  performing initial refinement of f(x)");
            for l in 0..2**initial_level-1 do
                refine(initial_level, l);
        }

        writeln("done.");
    }

    def init_quadrature(order: int) {
        quad_x   = getGLPoints(k);
        quad_w   = getGLWeights(k);
        if (quad_npt != quad_w.numElements) then
            halt("quadrature length mismatch");

        for i in 0..quad_npt-1 {
            var p = phi(quad_x[i], k);
            for m in 0..k-1 {
                quad_phi[i, m]  = p[m];
                quad_phiw[i, m] = quad_w[i]*p[m];
            }
        }

        quad_phiT = transpose(tensorDup(quad_phi));
    }


    /** s[n][l] = integral(phi[n][l](x) * f(x))
      for box (n,l) project f(x) using quadrature rule
      into scaling function basis
      @param n   level
      @param l   box index
      @return    s[n][l] = integral(phi[n][l](x) * f(x))
     */
    def project(n: int, l: int) {
        var s     : [quadDom] real;
        var h     = 0.5 ** n;
        var scale = sqrt(h);

        for mu in 0..k-1 {      // quadDom.dim(1)
            var x  = (l + quad_x[mu]) * h;
            var fx = f.f(x);
            for i in 0..k-1 do // quadDom.dim(2)
                s[i] += scale * fx * quad_phiw[mu, i];
        }

        return s;
    }


    /** refine numerical representation of f(x) to desired tolerance
      @param n   level
      @param l   box index
     */
    def refine(n: int, l: int) {
        // project f(x) at next level
        var s0 = project(n+1, 2*l);
        var s1 = project(n+1, 2*l+1);
        var s: [0..2*k-1] real;

        //writeln("   refine at (", n, ", ", l, ")");

        s[0..k-1]   = s0;
        s[k..2*k-1] = s1;

        // apply the two scale relationship to get difference coeff
        // in 1d this is O(k^2) flops
        var d = transform(s, hgT);

        // check to see if within tolerance
        // normf() is Frobenius norm == 2-norm for vectors
        var nf = normf(d[k..2*k-1]);
        if((nf < thresh) || (n >= (max_level-1))) {
            this.s[n+1, 2*l]   = s0;
            this.s[n+1, 2*l+1] = s1;
        }
        else {
            // these recursive calls on sub-trees can go in parallel
            // if the HashMap is syncronized
            refine(n+1, 2*l  );
            refine(n+1, 2*l+1);
        }
    }


    /* 
       Evaluate numerical representation of f at x.  Performed in scaling
       function basis only.
     */
    def this(x) {
        if compressed then reconstruct();
        return evaluate(0, 0, x);
    }


    /*
       eval f(x) using adaptively refined numerical representation of f(x)
       answer should be within tolerance of the analytical f(x)

       Descend tree looking for box (n,l) with scaling function
       coefficients containing the point x.
     */
    def evaluate(in n=0, in l=0, in x): real {
        if s.has_coeffs(n, l) {
            var p = phi(x, k);
            return inner(s[n, l], p)*sqrt(2.0**n);

        } else {
            (n, l, x) = (n+1, 2*l, 2*x);
            if x >= 1 then (l, x) = (l+1, x-1);
            return evaluate(n, l, x);
        }
    }


    def reconstruct() { }
}

def main() {
    use MadFn1d;

    writeln("Mad Chapel -- One Step Beyond\n");

    writeln("** var F1 : Function1d = nil;");
    var F1 : Function1d = nil;

    writeln("\n** var F2 = Function1d();");
    var F2 = Function1d();

    //writeln("Phi Norms:\n", F2.quad_phi,
    //        "\nPhi Transpose:\n", F2.quad_phiT,
    //        "\nPhi Weights:\n", F2.quad_phiw
    //       );

    writeln("\n** var F3 = Function1d(f=test3);");
    var test3 = Fn_Test3();
    var F3    = Function1d(k=7, thresh=1e-10, f=test3);

    //writeln("Phi Norms:\n", F2.quad_phi,
    //        "\nPhi Transpose:\n", F2.quad_phiT,
    //        "\nPhi Weights:\n", F2.quad_phiw
    //       );

    writeln("\nEvaluating F3 on [0, 1] (singularity at 0.5):");

    for i in 1..10 do
        writeln(" -- ", 0.1*i, ": (analytic) test3=", test3.f(0.1*i), " (numeric) F3=", F3(0.1*i));
}
