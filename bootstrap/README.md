# bootstrap - An executable

The `bootstrap` executable is a <SUMMARY-OF-FUNCTIONALITY>.

Note that the `bootstrap` executable in this package provides `build2` metadata.


## Usage

To start using `bootstrap` in your project, add the following build-time
`depends` value to your `manifest`, adjusting the version constraint as
appropriate:

```
depends: * bootstrap ^<VERSION>
```

Then import the executable in your `buildfile`:

```
import! [metadata] <TARGET> = bootstrap%exe{<TARGET>}
```


## Importable targets

This package provides the following importable targets:

```
exe{<TARGET>}
```

<DESCRIPTION-OF-IMPORTABLE-TARGETS>


## Configuration variables

This package provides the following configuration variables:

```
[bool] config.bootstrap.<VARIABLE> ?= false
```

<DESCRIPTION-OF-CONFIG-VARIABLES>
