# Stp2Glb

## Install Pre-requisites

First install the pre-requisites for occt and build requirements from conda-forge using [pixi](https://pixi.sh).

```bash
pixi install
```

### Local IDE development

You can use the presets in the CMakePresets.json file (which points to the pixi environment).

### Building

To build the STP2GLB executable using shared dependencies, run the following command: 
```bash
pixi run build && pixi run install
```

To build the STP2GLB executable using static dependencies, run the following command:
```bash
pixi run -e static build && pixi run -e static install
```

## Performance metrics
Todo
