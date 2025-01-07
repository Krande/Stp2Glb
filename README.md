# Stp2Glb

## Installation

First install the pre-requisites for occt and build requirements from conda-forge.

```bash
pixi install
```

### Local IDE development

You can use the presets in the CMakePresets.json file. 
But first you must create a `.env.json` file (which will be ignored by git) where you point to 
the conda env `environment.build.yml`. The .env.json file should look like this,
where you fill in the path to your conda env as the "PREFIX" value.

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "env-vars",
      "hidden": true,
      "environment": {
        "PREFIX": "C:/miniforge3/envs/stp2glb"
      }
    }
  ]
}
```


## Performance metrics
Todo
