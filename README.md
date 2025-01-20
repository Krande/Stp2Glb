# Stp2Glb

## Usage

```bash
STEP to GLB converter
Usage: STP2GLB.exe [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --stp REQUIRED              STEP filepath
  --glb REQUIRED              GLB filepath
  --lin-defl :FLOAT in [0 - 1] [0.1]
                              Linear deflection
  --ang-defl :FLOAT in [0 - 1] [0.5]
                              Angular deflection
  --rel-defl                  Relative deflection
  --debug                     Debug mode. More robust but slower
  --solid-only                Solid only
  --max-geometry-num [0]      Maximum number of geometries to convert
  --filter-names-include      Include Filter name. Command separated list
  --filter-names-file-include Include Filter name file
  --filter-names-exclude      Exclude Filter name. Command separated list
  --filter-names-file-exclude Exclude Filter name file
  --tessellation-timeout [30]
                              Tessellation timeout
```


 
## Development

### Install Pre-requisites

The pre-requisites build requirements are conda packages handled by using [pixi](https://pixi.sh).

Note that on windows the MSVC c++ compiler toolchain is used. 
So a pre-requisite on windows is that you have installed VS or VS build tools from https://visualstudio.microsoft.com/downloads/?q=build+tools

### Building

To build the STP2GLB executable using shared dependencies, run the following command: 
```bash
pixi run build && pixi run install
```

To build the STP2GLB executable using static dependencies, run the following command:
```bash
pixi run -e static build && pixi run -e static install
```


### Local IDE development

You can use the presets in the CMakePresets.json file (which points to the pixi environment). Just make sure you've
either installed the pixi environment using `pixi install` or have run any of the build commands above.


## Performance metrics
Todo
