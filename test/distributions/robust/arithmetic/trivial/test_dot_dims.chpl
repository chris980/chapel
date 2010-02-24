use driver_domains;

writeln(Dom1D.dims());
writeln(Dom2D.dims());
writeln(Dom3D.dims());
writeln(Dom4D.dims());
writeln(Dom2D64.dims());

var D: domain(2) distributed Dist2D = [200..300, 400..500];
var A: [D] real;

writeln(A.domain.dims());
