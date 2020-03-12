// Tests behavior when a module is both imported and used from different paths
module A {
  var x: int = 4;
  proc foo() {
    writeln("In A.foo()");
  }
}
module B {
  import A.foo;
}

module C {
  import A.x;
}

module D {
  use B, C;

  proc main() {
    writeln(x);
  }
}
