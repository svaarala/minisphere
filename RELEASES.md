Release Notes
=============

minisphere 3.1
--------------

* SphereFS prefixes have changed.  Single-character prefixes are now used for
  SphereFS paths instead of the `~usr`, `~sgm`, and `~sys` aliases used in
  previous versions.  Any code depending on the old prefixes will need to be
  updated.
* The user data folder has been renamed to "minisphere".  This was done to be
  more friendly to Linux users, for whom filenames with spaces are often
  inconvenient.  If you need to keep your save data from minisphere 3.0, move
  it into `<documents>/minisphere/save`.
* The Galileo API has been updated with new features.  These improvements bring
  some minor breaking changes with them as well.  Refer to the API reference
  for details.
* The search path for CommonJS modules has changed since 3.0.  Modules are now
  searched for in `@/lib/` instead of `@/commonjs/`.
* `ListeningSocket` has been renamed to `Server`.  Networking code will need to
  be updated.

minisphere 3.0
--------------

* SphereFS sandboxing is more comprehensive.  API calls taking filenames no
  longer accept absolute paths, and will throw a sandbox violation error if one
  is given.  As this behavior matches Sphere 1.x, few if any games should be
  affected by the change.
* minisphere 3.0 stores user data in a different location than past versions.
  Any save data stored in `<documents>/minisphere` will need to be moved into
  `<documents>/Sphere 2.0/saveData` to be picked up by the new version.
* miniRT has been overhauled for the 3.0 release and is now provided in the form
  of CommonJS modules.  Games will no longer be able to pull in miniRT globally
  using `RequireSystemScript()` and should instead use `require()` to get at the
  individual modules making up of the library.  Games relying on the older
  miniRT bits will need to be updated.
* When using the SSJ command-line debugger, source code is downloaded directly
  from minisphere without accessing the original source tree (which need not be
  present).  When CoffeeScript or TypeScript are used, the JavaScript code
  generated by the transpiler, not the original source, will be provided to the
  debugger.
* CommonJS module resolution has changed.  Previously the engine searched for
  modules in `~sgm/modules`.  minisphere 3.0 changes this to `~sgm/commonjs`.
* `Assert()` behavior differs from past releases.  Failing asserts will no
  longer throw, and minisphere completely ignores the error if no debugger is
  attached.  This was done for consistency with assert semantics in other
  programming languages.
* When a debugger is attached, minisphere 3.0 co-opts the `F12` key, normally
  used to take screenshots, for the purpose of triggering a prompt breakpoint.
  This may be surprising for those who aren�t expecting it.
* TypeScript support in minisphere 3.0 is mostly provisional.  In order to
  maintain normal Sphere script semantics, the TypeScript compiler API
  `ts.transpile()` is used to convert TypeScript to JavaScript before running
  the code.  While this allows all valid TypeScript syntax, some features such
  as compiler-enforced typing and module import will likely not be available.
* Official Windows builds of minisphere are compiled against Allegro 5.1.  Linux
  builds are compiled against Allegro 5.0 instead, which disables several
  features.  Notably, Galileo shader support is lost.  If a game attempts to
  construct a `ShaderProgram` object in a minisphere build compiled against
  Allegro 5.0, the constructor will throw an error.  Games using shaders should
  be prepared to handle the error.
* If SSJ terminates prematurely during a debugging session, either because of a
  crash or due to accidentally pressing `Ctrl+C`, minisphere will wait up to 30
  seconds for the debugger to reconnect.  During this time, you may enter
  `ssj -c` on the command line to connect to the running engine instance and
  pick up where you left off.
