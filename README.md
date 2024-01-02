# bmod-hooks

A series of plugins for Freelancer. These should be written into the `[Libraries]` section of dacom.ini and dacomsrv.ini as appropriate. What each plugin does is detailed below:

## BmodOffsets

- Patches Freelancer.exe to regenerate restart.fl on every load of the game.
- Sets a number of offsets from the [Limit Breaking 101 page](https://wiki.the-starport.net/FL%20Binaries/limit-breaking/).
- Adds a debug-only function to generate a hashmap at runtime, for easy resolution of bad CRC hashes.

## CustomInfocards

- Patches the dynamic part of the equipment infocard for all mountable equipment to provide more relevant and accurate information for each equipment piece.

## EquipmentModifications

- Adds a config file for renaming existing classes and defining custom ones. There are some limitations for this, but definitions can be found in `DATA\CONFIG\equipment_class_mods.ini`. `name_binding` is used to rename existing classes, whilst `new_hp_type` is used to define new ones in batches of 10.
- Changes the static ship infocards to dynamic ones, with calculated values for turn rate, etc. This is done by overwriting `ShipOverrideIDS` when the function to display the infocard is called.
- Implements burst fire. This can be adjusted per-gun with the `total_projectiles_per_fire` and `time_between_multiple_projectiles` keys on `[Gun]` blocks.

The offsets defined in this plugin are set to values specific to the Better Modernized Combat Mod, but can be adjusted, repurposed and added very easily by simply calling the `PatchM` and `PatchV` functions as appropriate.
