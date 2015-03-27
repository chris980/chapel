/* 2D matrix helper functions.
 *
 */

use Assert;

// Transpose inputMatrix and store in outputMatrix.
proc transpose(ref outputMatrix: [?D] real, ref inputMatrix: [?E] real)
  where D.rank == 2 && E.rank == 2
{
  if boundsChecking {
    assert(D.dim(1) == E.dim(2) && D.dim(2) == E.dim(1),
           "Dimensions of outputMatrix are not transpose of inputMatrix.");
  }

  var outCols = inputMatrix.domain.dim(1),
    outRows = inputMatrix.domain.dim(2);
  forall (row, col) in outputMatrix.domain {
    outputMatrix[row, col] = inputMatrix[col, row];
  }
}

// Calculate dot product (matrix multiplication) of A and B and store result in
// C. Specifically, it does C = A * B.
proc dotProduct(ref C: [?DC] real, ref A: [?DA] real, ref B: [?DB] real)
  where DC.rank == 2 && DA.rank == 2 && DB.rank == 2
{
  if boundsChecking {
    assert(DC.dim(1) == DA.dim(1) && DC.dim(2) == DB.dim(2),
           "Dimensions for C, A, or B do not work for dot product.");
    assert(DA.dim(2) == DB.dim(1),
           "Inner array dimensions do not match.");
  }

  // // This is the chapelerific way to write the 2d matrix dot
  // // product. Unfortunately, it is pretty slow (2015-03-15) at present so
  // // implement nested forall loops that are functionally the same thing.
  // var rows = A.domain.dim(1),
  //   cols = B.domain.dim(2);
  // forall (row, col) in C.domain {
  //   C[row, col] = + reduce (A[row, 1..] * B[1.., col]);
  // }

  forall col in DB.dim(2) {
    forall row in DA.dim(1) {
      C[row, col] = 0.0;

      for z in DA.dim(2) {
        C[row, col] += A[row, z] * B[z, col];
      }
    }
  }
}
