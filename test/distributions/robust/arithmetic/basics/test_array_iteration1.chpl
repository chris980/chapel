use driver_arrays;

def foo(Dom, Arr) {
  for e in Arr do
    e = next();

  forall e in Arr do
    e += 2;
}

foo(Dom1D, A1D);
foo(Dom2D, A2D);
foo(Dom3D, A3D);
foo(Dom4D, A4D);
foo(Dom2D64, A2D64);

outputArrays();
